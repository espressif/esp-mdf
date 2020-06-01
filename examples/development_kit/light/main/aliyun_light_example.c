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

#include "esp_http_client.h"

#include "mdf_common.h"
#include "mdf_event_loop.h"
#include "mwifi.h"
#include "mesh_utils.h"

#include "light_driver.h"
#include "light_handle.h"

#include "aliyun_gateway.h"
#include "aliyun_subdevice.h"
#include "aliyun_kv.h"

#define LIGHT_TID                     (1)
#define LIGHT_NAME                    "light"
#define LIGHT_RESTART_COUNT_RESET     (3)
#define LINKKIT_LIGHT_MODE_COLOR 1
#define LINKKIT_LIGHT_TYPE_RGBCW 5

static const char *TAG = "maliyun_example";
static bool g_need_enrollee = false;
esp_netif_t *sta_netif;

static mdf_err_t maliyun_linkkit_propery_set_cb(const char *params, int request_len)
{
    MDF_LOGD("maliyun_linkkit_propery_set_cb: %.*s", request_len, params);

    cJSON *pJson  = NULL;
    cJSON *pnext  = NULL;
    cJSON *psub   = NULL;

    pJson = cJSON_Parse(params);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightSwitch"))) {
        light_driver_set_switch(pnext->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "Brightness"))) {
        light_driver_set_brightness(pnext->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightMode"))) {
        //
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightType"))) {
        //
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "ColorTemperature"))) {
        light_driver_set_color_temperature((pnext->valueint - 2000) / 50);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "HSVColor"))) {
        psub = cJSON_GetObjectItem(pnext, "Hue");
        light_driver_set_hue(psub->valueint);
        psub = cJSON_GetObjectItem(pnext, "Saturation");
        light_driver_set_saturation(psub->valueint);
        psub = cJSON_GetObjectItem(pnext, "Value");
        light_driver_set_value(psub->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "RGBColor"))) {
        psub = cJSON_GetObjectItem(pnext, "Red");
        uint8_t Red = psub->valueint;
        psub = cJSON_GetObjectItem(pnext, "Green");
        uint8_t Green = psub->valueint;
        psub = cJSON_GetObjectItem(pnext, "Blue");
        uint8_t Blue = psub->valueint;
        light_driver_set_rgb(Red, Green, Blue);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "DeviceArray"))) {
        char *array_data = cJSON_PrintUnformatted(pnext);
        mdf_info_save("DeviceArray", &array_data, strlen(array_data));
        free(array_data);
    } else {
        MDF_LOGI("propery set Key not found");
    }

    aliyun_subdevice_post_property(params);

    cJSON_Delete(pJson);
    return MDF_OK;
}

mdf_err_t service_property_set_cb(uint32_t msg_id, const char *params, size_t params_len)
{
    MDF_LOGI("aliyun_subdevice_property_set_cb_t");
    maliyun_linkkit_propery_set_cb(params, strlen(params));
    return MDF_OK;
}

mdf_err_t config_push_reply_cb(uint32_t msg_id, int code, const char *data)
{
    MDF_LOGI("aliyun_subdevice_thing_config_push_reply_cb_t:%u", msg_id);
    MDF_LOGI("data:%s", data);
    aliyun_subdevice_reply_config_push(msg_id, code, data);
    return MDF_OK;
}

