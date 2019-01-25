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

#include "light_driver.h"

#define LIGHT_TID                     (1)
#define LIGHT_RESTART_COUNT_RESET     (3)
#define LIGHT_RESTART_TIMEOUT_MS      (3000)
#define LIGHT_STORE_RESTART_COUNT_KEY "light_count"

#define EVENT_GROUP_TRIGGER_HANDLE     BIT0
#define EVENT_GROUP_TRIGGER_RECV       BIT1

/**
 * @brief The value of the cid corresponding to each attribute of the light
 */
enum light_cid {
    LIGHT_CID_STATUS            = 0,
    LIGHT_CID_HUE               = 1,
    LIGHT_CID_SATURATION        = 2,
    LIGHT_CID_VALUE             = 3,
    LIGHT_CID_COLOR_TEMPERATURE = 4,
    LIGHT_CID_BRIGHTNESS        = 5,
    LIGHT_CID_MODE              = 6,
};

enum light_status {
    LIGHT_STATUS_OFF               = 0,
    LIGHT_STATUS_ON                = 1,
    LIGHT_STATUS_SWITCH            = 2,
    LIGHT_STATUS_HUE               = 3,
    LIGHT_STATUS_BRIGHTNESS        = 4,
    LIGHT_STATUS_COLOR_TEMPERATURE = 5,
};

static const char *TAG                          = "light";
static TaskHandle_t g_root_write_task_handle    = NULL;
static TaskHandle_t g_root_read_task_handle     = NULL;
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
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
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

#ifdef CONFIG_LIGHT_MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
#endif /**< CONFIG_LIGHT_MEMORY_DEBUG */
}

