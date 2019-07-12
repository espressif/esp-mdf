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

#include "mdf_common.h"
#include "mwifi.h"
#include "mlink.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#include "driver/rtc_io.h"
#include "driver/adc.h"

#include "iot_hts221.h"
#include "iot_bh1750.h"
#include "iot_led.h"
#include "iot_button.h"
#include "ulp_sensor.h"

#include "sense_epaper.h"
#include "sense_led.h"
#include "sense_sensors.h"

/**< GPIO number to wake-up button */
#define SENSE_WAKE_UP_BUTTON_IO            GPIO_NUM_34
#define SENSE_WAKE_UP_BUTTON_ACTIVE_LEVEL  (BUTTON_ACTIVE_LOW)

/**< Deep-sleep setting */
#define SENSE_ULP_READ_INTERVAL            CONFIG_SENSE_ULP_READ_INTERVAL
#define SENSE_LUMINANCE_THRESHOLD_LOW      CONFIG_SENSE_LUMINANCE_THRESHOLD_LOW
#define SENSE_LUMINANCE_THRESHOLD_UPPER    CONFIG_SENSE_LUMINANCE_THRESHOLD_UPPER
#ifdef CONFIG_SENSE_ENABLE_TIMER_WAKEUP
#define SENSE_TIMER_WAKE_UP_INTERVAL       CONFIG_SENSE_TIMER_WAKE_UP_INTERVAL
#endif

#define SENSE_EVENT_GROUP_TRIGGER_HANDLE   BIT0
#define SENSE_EVENT_GROUP_TRIGGER_RECV     BIT1

#define SENSE_SENSOR_TID                   22
#define SENSE_KEY_WORK_MODE                "SENSE_MODE"

typedef enum {
    SENSE_MESH_ACTIVE_MODE = 0,
    SENSE_DEEP_SLEEP_MODE,
    SENSE_WORK_MODE_MAX,
} sense_work_mode_t;

enum sensor_status_cid {
    CID_LUM  = 0,
    CID_HUM  = 1,
    CID_TEMP = 2,
};

static const char *TAG                          = "sense_main";
static TaskHandle_t g_root_write_task_handle    = NULL;
static TaskHandle_t g_root_read_task_handle     = NULL;
static EventGroupHandle_t g_event_group_trigger = NULL;

static esp_err_t sense_work_mode_set(sense_work_mode_t work_mode)
{
    sense_work_mode_t mode = work_mode;

    if (mdf_info_save(SENSE_KEY_WORK_MODE, &mode, 1) == ESP_FAIL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

static sense_work_mode_t sense_work_mode_get(void)
{
    sense_work_mode_t mode = SENSE_WORK_MODE_MAX;

    if (mdf_info_load(SENSE_KEY_WORK_MODE, &mode, 1) == ESP_FAIL) {
        mode = SENSE_WORK_MODE_MAX;
    }

    return mode;
}

static void sense_deepsleep_set(void)
{
#ifdef CONFIG_SENSE_ENABLE_ULP_WAKEUP
    bh1750_set_luminance_threshold(SENSE_LUMINANCE_THRESHOLD_LOW, SENSE_LUMINANCE_THRESHOLD_UPPER, 100);

    /**< The i2c io should set to RTC GPIO befor the board entering deepsleep mode. */
    rtc_sensor_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    rtc_sensor_io_init();

    /**< init_ulp_program */
    set_ulp_read_interval(SENSE_ULP_READ_INTERVAL);
    start_ulp_program();
    MDF_LOGI("esp_sleep_enable_ulp_wakeup");
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
#endif /**< CONFIG_SENSE_ENABLE_ULP_WAKEUP */

#ifdef CONFIG_SENSE_ENABLE_TIMER_WAKEUP
    MDF_LOGI("esp_sleep_enable_timer_wakeup, internal: %d ms", SENSE_TIMER_WAKE_UP_INTERVAL);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SENSE_TIMER_WAKE_UP_INTERVAL * 1000));
#endif

#ifdef CONFIG_SENSE_ENABLE_EXT1_WAKEUP
    MDF_LOGI("esp_sleep_enable_ext1_wakeup, GPIO: %d", SENSE_WAKE_UP_BUTTON_IO);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup((uint64_t)(((uint64_t)1) << SENSE_WAKE_UP_BUTTON_IO), SENSE_WAKE_UP_BUTTON_ACTIVE_LEVEL));
#endif

    sensor_deinit();
    sense_state_led_io_hold();
}

