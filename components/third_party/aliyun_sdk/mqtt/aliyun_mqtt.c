// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "mqtt_client.h"

#include "aliyun_mqtt.h"
#include "aliyun_platform.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_mqtt";

static esp_mqtt_client_handle_t g_mqtt_client = NULL;

static bool g_mqtt_connet_flag = false;
static xTimerHandle g_mqtt_disconnect_timer = NULL;

mdf_err_t aliyun_mqtt_get_connet_status(void)
{
    if (g_mqtt_connet_flag) {
        return MDF_OK;
    } else {
        return MDF_FAIL;
    }
}

void aliyun_mqtt_set_connet_status(bool flag)
{
    MDF_LOGI("aliyun_mqtt_set_connet_status :%d", flag);
    g_mqtt_connet_flag = flag;
}

mdf_err_t aliyun_mqtt_publish(const char *topic, const void *data, size_t len, aliyun_mqtt_qos_t qos)
{
    if (!g_mqtt_connet_flag) {
        return MDF_FAIL;
    }

    esp_err_t ret = esp_mqtt_client_publish(g_mqtt_client, (const char *)topic, (const char *)data, (int)len, (int)qos, false);

    return ret >= ESP_OK ? MDF_OK : MDF_FAIL;
}

int aliyun_mqtt_unsubscribe(const char *topic)
{
    if (!g_mqtt_connet_flag) {
        return MDF_FAIL;
    }

    return esp_mqtt_client_unsubscribe(g_mqtt_client, topic);
}

int aliyun_mqtt_subscribe(const char *topic, aliyun_mqtt_qos_t qos)
{
    if (!g_mqtt_connet_flag) {
        return MDF_FAIL;
    }

    return esp_mqtt_client_subscribe(g_mqtt_client, topic, qos);
}

static void mqtt_disconnected_callback(TimerHandle_t xTimer)
{
    MDF_LOGI("cloud_connect_status: false");
    aliyun_platform_set_cloud_connect_status(false);
}

static mdf_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            MDF_LOGI("MQTT_EVENT_BEFORE_CONNECT:");
            break;

        case MQTT_EVENT_CONNECTED:
            MDF_LOGI("MQTT_EVENT_CONNECTED");

            if (g_mqtt_disconnect_timer && xTimerIsTimerActive(g_mqtt_disconnect_timer)) {
                if (!xTimerStop(g_mqtt_disconnect_timer, 0)) {
                    MDF_LOGE("xTimer operation failed");
                }
            }

            aliyun_mqtt_set_connet_status(true);
            break;

        case MQTT_EVENT_DISCONNECTED:
            MDF_LOGI("MQTT_EVENT_DISCONNECTED");
            aliyun_mqtt_set_connet_status(false);

            if (!g_mqtt_disconnect_timer) {
                aliyun_platform_set_cloud_connect_status(false);
            } else if (!xTimerIsTimerActive(g_mqtt_disconnect_timer)) {
                if (!xTimerStart(g_mqtt_disconnect_timer, 0)) {
                    MDF_LOGE("xTimer operation failed");
                }
            }

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

        case MQTT_EVENT_DATA:
            MDF_LOGD("MQTT_EVENT_DATA(%d,%d,%d)", event->topic_len, event->data_len, event->topic_len + event->data_len);

            if (event->topic_len > CONFIG_ALIYUN_TOPIC_SIZE) {
                MDF_LOGE("Topic length exceeds limit");
                MDF_LOGD("(%.*s(%d):%.*s(%d)", event->topic_len, event->topic, event->topic_len, event->data_len, event->data, event->data_len);
                break;
            }

            if (event->data_len > CONFIG_ALIYUN_PAYLOAD_SIZE) {
                MDF_LOGE("Payload length exceeds limit");
                MDF_LOGD("(%.*s(%d):%.*s(%d)", event->topic_len, event->topic, event->topic_len, event->data_len, event->data, event->data_len);
                break;
            }

            if (strncmp((event->topic + event->topic_len - 5), "reply", 5) == 0) {
                char *topic = MDF_MALLOC(event->topic_len + 1);
                memcpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';

                if (strstr(topic, "service") != NULL) {
                    MDF_LOGD("Ignore topic(%s), drop it", topic);
                    MDF_FREE(topic);
                    break;
                }

                MDF_FREE(topic);
            }

            aliyun_buffer_t *mqtt_buf = MDF_CALLOC(1, sizeof(aliyun_buffer_t) + event->topic_len + event->data_len + 2);
            memcpy(mqtt_buf->data, event->topic, event->topic_len);
            mqtt_buf->topic = (char *)mqtt_buf->data;
            mqtt_buf->topic_len = event->topic_len;
            memcpy(mqtt_buf->data + event->topic_len + 1, event->data, event->data_len);
            mqtt_buf->payload = mqtt_buf->data + event->topic_len + 1;
            mqtt_buf->payload_len = event->data_len;

            if (aliyun_platform_mqtt_write(mqtt_buf, 0) != MDF_OK) {
                MDF_FREE(mqtt_buf);
                MDF_LOGW("MQTT message queue is full, this msg has been dropped");
            }

            break;

        case MQTT_EVENT_ERROR:
            MDF_LOGD("MQTT_EVENT_ERROR");
            break;

        default:
            MDF_LOGI("MQTT_EVENT(%d)", event->event_id);
            break;
    }

    return MDF_OK;
}

mdf_err_t aliyun_mqtt_connect(aliyun_mqtt_config_t *mqtt_config)
{
    MDF_PARAM_CHECK(mqtt_config);

    aliyun_platform_set_cloud_connect_status(false);

    MDF_LOGI("host:%s", mqtt_config->host);
    MDF_LOGI("port:%d", mqtt_config->port);
    MDF_LOGI("client_id:%s", mqtt_config->client_id);
    MDF_LOGI("username:%s", mqtt_config->username);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .host = mqtt_config->host,
        .port = mqtt_config->port,
        .client_id = mqtt_config->client_id,
        .username = mqtt_config->username,
        .password = mqtt_config->password,
        .event_handle = mqtt_event_handler,
        .transport = mqtt_config->transport,
        .cert_pem = mqtt_config->server_cert,
        .keepalive = 60,
        .disable_clean_session = true,
        .disable_auto_reconnect = false,
        .task_stack = CONFIG_ALIYUN_MQTT_TASK_SIZE,
        .buffer_size = CONFIG_ALIYUN_TOPIC_SIZE + CONFIG_ALIYUN_PAYLOAD_SIZE + 8,
        .task_prio = CONFIG_ALIYUN_MQTT_TASK_PRIOTY,
    };

    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    MDF_ERROR_CHECK(g_mqtt_client == NULL, MDF_FAIL, " esp_mqtt_client_init  Error");

    int delay = CONFIG_ALIYUN_DISCONNECT_DELAY_NOTIFY;

    if (delay) {
        g_mqtt_disconnect_timer = xTimerCreate("disconnect_timer", pdMS_TO_TICKS(CONFIG_ALIYUN_DISCONNECT_DELAY_NOTIFY * 1000),
                                               false, NULL, mqtt_disconnected_callback);
        MDF_ERROR_CHECK(g_mqtt_disconnect_timer == NULL, MDF_FAIL, "create disconnect timer failed");
    }

    return esp_mqtt_client_start(g_mqtt_client);
}

mdf_err_t aliyun_mqtt_disconnet(void)
{
    return esp_mqtt_client_destroy(g_mqtt_client);
}
