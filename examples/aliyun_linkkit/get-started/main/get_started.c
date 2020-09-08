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

#include "aliyun_gateway.h"
#include "aliyun_kv.h"
#include "aliyun_subdevice.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "mdf_common.h"
#include "mwifi.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "cJSON.h"
#include "sys/time.h"
#include "time.h"

#define DEVICE_VERSION CONFIG_DEVICE_VERSION
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PSWD
#define POWER_SWITCH_1 (0)
#define POWER_SWITCH_2 (2)
#define POWER_SWITCH_3 (4)

#define NETWORK_CONNECTED BIT0
#define ALIYUN_ONLINE BIT1
#define ALIYUN_OFFLINE BIT2

static const char company[] = "ESPRESSIF";
static EventGroupHandle_t g_user_event;
static xTimerHandle g_print_timer;
static const char *TAG = "get_started";
static const char identifier_freeheap[] = "FreeHeap";
static const char identifier_periodic[] = "Periodic";

/* thing model */
static mdf_err_t device_post_property(); // Device post property to cloud
static mdf_err_t device_post_event(); // Device post event to cloud
static mdf_err_t device_post_property_cb(uint32_t msg_id, int code, const char *data); // Cloud reply for post property callback
static mdf_err_t device_post_event_cb(uint32_t msg_id, int code, const char *data); // Device post event to cloud
static mdf_err_t device_property_set_cb(uint32_t msg_id, const char *params, size_t request_len); // Cloud set property callback
static mdf_err_t device_service_invoke_cb(uint32_t msg_id, const char *identifier, const char *params, size_t params_len); // Cloud invoke service callback

/* basic communication */
static mdf_err_t device_request_ntp_cb(int64_t utc_ms); // Cloud reply ntp request callback
static mdf_err_t device_update_info_cb(uint32_t msg_id, int code, const char *data); //Cloud reply update deviceinfo callback
static mdf_err_t device_delete_info_cb(uint32_t msg_id, int code, const char *data); //Cloud reply delete deviceinfo callback
static mdf_err_t device_get_config_cb(uint32_t msg_id, int code, const char *data); // Cloud reply get configuration callback
static mdf_err_t device_config_push_cb(uint32_t msg_id, const char *data); // Cloud push configuration callback
static mdf_err_t device_update_delete_info(char flag); //Device update or delete deviceinfo

static void device_print_system_info(xTimerHandle timer);

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static int retry_connect_count = 0;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                MDF_LOGI("wifi sta mode is running");
                MDF_ERROR_ASSERT(esp_wifi_connect());
                break;

            case WIFI_EVENT_STA_STOP:
                MDF_LOGI("wifi sta mode is stoped");
                break;

            case WIFI_EVENT_STA_CONNECTED: {
                MDF_LOGI("wifi is connected");
                retry_connect_count = 0;
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *ctx = (wifi_event_sta_disconnected_t *)event_data;
                MDF_LOGI("wifi is disconnected, reason: %d", ctx->reason);

                if (retry_connect_count++ < 5) {
                    MDF_ERROR_ASSERT(esp_wifi_connect());
                }

                break;
            }

            default:
                MDF_LOGD("WIFI_EVENT (%d)", event_id);
                break;
        }

    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                MDF_LOGI("wifi sta got ip");
                MDF_LOGI("net address: " IPSTR, IP2STR(&(event->ip_info.ip)));
                MDF_LOGI("net mask: " IPSTR, IP2STR(&(event->ip_info.netmask)));
                MDF_LOGI("net gateway: " IPSTR, IP2STR(&(event->ip_info.gw)));
                aliyun_gateway_init();
                aliyun_subdevice_init();
                xEventGroupSetBits(g_user_event, NETWORK_CONNECTED);
                break;
            }

            case IP_EVENT_STA_LOST_IP:
                MDF_LOGI("wifi sta lost ip");
                aliyun_gateway_deinit();
                xEventGroupClearBits(g_user_event, NETWORK_CONNECTED);
                break;

            default:
                MDF_LOGD("IP_EVENT (%d)", event_id);
                break;

        }

    } else if (event_base == ALIYUN_EVENT_BASE) {
        switch (event_id) {
            case ALIYUN_EVENT_ONLINE:
                MDF_LOGI("subdevice is online");
                xEventGroupClearBits(g_user_event, ALIYUN_OFFLINE);
                xEventGroupSetBits(g_user_event, ALIYUN_ONLINE);
                break;

            case ALIYUN_EVENT_OFFLINE:
                MDF_LOGI("subdevice is offline");
                xEventGroupClearBits(g_user_event, ALIYUN_ONLINE);
                xEventGroupSetBits(g_user_event, ALIYUN_OFFLINE);
                break;
        }
    } else {
        MDF_LOGE("Unsupportted event base(%s)", event_base);
    }
}

