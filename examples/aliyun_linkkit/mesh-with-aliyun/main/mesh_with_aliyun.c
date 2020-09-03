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

#include "mdf_common.h"
#include "mupgrade.h"
#include "mwifi.h"

#include "aliyun_gateway.h"
#include "aliyun_kv.h"
#include "aliyun_subdevice.h"

#include "cJSON.h"

#define DEVICE_VERSION CONFIG_DEVICE_VERSION
#define SSID CONFIG_ROUTER_SSID
#define PASSWORD CONFIG_ROUTER_PASSWORD
#define POWER_SWITCH_1 (0)
#define POWER_SWITCH_2 (2)
#define POWER_SWITCH_3 (4)

#define SUBDEVICE_ONLINE_BIT BIT0

static const char *TAG = "aliyu_mesh";

static const char company[] = "ESPRESSIF";
static xTimerHandle g_print_timer;
static const char identifier_freeheap[] = "FreeHeap";
static const char identifier_periodic[] = "Periodic";
static TaskHandle_t g_usertask_handle = NULL;
static esp_netif_t *netif_sta = NULL;
static bool is_added = false;
static EventGroupHandle_t g_events = NULL;

/* thing model */
static mdf_err_t device_post_property(); // Device post property to cloud
static mdf_err_t device_post_event(int32_t count); // Device post event to cloud
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

/* state  */
static mdf_err_t device_add_reply_cb(aliyun_device_reply_t *reply);
static mdf_err_t device_delete_reply_cb(aliyun_device_reply_t *reply);

static void device_print_system_info(xTimerHandle timer);

static mdf_err_t wifi_init()
{
    mdf_err_t ret = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    MDF_ERROR_ASSERT(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));

    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

void user_task(void *args)
{
    MDF_LOGI("user task for aliyun is running");

    g_events = xEventGroupCreate();
    assert(g_events);

    aliyun_subdevice_init();

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

    /* other callback */
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY, device_add_reply_cb));
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY, device_delete_reply_cb));

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(g_events, SUBDEVICE_ONLINE_BIT, false, false, portMAX_DELAY);

        if ((bits & SUBDEVICE_ONLINE_BIT) != SUBDEVICE_ONLINE_BIT) {
            continue;
        }

        mdf_err_t err = aliyun_subdevice_request_ntp();
        MDF_LOGI("Device request ntp, ret=%d", err);
        vTaskDelay(pdMS_TO_TICKS(500));
        err = aliyun_subdevice_inform_version(DEVICE_VERSION);
        MDF_LOGI("Device inform version(%s) ret=%d", DEVICE_VERSION, err);
        vTaskDelay(pdMS_TO_TICKS(500));
        device_update_delete_info('d');
        vTaskDelay(pdMS_TO_TICKS(500));
        device_update_delete_info('u');
        vTaskDelay(pdMS_TO_TICKS(500));
        device_post_property();
        vTaskDelay(pdMS_TO_TICKS(500));
        aliyun_subdevice_get_config();

        int32_t i = 0;

        while ((xEventGroupGetBits(g_events) & SUBDEVICE_ONLINE_BIT) == SUBDEVICE_ONLINE_BIT) {
            vTaskDelay(pdMS_TO_TICKS(60 * 1000));
            device_post_event(i++);
        }
    }

    MDF_LOGW("User task is exiting");
    g_usertask_handle = NULL;
    vTaskDelete(NULL);
}