static esp_err_t sense_deep_sleep_start()
{
    if (sense_work_mode_get() != SENSE_DEEP_SLEEP_MODE) {
        esp_err_t ret = sense_work_mode_set(SENSE_DEEP_SLEEP_MODE);
        MDF_ERROR_CHECK(ret != ESP_OK, ret, "sense_work_mode_set");
        MDF_LOGI("sense_work_mode_set, mode: %d", SENSE_DEEP_SLEEP_MODE);
    }

#ifdef CONFIG_USE_EPAPER_DISPLAY
    epaper_show_deepsleep();
#endif

    MDF_LOGI("esp deep sleep start");
    sense_deepsleep_set();
    esp_deep_sleep_start();
    return ESP_OK;
}

static void sense_mode_switch_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    do {
        ret = sense_deep_sleep_start();
        MDF_ERROR_BREAK(ret < 0, "sense_deep_sleep_start ret: %d", ret);
    } while (0);

    vTaskDelete(NULL);
}

static void button_tap_cb(void *arg)
{
    xTaskCreate(sense_mode_switch_task, "sense_mode_switch_task", 1024 * 3, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
}

static void button_press_3s_cb(void *arg)
{
    MDF_LOGW("Erase information saved in flash");
    mdf_info_erase(MDF_SPACE_NAME);
    vTaskDelay(500 / portTICK_RATE_MS);
    esp_restart();
}

static void sense_device_init(void)
{
    /**< initialize i2c bus and devices */
    sensor_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    sensor_init();

    /**< initialize status led */
    sense_state_led_init();

#ifdef CONFIG_SENSE_USE_EPAPER_DISPLAY
    epaper_gpio_init();
    epaper_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    epaper_init();
    show_ulp_value();
#endif /**< CONFIG_SENSE_USE_EPAPER_DISPLAY */

}

static void sense_wakeup_handle(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /**< reset to digital GPIO to read sensor's data */
    rtc_reset_gpio_degital();

    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        init_ulp_program();
    }

    switch (cause) {
        case ESP_SLEEP_WAKEUP_ULP:
            MDF_LOGI("ESP_SLEEP_WAKEUP_ULP");
            print_ulp_raw_data();
            break;

        case ESP_SLEEP_WAKEUP_EXT1:
            MDF_LOGI("ESP_SLEEP_WAKEUP_EXT1: %d, Switch to mesh & active mode", SENSE_WAKE_UP_BUTTON_IO);
            sense_work_mode_set(SENSE_MESH_ACTIVE_MODE);
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            MDF_LOGI("ESP_SLEEP_WAKEUP_TIMER");
            break;

        default:
            MDF_LOGI("not a deep sleep reset");
    }
}

static mdf_err_t wifi_init(void)
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
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

static esp_err_t mlink_get_value(uint16_t cid, void *arg)
{
    int *value = (int *)arg;
    int16_t temp_value = 0;
    int16_t hum_value = 0;
    float lum_value = 0;

    switch (cid) {
        case CID_TEMP:
            hts221_get_temperature(&temp_value);
            *value = (int)(temp_value / 10);
            break;

        case CID_HUM:
            hts221_get_humidity(&hum_value);
            *value = (int)(hum_value / 10);
            break;

        case CID_LUM:
            bh1750_get_luminance(&lum_value);
            *value = (int)lum_value;
            break;

        default:
            MDF_LOGE("no support cid: %d", cid);
            return ESP_FAIL;
    }

    MDF_LOGV("cid: %d, value: %d", cid, *value);
    return ESP_OK;
}

static mdf_err_t get_network_config(mwifi_init_config_t *init_config, mwifi_config_t *ap_config)
{
    MDF_PARAM_CHECK(init_config);
    MDF_PARAM_CHECK(ap_config);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .company_id = 0x02E5, /**< Espressif Incorporated */
        .tid        = SENSE_SENSOR_TID,
    };

    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    /**
     * @brief Switch to master mode to configure the network for other devices
     */
    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(blufi_config.name, "sense_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_LOGI("BLE name: %s", blufi_config.name);

    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));
    MDF_ERROR_ASSERT(mconfig_queue_read(&mconfig_data, portMAX_DELAY));
    MDF_ERROR_ASSERT(mconfig_chain_slave_deinit());
    MDF_ERROR_ASSERT(mconfig_blufi_deinit());

    memcpy(ap_config, &mconfig_data->config, sizeof(mwifi_config_t));
    memcpy(init_config, &mconfig_data->init_config, sizeof(mwifi_init_config_t));

    mdf_info_save("init_config", init_config, sizeof(mwifi_init_config_t));
    mdf_info_save("ap_config", ap_config, sizeof(mwifi_config_t));

    /**
     * @brief Switch to master mode to configure the network for other devices
     */
    if (mconfig_data->whitelist_size > 0) {
        for (int i = 0; i < mconfig_data->whitelist_size / sizeof(mconfig_whitelist_t); ++i) {
            MDF_LOGD("count: %d, data: " MACSTR,
                     i, MAC2STR((uint8_t *)mconfig_data->whitelist_data + i * sizeof(mconfig_whitelist_t)));
        }

        MDF_ERROR_ASSERT(mconfig_chain_master(mconfig_data, 60000 / portTICK_RATE_MS));
    }

    MDF_FREE(mconfig_data);

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