static mdf_err_t wifi_init()
{
    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    MDF_ERROR_ASSERT(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    MDF_ERROR_ASSERT(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    if (!esp_netif_create_default_wifi_sta()) {
        MDF_LOGE("Create default wifi sta netif failed");
        return MDF_FAIL;
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_ASSERT(esp_wifi_init(&init_cfg));
    wifi_config_t cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        }
    };
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
    MDF_ERROR_ASSERT(esp_wifi_start());
    MDF_LOGI("connect to wifi(%s,%s)", cfg.sta.ssid, cfg.sta.password);

    return MDF_OK;
}

void app_main()
{
    mdf_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    gpio_config_t gpio_cfg = { 0 };
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_INPUT_OUTPUT;
    gpio_cfg.pin_bit_mask = (((1ULL) << POWER_SWITCH_1) | ((1ULL) << POWER_SWITCH_2) | ((1ULL) << POWER_SWITCH_3));
    gpio_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));

    g_user_event = xEventGroupCreate();
    assert(g_user_event != NULL);
    MDF_ERROR_ASSERT(wifi_init());
    xEventGroupWaitBits(g_user_event, NETWORK_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);

    g_print_timer = xTimerCreate("print info", pdMS_TO_TICKS(1000 * 10), true, NULL, device_print_system_info);
    assert(g_print_timer != NULL);
    xTimerStart(g_print_timer, 0);

    ESP_ERROR_CHECK(esp_event_handler_register(ALIYUN_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL));

    /* thing model */
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_PROPERTY_REPLY, device_post_property_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_PROPERTY_SET, device_property_set_cb)); // thing model of setting property callback
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_SERVICE_INVOKE, device_service_invoke_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_EVENT_REPLY, device_post_event_cb));

    /* basic communication */
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_REQUEST_NTP_RESPONSE, device_request_ntp_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY, device_update_info_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY, device_delete_info_cb));
    /* Need enable in cloud before device use remote config function */
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_CONFIG_PUSH, device_config_push_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_GET_CONFIG_REPLY, device_get_config_cb));

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(g_user_event, ALIYUN_ONLINE, pdFAIL, pdFALSE, portMAX_DELAY);

        if ((bits & ALIYUN_ONLINE) != ALIYUN_ONLINE) {
            continue;
        }

        mdf_err_t err = aliyun_subdevice_request_ntp();
        MDF_LOGI("Device request ntp, ret=%d", err);
        ESP_ERROR_CHECK(aliyun_subdevice_inform_version(DEVICE_VERSION));
        ESP_ERROR_CHECK(device_update_delete_info('d'));
        ESP_ERROR_CHECK(device_update_delete_info('u'));
        ESP_ERROR_CHECK(device_post_property());
        ESP_ERROR_CHECK(aliyun_subdevice_get_config());

        int count = 0;

        while (1) {
            bits = xEventGroupWaitBits(g_user_event, ALIYUN_OFFLINE, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 10));

            if ((bits & ALIYUN_OFFLINE) == ALIYUN_OFFLINE) {
                break;
            }

            device_post_event(count++);
        }

    }
}

