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
#include "light_handle.h"

#include "mesh_utils.h"

#define LIGHT_TID                     (1)
#define LIGHT_NAME                    "light"
#define LIGHT_RESTART_COUNT_RESET     (3)

#define EVENT_GROUP_TRIGGER_RECV     BIT0

static const char *TAG                          = "light";
static TaskHandle_t g_root_write_task_handle    = NULL;
static TaskHandle_t g_root_read_task_handle     = NULL;
static EventGroupHandle_t g_event_group_trigger = NULL;

/**
 * @brief Read data from mesh network, forward data to extern IP network by http or udp.
 */
static void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = 0;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0};

    MDF_LOGI("root_write_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        ret = mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_root_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        MDF_LOGD("Root receive, addr: " MACSTR ", size: %d, data: %.*s",
                 MAC2STR(src_addr), size, size, data);

        switch (data_type.protocol) {
            case MLINK_PROTO_HTTPD: { // use http protocol
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

            case MLINK_PROTO_NOTICE: { // use udp protocol
                ret = mlink_notice_write(data, size, src_addr);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));
                break;
            }

            default:
                MDF_LOGW("Does not support the protocol: %d", data_type.protocol);
                break;
        }

FREE_MEM:
        MDF_FREE(data);
    }

    MDF_LOGW("root_write_task is exit");

    MDF_FREE(data);
    g_root_write_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Read data from extern IP network, forward data to destination device.
 */
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
        ret = mlink_httpd_read(&httpd_data, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK || !httpd_data, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        MDF_LOGD("Root receive, addrs_num: %d, addrs_list: " MACSTR ", size: %d, data: %.*s",
                 httpd_data->addrs_num, MAC2STR(httpd_data->addrs_list),
                 httpd_data->size, httpd_data->size, httpd_data->data);

        memcpy(&data_type.custom, &httpd_data->type, sizeof(mlink_httpd_type_t));
        ret = mwifi_root_write(httpd_data->addrs_list, httpd_data->addrs_num,
                               &data_type, httpd_data->data, httpd_data->size, true);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_root_write", mdf_err_to_name(ret));

FREE_MEM:

        if (httpd_data) {
            MDF_FREE(httpd_data->addrs_list);
            MDF_FREE(httpd_data->data);
            MDF_FREE(httpd_data);
        }
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

/**
 * @brief Handling data between wifi mesh devices.
 */
void node_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    while (true) {
        if (!mwifi_is_connected()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        ret = mwifi_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> Receive a packet targeted to self over the mesh network",
                       mdf_err_to_name(ret));

        if (data_type.upgrade) { // This mesh package contains upgrade data.
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %.*s", MAC2STR(src_addr), size, size, data);

        mlink_httpd_type_t *httpd_type = (mlink_httpd_type_t *)&data_type.custom;

        ret = mlink_handle(src_addr, httpd_type, data, size);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mlink_handle", mdf_err_to_name(ret));

        /**
         * @brief If this packet comes from a device on the mesh network, 
         *  it will notify the App that the device's status has changed.
         */
        if (httpd_type->from == MLINK_HTTPD_FROM_DEVICE) {
            data_type.protocol = MLINK_PROTO_NOTICE;
            ret = mwifi_write(NULL, &data_type, "status", strlen("status"), true);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mlink_handle", mdf_err_to_name(ret));
        }

FREE_MEM:
        MDF_FREE(data);
    }

    MDF_FREE(data);
    vTaskDelete(NULL);
}

/**
 * @brief Initialize espnow_to_mwifi_task for forward esp-now data to the wifi mesh network.
 */
static void espnow_to_mwifi_task(void *arg)
{
    mdf_err_t ret = MDF_OK;

    if (!g_event_group_trigger) {
        g_event_group_trigger = xEventGroupCreate();
    }

    while (true) {
        EventBits_t uxBits = xEventGroupWaitBits(g_event_group_trigger,
                             EVENT_GROUP_TRIGGER_RECV,
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
    }

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

            if (esp_mesh_is_root()) {
                uint8_t sta_mac[MWIFI_ADDR_LEN] = {0x0};
                MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));

                ret = mlink_notice_write("http", strlen("http"), sta_mac);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));
            }

            break;
        }

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address");

            /**
             * @brief Initialization mlink notice for inform the mobile phone that there is a mesh root device
             */
            ret = mlink_notice_init();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_init", mdf_err_to_name(ret));

            uint8_t sta_mac[MWIFI_ADDR_LEN] = {0x0};
            MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));

            ret = mlink_notice_write("http", strlen("http"), sta_mac);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));

            /**
             * @brief start mlink http server for handle data between device and moblie phone.
             */
            ret = mlink_httpd_start();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_start", mdf_err_to_name(ret));

            /**
             * @brief start root read/write task for hand data between mesh network and extern ip network.
             */
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

            /**
             * @brief Waiting for adjacent packets to be processed, avoiding loops
             */
            vTaskDelay(50 / portTICK_PERIOD_MS);

            /**
             * @brief Trigger handler
             */
            ret = mlink_trigger_handle(MLINK_COMMUNICATE_MESH);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_trigger_handle", mdf_err_to_name(ret));

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

void app_main()
{
    char name[32]                   = {0};
    uint8_t sta_mac[6]              = {0};
    mwifi_config_t ap_config        = {0x0};
    mwifi_init_config_t init_config = {0x0};

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
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
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
    if (mdf_info_load("init_config", &init_config, sizeof(mwifi_init_config_t)) == MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        if (restart_is_exception()) {
            light_driver_set_rgb(255, 0, 0); /**< red */
        } else {
            light_driver_set_switch(true);
        }
    } else {
        light_driver_breath_start(255, 255, 0); /**< yellow blink */
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config, LIGHT_TID, LIGHT_NAME));
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
     *          1.add device
     *          2.add characteristic of device
     *          3.add characteristic handle for get/set value of characteristic.
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
     *          while characteristic of device reaching conditions， will trigger the corresponding action.
     */
    MDF_ERROR_ASSERT(mlink_trigger_init());

    /**
     * @brief Initialize espnow_to_mwifi_task for forward esp-now data to the wifi mesh network.
     * esp-now data from button or other device.
     */
    xTaskCreate(espnow_to_mwifi_task, "espnow_to_mwifi", 1024 * 3,  NULL, 1, NULL);

    /**
     * @brief Add a request handler, handling request for devices on the LAN.
     */
    MDF_ERROR_ASSERT(mlink_set_handle("show_layer", light_show_layer));

    /**
     * @brief Initialize and start esp-mesh network according to network configuration information.
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Handling data between wifi mesh devices.
     */
    xTaskCreate(node_handle_task, "node_handle", 8 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    /**
     * @brief Periodically print system information.
     */
    TimerHandle_t timer = xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);
}
