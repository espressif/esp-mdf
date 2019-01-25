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

#include "esp_pm.h"

#include "mdf_common.h"
#include "mwifi.h"
#include "mlink.h"
#include "mupgrade.h"
#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#include "button_driver.h"

#define BUTTON_TID                        (13)

#define EVENT_GROUP_BUTTON_KEY_LONG_PUSH  BIT0
#define EVENT_GROUP_BUTTON_KEY_RELEASE    BIT1
#define EVENT_GROUP_BUTTON_KEY_SHORT_PUSH BIT2
#define EVENT_GROUP_BUTTON_MESH_CONNECTED BIT3

#define BUTTON_ESPNOW_CONFIG_STORE_KEY    "espnow_config"
#define BUTTON_MESH_INIT_CONFIG_STORE_KEY "init_config"
#define BUTTON_MESH_AP_CONFIG_STORE_KEY   "ap_config"

/**
 * @brief The value of the cid corresponding to each attribute of the key
 */
enum button_cid {
    BUTTON_CID_KEY0            = 0,
    BUTTON_CID_KEY1            = 1,
    BUTTON_CID_KEY2            = 2,
    BUTTON_CID_KEY3            = 3,
    BUTTON_CID_BATTERY_STATUS  = 4,
    BUTTON_CID_BATTERY_VOLTAGE = 5,
};

static const char *TAG                          = "button";
static EventGroupHandle_t g_event_group_trigger = NULL;

static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

/**
 * @brief Timed printing system information
 */
static void show_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    mesh_assoc_t mesh_assoc         = {0x0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_wifi_vnd_mesh_get(&mesh_assoc);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mesh_assoc.rssi, esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

#ifdef CONFIG_BUTTON_MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();

#endif /**< CONFIG_BUTTON_MEMORY_DEBUG */
}

static mdf_err_t button_get_network_config(mwifi_init_config_t *init_config, mwifi_config_t *ap_config)
{
    MDF_PARAM_CHECK(init_config);
    MDF_PARAM_CHECK(ap_config);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .company_id = 0x02E5, /**< Espressif Incorporated */
        .tid        = BUTTON_TID,
        .only_beacon = true,
    };

    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    /**
     * @brief Switch to master mode to configure the network for other devices
     */
    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(blufi_config.name, "button%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_LOGI("BLE name: %s", blufi_config.name);

    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));
    MDF_ERROR_ASSERT(mconfig_queue_read(&mconfig_data, portMAX_DELAY));
    MDF_ERROR_ASSERT(mconfig_chain_slave_deinit());
    MDF_ERROR_ASSERT(mconfig_blufi_deinit());

    memcpy(ap_config, &mconfig_data->config, sizeof(mwifi_config_t));
    memcpy(init_config, &mconfig_data->init_config, sizeof(mwifi_init_config_t));

    ap_config->mesh_type = MESH_LEAF;

    mdf_info_save(BUTTON_MESH_INIT_CONFIG_STORE_KEY, init_config, sizeof(mwifi_init_config_t));
    mdf_info_save(BUTTON_MESH_AP_CONFIG_STORE_KEY, ap_config, sizeof(mwifi_config_t));


    MDF_FREE(mconfig_data);

    return MDF_OK;
}

static void button_led_show_key_release()
{
    if (!button_usb_is_connected() && !mwifi_is_started()) {
        mlink_espnow_config_t espnow_config = {0x0};

        if (mdf_info_load(BUTTON_ESPNOW_CONFIG_STORE_KEY, &espnow_config,
                          sizeof(mlink_espnow_config_t)) != MDF_OK) {
            button_driver_led_blink_start(128, 128, 0, 100); /**< yellow blink */
            vTaskDelay(300 / portTICK_RATE_MS);
        } else if (!mlink_trigger_is_exist()) {
            button_driver_led_set_rgb(128, 128, 0);          /**< yellow */
            vTaskDelay(300 / portTICK_RATE_MS);
        } else {
            button_driver_led_set_rgb(0, 0, 0);              /**< off */
        }
    } else {
        if (!mwifi_is_connected()) {
            button_driver_led_blink_start(128, 128, 0, 2000); /**< yellow blink */
        } else if (!mlink_trigger_is_exist()) {
            button_driver_led_set_rgb(128, 128, 0);           /**< yellow */
        } else if (!button_battery_get_status()) {
            button_driver_led_blink_start(0, 0, 128, 3000);   /**< blue blink */
        } else {
            button_driver_led_set_rgb(0, 0, 255);             /**< blue */
        }
    }
}