static mdf_err_t device_post_property()
{
    mdf_err_t ret = MDF_FAIL;
    cJSON *property = cJSON_CreateObject();

    int level = gpio_get_level((gpio_num_t)POWER_SWITCH_1);
    cJSON *obj = cJSON_AddNumberToObject(property, "PowerSwitch_1", level);
    MDF_ERROR_GOTO(obj == NULL, _exit, "Add key:number to json failed");
    level = gpio_get_level((gpio_num_t)POWER_SWITCH_2);
    obj = cJSON_AddNumberToObject(property, "PowerSwitch_2", level);
    MDF_ERROR_GOTO(obj == NULL, _exit, "Add key:number to json failed");
    level = gpio_get_level((gpio_num_t)POWER_SWITCH_3);
    obj = cJSON_AddNumberToObject(property, "PowerSwitch_3", level);
    MDF_ERROR_GOTO(obj == NULL, _exit, "Add key:number to json failed");

    char *json_string = cJSON_PrintUnformatted(property);
    MDF_ERROR_GOTO(json_string == NULL, _exit, "format json failed");
    mdf_err_t err = aliyun_subdevice_post_property(json_string);
    MDF_LOGI("Device post property, ret=%d", err);
    free(json_string);
    ret = MDF_OK;
_exit:
    cJSON_Delete(property);
    return ret;
}

static mdf_err_t device_post_property_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device post property successful");
    } else {
        MDF_LOGW("Device post property failed, ERROR_CODE=%d", code);
    }

    return MDF_OK;
}

static mdf_err_t device_post_event_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device post event successful");
    } else {
        MDF_LOGW("Device post event failed, ERROR_CODE=%d", code);
    }

    return MDF_OK;
}

static mdf_err_t device_property_set_cb(uint32_t msg_id, const char *params, size_t request_len)
{
    MDF_LOGI("aliyun_propery_set_cb: %.*s(%zu)", (int)request_len, params, request_len);

    cJSON *property = cJSON_Parse(params);
    MDF_ERROR_CHECK(property == NULL, MDF_FAIL, "json parse failed");
    cJSON *item = cJSON_GetObjectItem(property, "PowerSwitch_1");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_1, (item->valueint == 0) ? 0 : 1);
    }

    item = cJSON_GetObjectItem(property, "PowerSwitch_2");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_2, (item->valueint == 0) ? 0 : 1);
    }

    item = cJSON_GetObjectItem(property, "PowerSwitch_3");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_3, (item->valueint == 0) ? 0 : 1);
    }

    aliyun_subdevice_reply_property_set(msg_id, 200, NULL);
    cJSON_Delete(property);

    return MDF_OK;
}

static mdf_err_t device_service_invoke_cb(uint32_t msg_id, const char *identifier, const char *params, size_t params_len)
{
    mdf_err_t ret = MDF_FAIL;
    MDF_LOGI("aliyun service invoke cb, identifier=%s, param=%.*s(%zu)", identifier, (int)params_len, params, params_len);
    cJSON *service = cJSON_Parse(params);
    MDF_ERROR_CHECK(service == NULL, MDF_FAIL, "Parse JSON failed");
    cJSON *reply = NULL;

    if (strcmp(identifier, identifier_freeheap) == 0) {
        reply = cJSON_CreateObject();
        MDF_ERROR_GOTO(reply == NULL, _exit, "Create reply json failed");
        cJSON *obj = cJSON_AddNumberToObject(reply, "FreeHeap", esp_get_free_heap_size());
        MDF_ERROR_GOTO(obj == NULL, _exit, "Create key-number failed");
    }

    if (reply != NULL) {
        char *data = cJSON_PrintUnformatted(reply);
        MDF_ERROR_GOTO(data == NULL, _exit, "Format json to string failed");
        aliyun_subdevice_reply_service_invoke(msg_id, 200, identifier, data);
        free(data);
    }

    ret = MDF_OK;
_exit:
    cJSON_Delete(service);
    cJSON_Delete(reply);
    return ret;
}

static mdf_err_t device_request_ntp_cb(int64_t utc_ms)
{
    MDF_LOGI("Request ntp success, utc=%lld", utc_ms);
    return MDF_OK;
}

static mdf_err_t device_update_info_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device update information(tag) successful");
    } else {
        MDF_LOGW("Device update information(tag) failed, ERROR_CODE=%d", code);
    }

    return MDF_OK;
}

static mdf_err_t device_delete_info_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device delete information successful");
    } else {
        MDF_LOGW("Device delete information(tag) failed, ERROR_CODE=%d", code);
    }

    return MDF_OK;
}