static void restart_count_erase_timercb(void *timer)
{
    if (!xTimerStop(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerStop timer: %p", timer);
    }

    if (!xTimerDelete(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerDelete timer: %p", timer);
    }

    mdf_info_erase(LIGHT_STORE_RESTART_COUNT_KEY);
    MDF_LOGD("Erase restart count");
}

static int restart_count_get()
{
    mdf_err_t ret          = MDF_OK;
    TimerHandle_t timer    = NULL;
    uint32_t restart_count = 0;

    /**< If the device restarts within the instruction time,
         the event_mdoe value will be incremented by one */
    mdf_info_load(LIGHT_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    restart_count++;
    ret = mdf_info_save(LIGHT_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Save the number of restarts within the set time");

    timer = xTimerCreate("restart_count_erase", LIGHT_RESTART_TIMEOUT_MS / portTICK_RATE_MS,
                         false, NULL, restart_count_erase_timercb);
    MDF_ERROR_CHECK(!timer, ret, "xTaskCreate, timer: %p", timer);

    xTimerStart(timer, 0);

    return restart_count;
}

static bool light_restart_is_exception()
{
    mdf_err_t ret                      = ESP_OK;
    ssize_t coredump_len               = 0;
    esp_partition_iterator_t part_itra = NULL;

    part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA,
                                   ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
    MDF_ERROR_CHECK(!part_itra, false, "<%s> esp_partition_find fail", mdf_err_to_name(ret));

    const esp_partition_t *coredump_part = esp_partition_get(part_itra);
    MDF_ERROR_CHECK(!coredump_part, false, "<%s> esp_partition_get fail", mdf_err_to_name(ret));


    ret = esp_partition_read(coredump_part, sizeof(ssize_t), &coredump_len, sizeof(ssize_t));
    MDF_ERROR_CHECK(ret, false, "<%s> esp_partition_read fail", mdf_err_to_name(ret));

    if (coredump_len <= 0) {
        return false;
    }

    /**< erase all coredump partition */
    // ret = esp_partition_erase_range(coredump_part, 0, coredump_part->size);
    // MDF_ERROR_CHECK(ret, false, "<%s> esp_partition_erase_range fail", mdf_err_to_name(ret));

    return true;
}

static mdf_err_t get_network_config(mwifi_init_config_t *init_config, mwifi_config_t *ap_config)
{
    MDF_PARAM_CHECK(init_config);
    MDF_PARAM_CHECK(ap_config);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .company_id = 0x02E5, /**< Espressif Incorporated */
        .tid        = LIGHT_TID,
    };

    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    /**
     * @brief Switch to master mode to configure the network for other devices
     */
    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(blufi_config.name, "light_%02x%02x", sta_mac[4], sta_mac[5]);
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

static mdf_err_t light_show_layer(mlink_handle_data_t *handle_data)
{
    switch (esp_mesh_get_layer()) {
        case 1:
            light_driver_set_rgb(255, 0, 0);   /**< red */
            break;

        case 2:
            light_driver_set_rgb(255, 128, 0); /**< orange */
            break;

        case 3:
            light_driver_set_rgb(255, 255, 0); /**< yellow */
            break;

        case 4:
            light_driver_set_rgb(0, 255, 0);   /**< green */
            break;

        case 5:
            light_driver_set_rgb(0, 255, 255); /**< cyan */
            break;

        case 6:
            light_driver_set_rgb(0, 0, 255);   /**< blue */
            break;

        case 7:
            light_driver_set_rgb(128, 0, 255); /**< purple */
            break;

        default:
            light_driver_set_rgb(255, 255, 255); /**< white */
            break;
    }

    return MDF_OK;
}

static mdf_err_t mlink_set_value(uint16_t cid, void *arg)
{
    int value = *((int *)arg);

    switch (cid) {
        case LIGHT_CID_STATUS:
            switch (value) {
                case LIGHT_STATUS_ON:
                case LIGHT_STATUS_OFF:
                    light_driver_set_switch(value);
                    break;

                case LIGHT_STATUS_SWITCH:
                    light_driver_set_switch(!light_driver_get_switch());
                    break;

                case LIGHT_STATUS_HUE: {
                    uint16_t hue = light_driver_get_hue();
                    hue = (hue + 60) % 360;

                    light_driver_set_saturation(100);
                    light_driver_set_hue(hue);
                    break;
                }

                case LIGHT_STATUS_BRIGHTNESS: {
                    if (light_driver_get_mode() == MODE_HSV) {
                        uint8_t value = (light_driver_get_value() + 20) % 100;
                        light_driver_set_value(value);
                    } else {
                        uint8_t brightness = (light_driver_get_brightness() + 20) % 100;
                        light_driver_set_brightness(brightness);
                    }

                    break;
                }

                case LIGHT_STATUS_COLOR_TEMPERATURE: {
                    uint8_t color_temperature = (light_driver_get_color_temperature() + 20) % 100;

                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_set_color_temperature(color_temperature);

                    break;
                }

                default:
                    break;
            }

            break;

        case LIGHT_CID_MODE:
            switch (value) {
                case MODE_BRIGHTNESS_INCREASE:
                    light_driver_fade_brightness(100);
                    break;

                case MODE_BRIGHTNESS_DECREASE:
                    light_driver_fade_brightness(0);
                    break;

                case MODE_HUE_INCREASE:
                    light_driver_set_saturation(100);
                    light_driver_fade_hue(360);
                    break;

                case MODE_HUE_DECREASE:
                    light_driver_set_saturation(100);
                    light_driver_fade_hue(0);
                    break;

                case MODE_WARM_INCREASE:
                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_fade_warm(100);
                    break;

                case MODE_WARM_DECREASE:
                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_fade_warm(0);
                    break;

                case MODE_NONE:
                    light_driver_fade_stop();
                    break;

                default:
                    break;
            }

            break;

        case LIGHT_CID_HUE:
            light_driver_set_hue(value);
            break;

        case LIGHT_CID_SATURATION:
            light_driver_set_saturation(value);
            break;

        case LIGHT_CID_VALUE:
            light_driver_set_value(value);
            break;

        case LIGHT_CID_COLOR_TEMPERATURE:
            light_driver_set_color_temperature(value);
            break;

        case LIGHT_CID_BRIGHTNESS:
            light_driver_set_brightness(value);
            break;

        default:
            MDF_LOGE("No support cid: %d", cid);
            return MDF_FAIL;
    }

    MDF_LOGD("cid: %d, value: %d", cid, value);

    return MDF_OK;
}

static mdf_err_t mlink_get_value(uint16_t cid, void *arg)
{
    int *value = (int *)arg;

    switch (cid) {
        case LIGHT_CID_STATUS:
            *value = light_driver_get_switch();
            break;

        case LIGHT_CID_HUE:
            *value = light_driver_get_hue();
            break;

        case LIGHT_CID_SATURATION:
            *value = light_driver_get_saturation();
            break;

        case LIGHT_CID_VALUE:
            *value = light_driver_get_value();
            break;

        case LIGHT_CID_COLOR_TEMPERATURE:
            *value = light_driver_get_color_temperature();
            break;

        case LIGHT_CID_BRIGHTNESS:
            *value = light_driver_get_brightness();
            break;

        case LIGHT_CID_MODE:
            *value = light_driver_get_mode();
            break;

        default:
            MDF_LOGE("No support cid: %d", cid);
            return MDF_FAIL;
    }

    MDF_LOGV("cid: %d, value: %d", cid, *value);

    return MDF_OK;
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

void request_handle_task(void *arg)
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
            light_driver_breath_stop();
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (!esp_mesh_is_root()) {
                break;
            }

            ret = mlink_notice_deinit();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_deinit", mdf_err_to_name(ret));

            ret = mlink_httpd_stop();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_stop", mdf_err_to_name(ret));
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

        case MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED:
            light_driver_breath_start(128, 128, 0); /**< yellow blink */
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            light_driver_breath_start(255, 128, 0); /**< orange blink */
            break;

        case MDF_EVENT_MCONFIG_BLUFI_FINISH:
        case MDF_EVENT_MCONFIG_CHAIN_FINISH:
            light_driver_breath_start(0, 255, 0); /**< green blink */
            break;

        case MDF_EVENT_MUPGRADE_STARTED:
            MDF_LOGI("Enter upgrade mode");
            light_driver_breath_start(0, 0, 128); /**< blue blink */
            vTaskDelay(3000 / portTICK_RATE_MS);
            light_driver_breath_stop();
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
            light_driver_breath_start(0, 0, 255); /**< blue blink */
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

            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_TRIGGER_HANDLE);
            break;

        case MDF_EVENT_MESPNOW_RECV:
            if ((int)ctx == MESPNOW_TRANS_PIPE_CONTROL) {
                xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_TRIGGER_RECV);
            }

            break;

        default:
            break;
    }

    return MDF_OK;
}

static void trigger_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;

    if (!g_event_group_trigger) {
        g_event_group_trigger = xEventGroupCreate();
    }

    for (;;) {
        EventBits_t uxBits = xEventGroupWaitBits(g_event_group_trigger,
                             EVENT_GROUP_TRIGGER_RECV | EVENT_GROUP_TRIGGER_HANDLE,
                             pdTRUE, pdFALSE, portMAX_DELAY);

        if (uxBits & EVENT_GROUP_TRIGGER_RECV) {
            uint8_t *data       = NULL;
            uint8_t *addrs_list = NULL;
            size_t addrs_num    = 0;
            size_t size         = 0;
            mwifi_data_type_t data_type = {
                .protocol = MLINK_PROTO_HTTPD,
            };

            mlink_httpd_type_t httpd_type = {
                .format = MLINK_HTTPD_FORMAT_JSON,
                .from   = MLINK_HTTPD_FROM_DEVICE,
                .resp   = false,
            };

            memcpy(&data_type.custom, &httpd_type, sizeof(mlink_httpd_type_t));

            ret = mlink_espnow_read(&addrs_list, &addrs_num, &data, &size, portMAX_DELAY);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_espnow_read", esp_err_to_name(ret));

            MDF_LOGI("Mlink espnow read data: %.*s", size, data);

            for (int i = 0; i < addrs_num; ++i) {
                ret = mwifi_write(addrs_list  + 6 * i, &data_type, data, size, true);
                MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", esp_err_to_name(ret));
            }

            MDF_FREE(data);
            MDF_FREE(addrs_list);
        }

        if (uxBits & EVENT_GROUP_TRIGGER_HANDLE) {
            ret = mlink_trigger_handle(MLINK_COMMUNICATE_MESH);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_trigger_handle", mdf_err_to_name(ret));
        }
    }

    vTaskDelete(NULL);
}