static mdf_err_t light_post_property_cloud(void)
{
    mdf_err_t ret = MDF_OK;
    char *info_data = NULL;

    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    char str_sta_mac[18]      = {0};
    char str_parent_bssid[18] = {0};
    char str_device_array[100] = {0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_get_channel(&primary, &second);
    esp_mesh_get_parent_bssid(&parent_bssid);

    sprintf(str_sta_mac, MACSTR, MAC2STR(sta_mac));
    sprintf(str_parent_bssid, MACSTR, MAC2STR(parent_bssid.addr));

    cJSON *pJson = cJSON_CreateObject();

    MDF_ERROR_GOTO(pJson == NULL, JSON_EXIT, "cJSON_CreateObject pJson Error");

    cJSON_AddStringToObject(pJson, "WIFI_Band", "2.4G");
    cJSON_AddNumberToObject(pJson, "WiFI_RSSI", mwifi_get_parent_rssi());

    cJSON_AddStringToObject(pJson, "WIFI_AP_BSSID", str_parent_bssid);
    cJSON_AddStringToObject(pJson, "MACAddress", str_sta_mac);

    cJSON_AddNumberToObject(pJson, "WIFI_Channel", primary);
    cJSON_AddNumberToObject(pJson, "WiFI_SNR", 0);


    cJSON_AddNumberToObject(pJson, "NodeNum", esp_mesh_get_total_node_num());
    cJSON_AddNumberToObject(pJson, "FreeHeap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(pJson, "Layer", esp_mesh_get_layer());

    cJSON_AddNumberToObject(pJson, "LightSwitch", light_driver_get_switch());
    cJSON_AddNumberToObject(pJson, "Brightness", light_driver_get_brightness());
    cJSON_AddNumberToObject(pJson, "ColorTemperature", light_driver_get_color_temperature() * 50 + 2000);
    cJSON_AddNumberToObject(pJson, "LightMode", LINKKIT_LIGHT_MODE_COLOR);
    cJSON_AddNumberToObject(pJson, "LightType", LINKKIT_LIGHT_TYPE_RGBCW);

    if (mdf_info_load("DeviceArray", &str_device_array, 100) == MDF_OK) {
        cJSON_AddRawToObject(pJson,  "DeviceArray", str_device_array);
    } else {
        cJSON_AddRawToObject(pJson,  "DeviceArray", "[\"fenzu\"]");
    }

    cJSON      *item =  cJSON_CreateObject();
    MDF_ERROR_GOTO(item == NULL, JSON_EXIT, "cJSON_CreateObject item Error");

    cJSON_AddNumberToObject(item, "Hue", light_driver_get_hue());
    cJSON_AddNumberToObject(item, "Saturation", light_driver_get_saturation());
    cJSON_AddNumberToObject(item, "Value", light_driver_get_value());

    cJSON_AddItemToObject(pJson, "HSVColor", item);

    info_data = cJSON_PrintUnformatted(pJson);
    MDF_ERROR_GOTO(info_data == NULL, JSON_EXIT, "cJSON_PrintUnformatted Error");

    ret = aliyun_subdevice_post_property(info_data);
    MDF_LOGI("maliyun_linkkit_report buf:%s", info_data);

    free(info_data);
    cJSON_Delete(pJson);

    return ret;

JSON_EXIT:

    if (info_data != NULL) {
        free(info_data);
    }

    if (pJson != NULL) {
        cJSON_Delete(pJson);
    }

    return MDF_FAIL;

}

static void aliyun_user_task(void *arg)
{
    MDF_LOGI("aliyun_user_task is running");

    mdf_err_t ret               = MDF_FAIL;
    aliyun_device_meta_t subdevice_meta   = {0};

    aliyun_subdevice_set_callback(ALIYUN_MQTT_PROPERTY_SET, service_property_set_cb);
    aliyun_subdevice_set_callback(ALIYUN_MQTT_CONFIG_PUSH, config_push_reply_cb);

    aliyun_get_meta(&subdevice_meta);

    MDF_LOGI("product_key:%s", subdevice_meta.product_key);
    MDF_LOGI("device_name:%s", subdevice_meta.device_name);

    while (mwifi_is_connected()) {

        while (aliyun_subdevice_add_to_gateway(&subdevice_meta, 20000) != MDF_OK) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        light_driver_set_rgb(0, 0, 255);
        MDF_LOGI("aliyun_subdevice_add_to_gateway ok");

        if (g_need_enrollee == true) {
            ret = aliyun_subdevice_match_enrollee();
            MDF_LOGI("aliyun_subdevice_match_enrollee , ret:%d", ret);

            if (esp_mesh_is_root()) {
                ret = aliyun_subdevice_reset();
                MDF_LOGI("aliyun_subdevice_reset , ret:%d", ret);
            }

            g_need_enrollee = false;
        }

        ret = aliyun_subdevice_inform_version(CONFIG_LIGHT_VERSION);
        MDF_LOGI("aliyun_subdevice_inform_version , ret:%d", ret);

        ret = aliyun_subdevice_get_config();
        MDF_LOGI("aliyun_subdevice_get_config , ret:%d", ret);

        ret = aliyun_subdevice_request_ntp();
        MDF_LOGI("aliyun_subdevice_request_ntp , ret:%d", ret);

        ret = light_post_property_cloud();
        MDF_LOGI("light_post_property_cloud, ret: %d", ret);

        while (! aliyun_subdevice_get_login_status()) {
            vTaskDelay(3000 / portTICK_RATE_MS);
        }

        MDF_LOGI("aliyun subdevice connect error");
    }

    MDF_LOGW("aliyun_user_task is exit");
    vTaskDelete(NULL);
}

void user_task_init()
{
    xTaskCreate(&aliyun_user_task, "aliyun_user_task", 8 * 1024, NULL, 5, NULL);
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    mdf_err_t ret = MDF_OK;

    switch (event) {
        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("MDF_EVENT_PARENT_CONNECTED");
            MDF_LOGI("free heap size:%u", esp_get_free_heap_size());

            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_start(sta_netif);
            }

            user_task_init();
            aliyun_subdevice_init();
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP:
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_GOT_IP");
            aliyun_gateway_init();
            break;

        case MDF_EVENT_MWIFI_ROOT_LOST_IP:
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_LOST_IP");
            aliyun_gateway_deinit();
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            if (esp_mesh_is_root()) {
                ret = aliyun_gateway_refresh_subdevice();
                MDF_ERROR_CHECK(ret != MDF_FAIL, ret, "aliyun_gateway_refresh_subdevice error !!");
            }

            break;

        case MDF_EVENT_MUPGRADE_STARTED:
            MDF_LOGI("Enter upgrade mode");
            light_driver_breath_start(0, 0, 128); /**< blue blink */

            break;

        case MDF_EVENT_MUPGRADE_STATUS: {
            MDF_LOGI("The upgrade progress is: %d%%", (int)ctx);

            if ((int)ctx == 50) {
                aliyun_subdevice_post_ota_progress((int)ctx, NULL);
            }

            break;
        }

        case MDF_EVENT_MUPGRADE_FINISH:
            aliyun_subdevice_post_ota_progress(100, NULL);
            MDF_LOGI("Upgrade completed waiting for restart");
            light_driver_breath_start(0, 0, 255); /**< blue blink */
            break;

        default:
            break;
    }

    return MDF_OK;
}

/**
 * @brief Periodically print system information.
 */
void show_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, connect_status:%d,  channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u, login_status:%d", mwifi_get_root_status(), primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size(), aliyun_subdevice_get_login_status());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

#ifdef CONFIG_LIGHT_MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
#endif /**< CONFIG_LIGHT_MEMORY_DEBUG */
}