static mdf_err_t device_get_config_cb(uint32_t msg_id, int code, const char *data)
{
    MDF_LOGD("Get configuration from cloud");
    printf("%s\n", data);
    return MDF_OK;
}

static mdf_err_t device_config_push_cb(uint32_t msg_id, const char *data)
{
    MDF_LOGD("Recv configuration push from cloud");
    printf("%s\n", data);
    aliyun_subdevice_reply_config_push(msg_id, 200, NULL);
    return MDF_OK;
}

/* flag == u, update deviceinfo, flag != u, delete deviceinfo */
static mdf_err_t device_update_delete_info(char flag)
{
    mdf_err_t ret = MDF_FAIL;
    cJSON *json = cJSON_CreateArray();
    MDF_ERROR_GOTO(json == NULL, _exit, "Create json failed");
    cJSON *item = cJSON_CreateObject();
    MDF_ERROR_GOTO(item == NULL, _exit, "Create item failed");
    cJSON_AddItemToArray(json, item);
    cJSON *obj = cJSON_AddStringToObject(item, "attrKey", "Company");
    MDF_ERROR_GOTO(obj == NULL, _exit, "Add string to json failed");

    if (flag == 'u') {
        obj = cJSON_AddStringToObject(item, "attrValue", company);
        MDF_ERROR_GOTO(obj == NULL, _exit, "Add string to json failed");
    }

    item = cJSON_CreateObject();
    MDF_ERROR_GOTO(item == NULL, _exit, "Create item failed");
    cJSON_AddItemToArray(json, item);
    obj = cJSON_AddStringToObject(item, "attrKey", "Mac");
    MDF_ERROR_GOTO(obj == NULL, _exit, "Add string failed");

    if (flag == 'u') {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char str[20];
        snprintf(str, sizeof(str), MACSTR, MAC2STR(mac));
        obj = cJSON_AddStringToObject(item, "attrValue", str);
        MDF_ERROR_GOTO(obj == NULL, _exit, "Add string to json failed");
    }

    char *payload = cJSON_PrintUnformatted(json);
    MDF_ERROR_GOTO(payload == NULL, _exit, "Format json to string failed");

    if (flag == 'u') {
        mdf_err_t err = aliyun_subdevice_update_deviceinfo(payload);
        MDF_LOGI("Device upate deviceinfo, ret=%d", err);
    } else {
        mdf_err_t err = aliyun_subdevice_delete_deviceinfo(payload);
        MDF_LOGI("Device delete deviceinfo, ret=%d", err);
    }

    MDF_FREE(payload);
    ret = MDF_OK;
_exit:
    cJSON_Delete(json);
    return ret;
}

static mdf_err_t device_post_event(int32_t count)
{
    mdf_err_t ret = MDF_FAIL;
    cJSON *obj = cJSON_CreateObject();
    MDF_ERROR_GOTO(obj == NULL, _exit, "Create json obj failed");
    cJSON *item = cJSON_AddNumberToObject(obj, "count", count);
    MDF_ERROR_GOTO(item == NULL, _exit, "Add key-number failed");
    char *str = cJSON_PrintUnformatted(obj);
    MDF_ERROR_GOTO(str == NULL, _exit, "Format json obj to string failed");
    mdf_err_t err = aliyun_subdevice_post_event(identifier_periodic, str);
    MDF_LOGI("Device post event, ret=%d", err);
    MDF_FREE(str);
_exit:
    cJSON_Delete(obj);
    return ret;
}

static void device_print_system_info(xTimerHandle timer)
{
    static int32_t count = 0;
    static char format_time[32];

    struct timeval tv;
    struct timezone tz;
    int err = gettimeofday(&tv, &tz);
    assert(err == 0);
    struct tm *ts = gmtime(&tv.tv_sec);
    err = strftime(format_time, sizeof(format_time), "%FT%TZ", ts);
    assert(err > 0);

    ESP_LOGI("", "time: %s, free heap: %u, count, %u", format_time, esp_get_free_heap_size(), count);

    if (count % 10 == 9) {
        mdf_mem_print_record();
    }

    if (count % 50 == 0) {
        mdf_mem_print_heap();
    }

    count++;

    if (count < 0) {
        count = 0;
    }
}