/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");

            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_start(netif_sta);
            } else {
                aliyun_subdevice_parent_connected();
            }

            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (!esp_mesh_is_root()) {
                aliyun_subdevice_parent_disconnected();
            }

            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP:
            MDF_LOGI("root got ip");
            aliyun_gateway_init();
            break;

        case MDF_EVENT_MWIFI_ROOT_LOST_IP:
            MDF_LOGI("root lost ip");
            aliyun_gateway_deinit();
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            if (esp_mesh_is_root()) {
                mdf_err_t ret = aliyun_gateway_refresh_subdevice();
                MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun gateway refresh subdevide error");
            }

            break;

        case MDF_EVENT_MWIFI_TODS_STATE: {
            mesh_event_info_t *info = ctx;
            MDF_LOGI("toDS=%d", info->toDS_state);
            aliyun_subdevice_update_tods(info->toDS_state);

            break;
        }

        case MDF_EVENT_MUPGRADE_STATUS: {
            MDF_LOGI("The upgrade progress is: %d%%", (int)ctx);

            if ((int)ctx == 50) {
                aliyun_subdevice_post_ota_progress((int)ctx, NULL);
            }

            break;
        }

        case MDF_EVENT_MWIFI_NETWORK_STATE: {
            mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)ctx;
            MDF_LOGI("mesh network is rootless: %d", network_state->is_rootless);
            aliyun_subdevice_set_rootless(network_state->is_rootless);
            break;
        }

        case ALIYUN_EVENT_ONLINE: {
            MDF_LOGW("aliyun subdevice is online.");
            xEventGroupSetBits(g_events, SUBDEVICE_ONLINE_BIT);
            break;
        }

        case ALIYUN_EVENT_OFFLINE: {
            MDF_LOGW("aliyun subdevice is offline.");
            xEventGroupClearBits(g_events, SUBDEVICE_ONLINE_BIT);
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    esp_log_level_set("mwifi", ESP_LOG_INFO);

    gpio_config_t gpio_cfg = { 0 };
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_INPUT_OUTPUT;
    gpio_cfg.pin_bit_mask = (((1ULL) << POWER_SWITCH_1) | ((1ULL) << POWER_SWITCH_2) | ((1ULL) << POWER_SWITCH_3));
    gpio_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));

    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config = {
        .router_ssid = SSID,
        .router_password = PASSWORD,
        .channel = CONFIG_MESH_CHANNEL,
        .mesh_id = CONFIG_MESH_ID,
    };
    /* Initialize wifi mesh. */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());

    xTaskCreate(user_task, "user task", 8 * 1024, NULL, 5, &g_usertask_handle);

    g_print_timer = xTimerCreate("print info", pdMS_TO_TICKS(1000 * 10), true, NULL, device_print_system_info);
    assert(g_print_timer != NULL);
    vTaskDelay(pdMS_TO_TICKS(esp_random() % 40 * 1000));
    xTimerStart(g_print_timer, 0);
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

static mdf_err_t device_add_reply_cb(aliyun_device_reply_t *reply)
{
    MDF_LOGI("this is subdevice add reply callback (%s, %s, %d)", reply->device_name, reply->product_key, reply->status);
    is_added = true;
    return MDF_OK;
}

static mdf_err_t device_delete_reply_cb(aliyun_device_reply_t *reply)
{
    MDF_LOGI("this is subdevice delete reply callback (%s, %s, %d)", reply->device_name, reply->product_key, reply->status);
    return MDF_OK;
}

static void device_print_system_info(xTimerHandle timer)
{
    static struct {
        uint32_t free_heap;
        uint32_t minimum_free_head;
        wifi_second_chan_t second;
        char format_time[32];
        int mesh_layer;
        int node_total_number;
        bool is_online;
        int32_t count;
        bool root_status;
        uint8_t primary;
        mesh_addr_t parent_bssid;
        uint8_t mac[MWIFI_ADDR_LEN];
        uint8_t root_mac[MWIFI_ADDR_LEN];
        int8_t parent_rssi;
    } status;

    struct timeval tv;
    struct timezone tz;
    int err = gettimeofday(&tv, &tz);
    assert(err == 0);
    struct tm *ts = gmtime(&tv.tv_sec);
    err = strftime(status.format_time, sizeof(status.format_time), "%FT%TZ", ts);
    assert(err > 0);

    mesh_assoc_t assoc = { 0 };
    esp_wifi_vnd_mesh_get(&assoc);
    memcpy(status.root_mac, assoc.rc_addr, sizeof(status.root_mac));

    esp_wifi_get_mac(ESP_IF_WIFI_STA, status.mac);
    esp_wifi_get_channel(&status.primary, &status.second);
    esp_mesh_get_parent_bssid(&status.parent_bssid);
    status.free_heap = esp_get_free_heap_size();
    status.minimum_free_head = esp_get_minimum_free_heap_size();
    status.root_status = mwifi_get_root_status();
    status.mesh_layer = esp_mesh_get_layer();
    status.node_total_number = esp_mesh_get_total_node_num();
    status.parent_rssi = mwifi_get_parent_rssi();
    status.is_online = aliyun_subdevice_is_online();

    ESP_LOGI("SYS", "%s, free heap: %u, connect:%d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR ", node num: %d, parent rssi: %d, login_status:%d",
             status.format_time, status.free_heap, status.root_status, status.mesh_layer, MAC2STR(status.mac), MAC2STR(status.parent_bssid.addr),
             status.node_total_number, status.parent_rssi, status.is_online);
    ESP_LOGI("SYS", "root mac: " MACSTR ", Minimum free heap: %u", MAC2STR(status.root_mac), status.minimum_free_head);

    if (status.count % 10 == 9) {
        mdf_mem_print_record();
    }

    if (status.count % 50 == 0) {
        mdf_mem_print_heap();
    }

    status.count++;

    if (status.count < 0) {
        status.count = 0;
    }
}