static void button_led_show_key_push()
{
    mlink_espnow_config_t espnow_config = {0x0};

    if (mdf_info_load(BUTTON_ESPNOW_CONFIG_STORE_KEY, &espnow_config,
                      sizeof(mlink_espnow_config_t)) == MDF_OK
            && mlink_trigger_is_exist()
            && (!mwifi_is_started() || mwifi_is_connected())) {
        button_driver_led_set_rgb(0, 255, 0);   /**< green */
    } else {
        button_driver_led_set_rgb(128, 128, 0); /**< yellow */
    }
}

static mdf_err_t mlink_get_value(uint16_t cid, void *arg)
{
    int *value = (int *)arg;

    switch (cid) {
        case BUTTON_CID_KEY0:
        case BUTTON_CID_KEY1:
        case BUTTON_CID_KEY2:
        case BUTTON_CID_KEY3:
            *value = button_key_get_status(cid);
            break;

        case BUTTON_CID_BATTERY_STATUS:
            *value = button_battery_get_status();
            break;

        case BUTTON_CID_BATTERY_VOLTAGE:
            *value = button_battery_get_electricity();
            break;

        default:
            MDF_LOGE("No support cid: %d", cid);
            return MDF_FAIL;
    }

    MDF_LOGV("cid: %d, value: %d", cid, *value);

    return MDF_OK;
}

