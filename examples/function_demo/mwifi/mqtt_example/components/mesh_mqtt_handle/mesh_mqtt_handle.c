/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "mesh_mqtt_handle.h"
#include "mlink.h"
#include "mwifi.h"
#include "esp_wifi.h"

typedef struct {
    uint8_t addr[6]; /**< source MAC address  */
    size_t size;     /**< Received size */
    uint8_t *data; /**< Received data */
} queue_data_t;

enum {
    UNSUBSCRIBED,
    SUBSCRIBED,
};

static const char *TAG                   = "mesh_mqtt";
static xQueueHandle g_queue_handle       = NULL;
static xSemaphoreHandle g_connect_handle = NULL;
static esp_mqtt_client_handle_t g_client = NULL;
static bool g_mqtt_connect_flag          = false;
static bool g_mqtt_start_flag            = false;

bool mesh_mqtt_is_connect()
{
    return g_mqtt_connect_flag;
}

static mdf_err_t mqtt_update_subdev(uint8_t type, uint8_t *subdev_list, size_t subdev_num)
{
    MDF_PARAM_CHECK(subdev_list);
    MDF_PARAM_CHECK(g_client);

    char *addr_list  = NULL;
    char mac_str[13] = {0};
    char *data       = NULL;
    char *topic_str  = NULL;
    uint8_t root_mac[6] = {0};

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));

    for (int i = 0; i < subdev_num; ++i) {
        mlink_json_pack(&addr_list, "[]", mlink_mac_hex2str(subdev_list + i * MWIFI_ADDR_LEN, mac_str));

        /* subscribe topic: /topic/subdev/mac/recv eg: /topic/subdev/240ac4085480/recv*/
        asprintf(&topic_str, "/topic/subdev/%s/recv", mac_str);

        if (type == SUBSCRIBED) {
            MDF_LOGI("subscribe: %s", topic_str);
            esp_mqtt_client_subscribe(g_client, topic_str, 0);
        } else {
            MDF_LOGI("unsubscribe: %s", topic_str);
            esp_mqtt_client_unsubscribe(g_client, topic_str);
        }

        MDF_FREE(topic_str);
    }

    /* gateway topic: /topic/gateway/mac/update eg: /topic/gateway/240ac4085480/update */
    mlink_mac_hex2str(root_mac, mac_str);
    mlink_json_pack(&data, "subdev_addrs", addr_list);
    mlink_json_pack(&data, "gateway_addr", mac_str);

    MDF_LOGD("size: %d, data: %s", strlen(data), data);

    asprintf(&topic_str, "/topic/gateway/%s/update", mac_str);
    esp_mqtt_client_publish(g_client, topic_str, data, 0, 1, 0);
    MDF_FREE(topic_str);

    MDF_FREE(addr_list);
    MDF_FREE(data);

    return MDF_OK;
}

mdf_err_t mesh_mqtt_subscribe(uint8_t *subdev_list, size_t subdev_num)
{
    return mqtt_update_subdev(SUBSCRIBED, subdev_list, subdev_num);
}

mdf_err_t mesh_mqtt_unsubscribe(uint8_t *subdev_list, size_t subdev_num)
{
    return mqtt_update_subdev(UNSUBSCRIBED, subdev_list, subdev_num);
}

mdf_err_t mesh_mqtt_write(uint8_t *addr, void *data, size_t size)
{
    MDF_PARAM_CHECK(g_client);
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(data);

    char *topic_str  = NULL;

    /* publish data topic: /topic/subdev/mac/send eg: /topic/subdev/240ac4085480/send*/
    asprintf(&topic_str, "/topic/subdev/%02x%02x%02x%02x%02x%02x/send", MAC2STR(addr));
    esp_mqtt_client_publish(g_client, topic_str, data, size, 0, 0);
    MDF_FREE(topic_str);

    return MDF_OK;
}

mdf_err_t mesh_mqtt_read(uint8_t *addr, void **data, size_t *size, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(g_client);
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);

    queue_data_t q_data = {0x0};

    if (xQueueReceive(g_queue_handle, &q_data, wait_ticks) != pdPASS) {
        MDF_LOGD("Read queue timeout");
        return MDF_FAIL;
    }

    *size = q_data.size;
    *data = q_data.data;
    memcpy(addr, q_data.addr, 6);

    return MDF_OK;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            MDF_LOGD("MQTT_EVENT_CONNECTED");
            xSemaphoreGive(g_connect_handle);
            g_mqtt_connect_flag = true;
            mdf_event_loop_send(MDF_EVENT_CUSTOM_MQTT_CONNECT, NULL);
            break;

        case MQTT_EVENT_DISCONNECTED:
            MDF_LOGD("MQTT_EVENT_DISCONNECTED");
            g_mqtt_connect_flag = false;
            mdf_event_loop_send(MDF_EVENT_CUSTOM_MQTT_DISCONNECT, NULL);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            MDF_LOGD("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            MDF_LOGD("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            MDF_LOGD("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA: {
            MDF_LOGD("MQTT_EVENT_DATA, topic: %.*s, data: %.*s",
                     event->topic_len, event->topic, event->data_len, event->data);

            queue_data_t q_data = {0x0};
            q_data.data = MDF_MALLOC(event->data_len);
            q_data.size = event->data_len;
            mlink_mac_str2hex(event->topic + strlen("/topic/subdev/"), q_data.addr);
            memcpy(q_data.data, event->data, event->data_len);

            if (xQueueSend(g_queue_handle, &q_data, 0) != pdPASS) {
                MDF_LOGD("Send receive queue failed");
                MDF_FREE(q_data.data);
            }

            break;
        }

        case MQTT_EVENT_ERROR:
            MDF_LOGD("MQTT_EVENT_ERROR");
            break;

        default:
            MDF_LOGD("Other event id:%d", event->event_id);
            break;
    }

    return ESP_OK;
}

mdf_err_t mesh_mqtt_start(char *url)
{
    if (g_mqtt_start_flag) {
        return MDF_FAIL;
    }

    g_mqtt_start_flag = true;
    MDF_PARAM_CHECK(url);

    /**
     * Connect semaphore
     */
    g_connect_handle = xSemaphoreCreateBinary();

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = url,
        .event_handle = mqtt_event_handler,
        // .client_cert_pem = (const char *)client_cert_pem_start,
        // .client_key_pem = (const char *)client_key_pem_start,
    };

    g_queue_handle = xQueueCreate(3, sizeof(queue_data_t));
    g_client = esp_mqtt_client_init(&mqtt_cfg);
    MDF_ERROR_ASSERT(esp_mqtt_client_start(g_client));

    /**
     * Waitting connect complete
     */
    xSemaphoreTake(g_connect_handle, portMAX_DELAY);

    return MDF_OK;
}

mdf_err_t mesh_mqtt_stop()
{
    if (!g_mqtt_start_flag) {
        return MDF_FAIL;
    }

    g_mqtt_start_flag = false;

    queue_data_t q_data = {0x0};

    g_mqtt_connect_flag = false;

    if (uxQueueMessagesWaiting(g_queue_handle)) {
        if (xQueueReceive(g_queue_handle, (void *)&q_data, 0)) {
            MDF_FREE(q_data.data);
        }
    }

    vQueueDelete(g_queue_handle);
    g_queue_handle = NULL;

    vSemaphoreDelete(g_connect_handle);
    g_connect_handle = NULL;

    esp_mqtt_client_stop(g_client);
    esp_mqtt_client_destroy(g_client);
    g_client = NULL;

    return MDF_OK;
}