#ifdef CONFIG_SENSOR_MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
#endif /**< CONFIG_SENSOR_MEMORY_DEBUG */
}

static void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = 0;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0};

    MDF_LOGI("root_write_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        size = MWIFI_PAYLOAD_LEN * 4;
        MDF_FREE(data);
        ret = mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_root_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            continue;
        }

        MDF_LOGD("Root receive, addr: " MACSTR ", size: %d, data: %.*s",
                 MAC2STR(src_addr), size, size, data);

        switch (data_type.protocol) {
            case MLINK_PROTO_HTTPD: {
                mlink_httpd_t httpd_data  = {
                    .size       = size,
                    .data       = data,
                    .addrs_num  = 1,
                    .addrs_list = src_addr,
                };
                memcpy(&httpd_data.type, &data_type.custom, sizeof(httpd_data.type));

                ret = mlink_httpd_write(&httpd_data, portMAX_DELAY);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));

                break;
            }

            case MLINK_PROTO_NOTICE: {
                ret = mlink_notice_write(data, size, src_addr);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));
                break;
            }

            default:
                MDF_LOGW("Does not support the protocol: %d", data_type.protocol);
                break;
        }
    }

    MDF_LOGW("root_write_task is exit");

    MDF_FREE(data);
    g_root_write_task_handle = NULL;
    vTaskDelete(NULL);
}

static void root_read_task(void *arg)
{
    mdf_err_t ret               = MDF_OK;
    mlink_httpd_t *httpd_data   = NULL;
    mwifi_data_type_t data_type = {
        .compression = true,
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
    };

    MDF_LOGI("root_read_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        if (httpd_data) {
            MDF_FREE(httpd_data->addrs_list);
            MDF_FREE(httpd_data->data);
            MDF_FREE(httpd_data);
        }

        ret = mlink_httpd_read(&httpd_data, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK || !httpd_data, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        MDF_LOGD("Root receive, addrs_num: %d, addrs_list: " MACSTR ", size: %d, data: %.*s",
                 httpd_data->addrs_num, MAC2STR(httpd_data->addrs_list),
                 httpd_data->size, httpd_data->size, httpd_data->data);

        memcpy(&data_type.custom, &httpd_data->type, sizeof(mlink_httpd_type_t));
        ret = mwifi_root_write(httpd_data->addrs_list, httpd_data->addrs_num,
                               &data_type, httpd_data->data, httpd_data->size, true);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_write", mdf_err_to_name(ret));
    }

    MDF_LOGW("root_read_task is exit");

    if (httpd_data) {
        MDF_FREE(httpd_data->addrs_list);
        MDF_FREE(httpd_data->data);
        MDF_FREE(httpd_data);
    }

    g_root_read_task_handle = NULL;
    vTaskDelete(NULL);
}