static void request_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = 0;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        MDF_FREE(data);
        ret = mwifi_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Receive a packet targeted to self over the mesh network",
                           mdf_err_to_name(ret));

        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %.*s", MAC2STR(src_addr), size, size, data);

        if (data_type.upgrade) {
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));

            continue;
        }

        mlink_httpd_type_t *httpd_type = (mlink_httpd_type_t *)&data_type.custom;

        ret = mlink_handle(src_addr, httpd_type, data, size);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));

        data_type.protocol = MLINK_PROTO_NOTICE;

        if (httpd_type->from == MLINK_HTTPD_FROM_DEVICE) {
            ret = mwifi_write(NULL, &data_type, "status", strlen("status"), true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));
        }
    }

    MDF_FREE(data);
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
    mdf_err_t ret                      = MDF_OK;
    static bool s_button_key_push_flag = false;

    MDF_LOGI("event_loop_cb, event: 0x%x", event);

    switch (event) {
        case MDF_EVENT_MWIFI_PARENT_CONNECTED: {
            MDF_LOGI("Parent is connected on station interface");
            wifi_second_chan_t second           = 0;
            mlink_espnow_config_t espnow_config = {0x0};

            esp_mesh_get_parent_bssid((mesh_addr_t *)&espnow_config.parent_bssid);
            esp_wifi_get_channel(&espnow_config.channel, &second);
            mdf_info_save(BUTTON_ESPNOW_CONFIG_STORE_KEY, &espnow_config, sizeof(mlink_espnow_config_t));

            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_BUTTON_MESH_CONNECTED);
            break;
        }

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        case MDF_EVENT_MUPGRADE_STARTED:
            MDF_LOGI("Enter upgrade mode");
            break;

        case MDF_EVENT_MUPGRADE_STATUS: {
            MDF_LOGI("The upgrade progress is: %d%%", (int)ctx);
            mwifi_data_type_t data_type = {
                .protocol = MLINK_PROTO_NOTICE,
            };
            ret = mwifi_write(NULL, &data_type, "ota_status", strlen("ota_status"), true);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", esp_err_to_name(ret));
            break;
        }

        case MDF_EVENT_MUPGRADE_FINISH:
            MDF_LOGI("Upgrade completed waiting for restart");
            break;

        case MDF_EVENT_MLINK_SYSTEM_RESET:
        case MDF_EVENT_BUTTON_KEY_EXCEPTION:
            MDF_LOGW("Erase information saved in flash and system restart");

            if (event == MDF_EVENT_BUTTON_KEY_EXCEPTION && !mwifi_is_started()) {
                break;
            }

            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_BREAK(ret != 0, "Erase the information");

            button_driver_led_set_rgb(255, 0, 0);          /**< red */
            vTaskDelay(1000 / portTICK_RATE_MS);
            esp_restart();
            break;

        case MDF_EVENT_MLINK_SYSTEM_REBOOT:
            MDF_LOGW("Restart PRO and APP CPUs");
            esp_restart();
            break;

        case MDF_EVENT_BUTTON_CHARGING_COMPLETED:
            MDF_LOGI("Battery charging is complete");
            break;

        case MDF_EVENT_BUTTON_CHARGING_STOPED:
            button_driver_deinit();
            break;

        case MDF_EVENT_BUTTON_KEY_SHORT_PRESS:
        case MDF_EVENT_BUTTON_KEY_LONG_PRESS_RELEASE:
            s_button_key_push_flag = false;
            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_BUTTON_KEY_RELEASE);
            break;

        case MDF_EVENT_BUTTON_KEY_LONG_PRESS_PUSH:
            s_button_key_push_flag = true;
            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_BUTTON_KEY_LONG_PUSH);
            break;

        case MDF_EVENT_BUTTON_KEY_PUSH:
            s_button_key_push_flag = true;
            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_BUTTON_KEY_SHORT_PUSH);
            break;

        case MDF_EVENT_BUTTON_KEY_RELEASE:
            s_button_key_push_flag = false;
            break;

        default:
            break;
    }

    if (s_button_key_push_flag) {
        button_led_show_key_push();
    } else {
        button_led_show_key_release();
    }

    return MDF_OK;
}

static mdf_err_t button_mesh_mode()
{
    mdf_err_t ret                   = MDF_OK;
    char name[32]                   = {0};
    uint8_t sta_mac[6]              = {0};
    mwifi_config_t ap_config        = {0x0};
    mwifi_init_config_t init_config = {0x0};

    /**
     * @brief Indicate the status of the device by means of a button
     */
    if (mdf_info_load(BUTTON_MESH_INIT_CONFIG_STORE_KEY, &init_config, sizeof(mwifi_init_config_t)) != MDF_OK
            || mdf_info_load(BUTTON_MESH_AP_CONFIG_STORE_KEY, &ap_config, sizeof(mwifi_config_t)) != MDF_OK) {
        MDF_ERROR_ASSERT(button_get_network_config(&init_config, &ap_config));
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));
    }

    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    /**
     * @brief Configure MLink (LAN communication module)
     */
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    snprintf(name, sizeof(name), "button_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_ERROR_ASSERT(mlink_add_device(BUTTON_TID, name, CONFIG_BUTTON_VERSION));

    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_KEY0, "key_0", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 1, 5));
    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_KEY1, "key_1", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 1, 5));
    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_KEY2, "key_2", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 1, 5));
    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_KEY3, "key_3", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 1, 5));
    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_BATTERY_STATUS, "battery_status", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_READ, 0, 1, 100));
    MDF_ERROR_ASSERT(mlink_add_characteristic(BUTTON_CID_BATTERY_VOLTAGE, "battery_voltage", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_READ, 0, 1, 100));
    MDF_ERROR_ASSERT(mlink_add_characteristic_handle(mlink_get_value, NULL));

    /**
     * @brief Initialize esp-mesh
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Data transfer between wifi mesh devices
     */
    xTaskCreate(request_handle_task, "request_handle", 8 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    TimerHandle_t timer = xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);

    while (xEventGroupWaitBits(g_event_group_trigger,
                               EVENT_GROUP_BUTTON_KEY_LONG_PUSH | EVENT_GROUP_BUTTON_KEY_RELEASE,
                               pdTRUE, pdFALSE, portMAX_DELAY)) {
        ret = mlink_trigger_handle(MLINK_COMMUNICATE_MESH);
        button_key_reset_status();
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Data transmission failed", mdf_err_to_name(ret));
    }

    return MDF_OK;
}