void app_main()
{
    char name[32]                       = {0};
    uint8_t sta_mac[6]                  = {0};
    mwifi_config_t ap_config            = {0x0};
    mwifi_init_config_t init_config     = {0x0};
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
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Continuous power off and restart more than three times to reset the device
     */
    if (restart_count_get() >= LIGHT_RESTART_COUNT_RESET) {
        MDF_LOGW("Erase information saved in flash");
        mdf_info_erase(MDF_SPACE_NAME);
    }

    /**
     * @brief Initialize wifi
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    /**
     * @brief Light driver initialization
     */
    MDF_ERROR_ASSERT(light_driver_init(&driver_config));

    /**
     * @brief Indicate the status of the device by means of a light
     */
    if (mdf_info_load("init_config", &init_config, sizeof(mwifi_init_config_t)) == MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        if (light_restart_is_exception()) {
            light_driver_set_rgb(255, 0, 0); /**< red */
        } else {
            light_driver_set_switch(true);
        }
    } else {
        light_driver_breath_start(255, 255, 0); /**< yellow blink */
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config));
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));
    }

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
    snprintf(name, sizeof(name), "light_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_ERROR_ASSERT(mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_STATUS, "on", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 3, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_HUE, "hue", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 360, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_SATURATION, "saturation", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 100, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_VALUE, "value", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 100, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_COLOR_TEMPERATURE, "color_temperature", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 100, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_BRIGHTNESS, "brightness", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 100, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_MODE, "mode", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RW, 1, 3, 1));
    MDF_ERROR_ASSERT(mlink_add_characteristic_handle(mlink_get_value, mlink_set_value));

    /**
     * @brief Initialize trigger handler
     */
    MDF_ERROR_ASSERT(mlink_trigger_init());
    xTaskCreate(trigger_handle_task, "trigger_handle", 1024 * 3,  NULL, 1, NULL);

    /**
     * @brief Add a request handler
     */
    MDF_ERROR_ASSERT(mlink_set_handle("show_layer", light_show_layer));

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
}