static void trigger_handle_task(void *arg)
{
    int16_t temp = 0;
    int16_t hum = 0;
    float lum = 0;
    sense_work_mode_t work_mode = sense_work_mode_get();

    do {
        hts221_get_temperature(&temp);
        hts221_get_humidity(&hum);
        bh1750_get_luminance(&lum);

        MDF_LOGI("luminance: %f, humidity: %f, temperature: %f", lum, hum / 10.0, temp / 10.0);

        if (lum < SENSE_LUMINANCE_THRESHOLD_LOW || lum > SENSE_LUMINANCE_THRESHOLD_UPPER) {
            if (mlink_trigger_handle(MLINK_COMMUNICATE_MESH) != MDF_OK) {
                MDF_LOGE("mlink trigger handle error");
            }
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    } while (work_mode == SENSE_MESH_ACTIVE_MODE);

    sense_deep_sleep_start();
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
    MDF_LOGI("event_loop_cb, event: 0x%x", event);
    mdf_err_t ret = MDF_OK;

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_ON);
            xTaskCreate(trigger_handle_task, "trigger_handle", 1024 * 3,  NULL, 1, NULL);
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        case MDF_EVENT_MWIFI_FIND_NETWORK: {
            MDF_LOGI("the root connects to another router with the same SSID");
            mwifi_config_t ap_config  = {0x0};
            wifi_second_chan_t second = 0;

            mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t));
            esp_wifi_get_channel(&ap_config.channel, &second);
            mwifi_set_config(&ap_config);
            mdf_info_save("ap_config", &ap_config, sizeof(mwifi_config_t));
            break;
        }

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE: {
            MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());

            uint8_t sta_mac[MWIFI_ADDR_LEN] = {0x0};
            MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));

            ret = mlink_notice_write("http", strlen("http"), sta_mac);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));
            break;
        }

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address");

            ret = mlink_notice_init();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_init", mdf_err_to_name(ret));

            uint8_t sta_mac[MWIFI_ADDR_LEN] = {0x0};
            MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));

            ret = mlink_notice_write("http", strlen("http"), sta_mac);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));

            ret = mlink_httpd_start();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_start", mdf_err_to_name(ret));

            if (!g_root_write_task_handle) {
                xTaskCreate(root_write_task, "root_write", 4 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_write_task_handle);
            }

            if (!g_root_read_task_handle) {
                xTaskCreate(root_read_task, "root_read", 8 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_read_task_handle);
            }

            break;
        }

        case MDF_EVENT_MWIFI_ROOT_LOST_IP:
            MDF_LOGI("Root loses the IP address");

            ret = mlink_notice_deinit();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_deinit", mdf_err_to_name(ret));

            ret = mlink_httpd_stop();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_stop", mdf_err_to_name(ret));
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED:
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            break;

        case MDF_EVENT_MCONFIG_BLUFI_FINISH:
        case MDF_EVENT_MCONFIG_CHAIN_FINISH:
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
            MDF_LOGW("Erase information saved in flash and system restart");

            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_BREAK(ret != 0, "Erase the information");

            esp_restart();
            break;

        case MDF_EVENT_MLINK_SYSTEM_REBOOT:
            MDF_LOGW("Restart PRO and APP CPUs");
            esp_restart();
            break;

        case MDF_EVENT_MLINK_SET_STATUS:
            if (!g_event_group_trigger) {
                g_event_group_trigger = xEventGroupCreate();
            }

            xEventGroupSetBits(g_event_group_trigger, SENSE_EVENT_GROUP_TRIGGER_HANDLE);
            break;

        case MDF_EVENT_MESPNOW_RECV:
            if ((int)ctx == MESPNOW_TRANS_PIPE_CONTROL) {
                xEventGroupSetBits(g_event_group_trigger, SENSE_EVENT_GROUP_TRIGGER_RECV);
            }

            break;

        default:
            break;
    }

    return MDF_OK;
}

static void request_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        MDF_FREE(data);
        ret = mwifi_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Receive a packet targeted to self over the mesh network",
                           mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));

            continue;
        }

        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %.*s", MAC2STR(src_addr), size, size, data);

        mlink_httpd_type_t *httpd_type = (mlink_httpd_type_t *)&data_type.custom;

        ret = mlink_handle(src_addr, httpd_type, data, size);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));

        if (httpd_type->from == MLINK_HTTPD_FROM_DEVICE) {
            data_type.protocol = MLINK_PROTO_NOTICE;
            ret = mwifi_write(NULL, &data_type, "status", strlen("status"), true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));
        }
    }

    MDF_FREE(data);
    vTaskDelete(NULL);
}

void app_main()
{
    char name[32]                       = {0};
    uint8_t sta_mac[6]                  = {0};
    mwifi_config_t ap_config            = {0x0};
    mwifi_init_config_t init_config     = {0x0};

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    /**
     * @brief Initialize meshkit sense
     */
    sense_wakeup_handle();
    sense_device_init();

    /**
     * @brief press 3 senconds to clear mesh configuration
     */
    button_handle_t btn_handle = iot_button_create(SENSE_WAKE_UP_BUTTON_IO, SENSE_WAKE_UP_BUTTON_ACTIVE_LEVEL);
    iot_button_add_custom_cb(btn_handle, 3, button_press_3s_cb, NULL);

    /**
     * @brief Mesh config
     */
    if (mdf_info_load("init_config", &init_config, sizeof(mwifi_init_config_t)) == MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
    } else {
        sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_SLOW_BLINK);
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config));
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));
    }

    sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_QUICK_BLINK);

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    /**
     * @brief Configure MLink (LAN communication module)
     */
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    snprintf(name, sizeof(name), "sense_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_ERROR_ASSERT(mlink_add_device(SENSE_SENSOR_TID, name, "0.0.1"));
    MDF_ERROR_ASSERT(mlink_add_characteristic(CID_TEMP, "temperature", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 120, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(CID_HUM, "humidity", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 100, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(CID_LUM, "luminance", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RT, 0, 1200, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic_handle(mlink_get_value, NULL));

    /**
     * @brief Initialize esp-mesh
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Initialize event trigger
     */
    mlink_trigger_init();

    /**
     * @brief Data transfer between wifi mesh devices
     */
    xTaskCreate(request_handle_task, "request_handle", 8 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    TimerHandle_t timer = xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);

    if (sense_work_mode_get() == SENSE_MESH_ACTIVE_MODE) {
        /**
        * @Tap button to switch work mode
        */
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_tap_cb, NULL);
    }
}