void app_main()
{
    mwifi_config_t ap_config        = {0x0};
    mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();

    /**
     * NOTE:
     *  If the module has SPI flash, GPIOs 6-11 are connected to the module’s integrated SPI flash and PSRAM.
     *  If the module has PSRAM, GPIOs 16 and 17 are connected to the module’s integrated PSRAM.
     */
    light_driver_config_t driver_config = {
        .gpio_red        = CONFIG_LIGHT_GPIO_RED,
        .gpio_green      = CONFIG_LIGHT_GPIO_GREEN,
        .gpio_blue       = CONFIG_LIGHT_GPIO_BLUE,
        .gpio_cold       = CONFIG_LIGHT_GPIO_COLD,
        .gpio_warm       = CONFIG_LIGHT_GPIO_WARM,
        .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,
        .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,
    };

    /**
     * @brief Continuous power off and restart more than three times to reset the device
     */
    if (restart_count_get() >= LIGHT_RESTART_COUNT_RESET) {
        MDF_LOGW("Erase information saved in flash");
        mdf_info_erase(MDF_SPACE_NAME);
    }

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));

    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    MDF_ERROR_ASSERT(esp_netif_create_default_wifi_mesh_netifs(&sta_netif, NULL));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    /**
     * @brief Light driver initialization
     */
    MDF_ERROR_ASSERT(light_driver_init(&driver_config));

    /**
     * @brief   1.Get Mwifi initialization configuration information and Mwifi AP configuration information from nvs flash.
     *          2.If there is no network configuration information in the nvs flash,
     *              obtain the network configuration information through the blufi or mconfig chain.
     *          3.Indicate the status of the device by means of a light
     */
    if (mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) != MDF_OK) {
        light_driver_breath_start(255, 255, 0); /**< yellow blink */
        g_need_enrollee = true;
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config, LIGHT_TID, LIGHT_NAME));
        ap_config.mesh_id[0] = ap_config.mesh_id[0] + 1;
        ap_config.mesh_id[1] = ap_config.mesh_id[1] + 2;
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));

        /**
         * @brief Save configuration information to nvs flash.
         *
         * @note `init_config` uses MWIFI_INIT_CONFIG_DEFAULT(), no need to use APP configuration under normal circumstances
         */
        mdf_info_save("ap_config", &ap_config, sizeof(mwifi_config_t));
    } else {
        light_driver_set_switch(true);   /**< turn on */
    }

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    /**
     * @brief Initialize and start esp-mesh network according to network configuration information.
     */

    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    MDF_LOGW("esp_get_free_heap_size: %d",  esp_get_free_heap_size());
    TimerHandle_t timer = xTimerCreate("update_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);
}