static mdf_err_t button_espnow_mode()
{
    mdf_err_t ret                       = MDF_OK;
    mlink_espnow_config_t espnow_config = {0};

    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    ret = mdf_info_load(BUTTON_ESPNOW_CONFIG_STORE_KEY, &espnow_config, sizeof(mlink_espnow_config_t));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Device network event is not configured");

    ret = mlink_trigger_is_exist();
    MDF_ERROR_CHECK(ret != true, MDF_FAIL, "Device association event is not configured");

    ret = mlink_espnow_init(&espnow_config);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_espnow_init");

    MDF_ERROR_ASSERT(mlink_add_characteristic_handle(mlink_get_value, NULL));

    for (int wait_ms = 100;; wait_ms = 5000) {
        EventBits_t uxBits = xEventGroupWaitBits(g_event_group_trigger,
                             EVENT_GROUP_BUTTON_KEY_LONG_PUSH | EVENT_GROUP_BUTTON_KEY_RELEASE | EVENT_GROUP_BUTTON_KEY_SHORT_PUSH,
                             pdTRUE, pdFALSE, wait_ms / portTICK_RATE_MS);
        MDF_ERROR_CHECK(!uxBits, MDF_ERR_TIMEOUT, "xEventGroupWaitBits");

        if (uxBits & EVENT_GROUP_BUTTON_KEY_SHORT_PUSH) {
            continue;
        }

        if (mlink_trigger_handle(MLINK_COMMUNICATE_ESPNOW) != MDF_OK) {
            MDF_LOGW("Data transmission failed");
            button_key_reset_status();

            mwifi_config_t ap_config        = {0x0};
            mwifi_init_config_t init_config = {0x0};

            /**
             * @brief Initialize esp-mesh
             */
            if (mdf_info_load(BUTTON_MESH_INIT_CONFIG_STORE_KEY, &init_config, sizeof(mwifi_init_config_t)) == MDF_OK
                    && mdf_info_load(BUTTON_MESH_AP_CONFIG_STORE_KEY, &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
                ap_config.channel = 0;

                MDF_ERROR_ASSERT(mwifi_init(&init_config));
                MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
                MDF_ERROR_ASSERT(mwifi_start());
                EventBits_t uxBits = xEventGroupWaitBits(g_event_group_trigger,
                                     EVENT_GROUP_BUTTON_MESH_CONNECTED,
                                     pdTRUE, pdFALSE, 10000 / portTICK_RATE_MS);
                MDF_ERROR_CHECK(!uxBits, MDF_ERR_TIMEOUT, "xEventGroupWaitBits");
            }
        }

        if (uxBits & EVENT_GROUP_BUTTON_KEY_RELEASE) {
            break;
        }
    }

    return MDF_OK;
}

void app_main()
{
    g_event_group_trigger = xEventGroupCreate();

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("gpio", ESP_LOG_WARN);

    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));

    /**
     * @brief Initialize wifi
     */
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    mlink_trigger_init();

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    /**
     * @brief Button driver initialization
     */
    MDF_ERROR_ASSERT(button_driver_init());

    if (button_usb_is_connected()) {
        /**
         * @brief Support distribution network, binding, upgrade and control in ESP-mesh mode
         */
        button_mesh_mode();
    } else {
        /**
         * @brief Only send control commands in ESP-NOW mode
         */
        button_espnow_mode();
    }

    button_led_show_key_release();
    button_driver_deinit();
}
