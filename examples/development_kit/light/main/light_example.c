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
#include "mwifi.h"
#include "mlink.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"
#include "mdebug_console.h"
#include "mdebug_log.h"

#include "light_driver.h"
#include "light_handle.h"

#include "mesh_utils.h"

#define LIGHT_TID                     (1)
#define LIGHT_NAME                    "light"
#define LIGHT_RESTART_COUNT_FALLBACK  CONFIG_LIGHT_RESTART_COUNT_FALLBACK
#define LIGHT_RESTART_COUNT_RESET     CONFIG_LIGHT_RESTART_COUNT_RESET

static const char *TAG                       = "light_example";
esp_netif_t *sta_netif;
static TaskHandle_t g_root_write_task_handle = NULL;
static TaskHandle_t g_root_read_task_handle  = NULL;
static bool g_config_from_blufi_flag         = false;

/**
 * @brief Read data from mesh network, forward data to extern IP network by http or udp.
 */
static void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = 0;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t mwifi_type      = {0};

    MDF_LOGI("root_write_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        ret = mwifi_root_read(src_addr, &mwifi_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        if (mwifi_type.upgrade) {
            ret = mupgrade_root_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        MDF_LOGD("Root receive, addr: " MACSTR ", size: %d, data: %.*s",
                 MAC2STR(src_addr), size, size, data);

        switch (mwifi_type.protocol) {
            case MLINK_PROTO_HTTPD: { // use http protocol
                mlink_httpd_t httpd_data  = {
                    .size       = size,
                    .data       = data,
                    .addrs_num  = 1,
                    .addrs_list = src_addr,
                };
                memcpy(&httpd_data.type, &mwifi_type.custom, sizeof(httpd_data.type));

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
                MDF_LOGW("Does not support the protocol: %d", mwifi_type.protocol);
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
    mwifi_data_type_t mwifi_type = {
        .compression = true,
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
    };

    MDF_LOGI("root_read_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        ret = mlink_httpd_read(&httpd_data, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK || !httpd_data, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        MDF_LOGD("Root send, addrs_num: %d, addrs_list: " MACSTR ", size: %d, data: %.*s",
                 httpd_data->addrs_num, MAC2STR(httpd_data->addrs_list),
                 httpd_data->size, httpd_data->size, httpd_data->data);

        mwifi_type.group = httpd_data->group;
        memcpy(&mwifi_type.custom, &httpd_data->type, sizeof(mlink_httpd_type_t));

        ret = mwifi_root_write(httpd_data->addrs_list, httpd_data->addrs_num,
                               &mwifi_type, httpd_data->data, httpd_data->size, true);
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

static void delay_func_timer_cb(void *timer)
{
    char *data = (char *)pvTimerGetTimerID(timer);

    mlink_handle_data_t handle_data = {
        .req_data    = data,
        .req_size    = strlen(data),
        .req_fromat  = MLINK_HTTPD_FORMAT_JSON,
        .resp_data   = NULL,
        .resp_size   = 0,
        .resp_fromat = MLINK_HTTPD_FORMAT_JSON,
    };

    if (mlink_handle_request(&handle_data) != MDF_OK) {
        MDF_LOGW("Call the handler in the request list");
    }

    MDF_FREE(handle_data.resp_data);
    MDF_FREE(data);
    xTimerStop(timer, 0);
    xTimerDelete(timer, 0);
}

static mdf_err_t delay_func_call(char *data, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(data);

    MDF_LOGD("tsf_time: %lld, wait_ticks: %d, delay_data: %s", esp_mesh_get_tsf_time(), wait_ticks, data);
    TimerHandle_t timer = xTimerCreate("delay_func", wait_ticks, false, data, delay_func_timer_cb);
    xTimerStart(timer, 0);

    return MDF_OK;
}

/**
 * @brief Handling data between wifi mesh devices.
 */
void node_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t mwifi_type     = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mlink_httpd_type_t *header_info  = NULL;

    while (true) {
        ret = mwifi_read(src_addr, &mwifi_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> Receive a packet targeted to self over the mesh network",
                       mdf_err_to_name(ret));

        if (mwifi_type.upgrade) { // This mesh package contains upgrade data.
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %.*s", MAC2STR(src_addr), size, size, data);

        /*< Header information for http data */
        header_info = (mlink_httpd_type_t *)&mwifi_type.custom;
        MDF_ERROR_GOTO(header_info->format != MLINK_HTTPD_FORMAT_JSON, FREE_MEM,
                       "The current version only supports the json protocol");

        /**
         * @brief Delayed call to achieve synchronous execution
         */
        char tsf_time_str[16] = {0x0};
        int64_t delay_ticks   = 0;

        if (mlink_json_parse((char *)data, "tsf_time", tsf_time_str) == MDF_OK) {
            int64_t tsf_time_us = 0;
            sscanf(tsf_time_str, "%llu", &tsf_time_us);
            delay_ticks = pdMS_TO_TICKS((tsf_time_us - esp_mesh_get_tsf_time()) / 1000);
            MDF_LOGD("delay_ticks: %lld ms", delay_ticks);
        }

        if (delay_ticks > 0 && delay_ticks < 60 * 1000) {
            char *delay_data = MDF_CALLOC(1, size + 1);
            memcpy(delay_data, data, size);
            delay_func_call(delay_data, delay_ticks);
            goto FREE_MEM;
        }

        /**
         * @brief Processing request commands, generating response data
         *
         * @note  Handling only the body part of http, the header
         *        of http is handled by mlink_httpd
         */
        mlink_handle_data_t handle_data = {
            .req_data    = (char *)data,
            .req_size    = size,
            .req_fromat  = MLINK_HTTPD_FORMAT_JSON,
            .resp_data   = NULL,
            .resp_size   = 0,
            .resp_fromat = MLINK_HTTPD_FORMAT_JSON,
        };
        ret = mlink_handle_request(&handle_data);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mlink_handle", mdf_err_to_name(ret));

        if (handle_data.resp_fromat == MLINK_HTTPD_FORMAT_JSON) {
            mlink_json_pack(&handle_data.resp_data, "status_msg", mdf_err_to_name(ret));
            handle_data.resp_size = mlink_json_pack(&handle_data.resp_data, "status_code", -ret);
        }

        /**
         * @brief If this packet comes from a device on the mesh network,
         *  it will notify the App that the device's status has changed.
         */
        if (header_info->from == MLINK_HTTPD_FROM_DEVICE && mwifi_get_root_status()) {
            mwifi_type.protocol = MLINK_PROTO_NOTICE;
            ret = mwifi_write(NULL, &mwifi_type, "status", strlen("status"), true);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mlink_handle", mdf_err_to_name(ret));
        }

        /**
         * @brief Send the response data to the source device
         */
        if (header_info->resp) {
            uint8_t *dest_addr = (header_info->from == MLINK_HTTPD_FROM_SERVER) ? NULL : src_addr;
            /*< Populate the header information of http */
            header_info->format = handle_data.resp_fromat;
            header_info->from   = MLINK_HTTPD_FROM_DEVICE;

            mwifi_type.protocol = MLINK_PROTO_HTTPD;
            mwifi_type.compression = true;
            ret = mwifi_write(dest_addr, &mwifi_type, handle_data.resp_data, handle_data.resp_size, true);

            if (handle_data.resp_fromat == MLINK_HTTPD_FORMAT_HEX) {
                MDF_LOGI("Node send, size: %d, data: ", handle_data.resp_size);
                ESP_LOG_BUFFER_HEX(TAG, handle_data.resp_data, handle_data.resp_size);
            } else {
                MDF_LOGI("Node send, size: %d, data: %.*s", handle_data.resp_size,
                         handle_data.resp_size, handle_data.resp_data);
            }
        }

        MDF_FREE(handle_data.resp_data);
        MDF_ERROR_GOTO(ret != ESP_OK, FREE_MEM, "<%s> mdf_write", mdf_err_to_name(ret));

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
    mdf_err_t ret       = MDF_OK;
    uint8_t *data       = NULL;
    uint8_t *addrs_list = NULL;
    size_t addrs_num    = 0;
    size_t size         = 0;
    uint32_t type       = 0;

    mwifi_data_type_t mwifi_type = {
        .protocol = MLINK_PROTO_HTTPD,
    };

    mlink_httpd_type_t header_info = {
        .format = MLINK_HTTPD_FORMAT_JSON,
        .from   = MLINK_HTTPD_FROM_DEVICE,
        .resp   = false,
    };

    memcpy(&mwifi_type.custom, &header_info, sizeof(mlink_httpd_type_t));

    while (mlink_espnow_read(&addrs_list, &addrs_num, &data, &size, &type, portMAX_DELAY) == MDF_OK) {
        /*< Send to yourself if the destination address is empty */
        if (MWIFI_ADDR_IS_EMPTY(addrs_list) && addrs_num == 1) {
            esp_wifi_get_mac(ESP_IF_WIFI_STA, addrs_list);
        }

        mwifi_type.group = (type == MLINK_ESPNOW_COMMUNICATE_GROUP) ? true : false;
        MDF_LOGI("Mlink espnow read data: %.*s", size, data);

        for (int i = 0; i < addrs_num; ++i) {
            ret = mwifi_write(addrs_list  + 6 * i, &mwifi_type, data, size, true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        }

        MDF_FREE(data);
        MDF_FREE(addrs_list);
    }

    MDF_LOGW("espnow_to_mwifi_task is exit");
    vTaskDelete(NULL);
}

#ifdef CONFIG_LIGHT_BLE_GATEWAY
mdf_err_t mlink_ble_write(void *data, size_t size)
{
    mdf_err_t ret         = MDF_OK;
    char *header          = NULL;
    char *body            = NULL;
    char **addr_list_json = NULL;
    size_t addr_list_num  = 0;
    mwifi_data_type_t mwifi_type = {
        .protocol = MLINK_PROTO_HTTPD,
    };
    mlink_httpd_type_t header_info = {
        .format = MLINK_HTTPD_FORMAT_JSON,
        .from   = MLINK_HTTPD_FROM_DEVICE,
        .resp   = false,
    };

    memcpy(&mwifi_type.custom, &header_info, sizeof(mlink_httpd_type_t));

    ret = mlink_json_parse(data, "header", &header);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: header");
    ret = mlink_json_parse(data, "body", &body);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: header");

    if (mlink_json_parse(header, "group", &addr_list_num) == MDF_OK && addr_list_num > 0) {
        mwifi_type.group = true;
        addr_list_json = MDF_CALLOC(addr_list_num, sizeof(char *));
        ret = mlink_json_parse(header, "group", addr_list_json);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: group");
    } else if (mlink_json_parse(header, "addr", &addr_list_num) == MDF_OK && addr_list_num > 0) {
        addr_list_json = MDF_CALLOC(addr_list_num, sizeof(char *));
        ret = mlink_json_parse(header, "addr", addr_list_json);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: addr");
    } else {
        MDF_LOGW("Data format error");
        ret = MDF_ERR_NOT_SUPPORTED;
        goto EXIT;
    }

    MDF_LOGI("addr_num: %d, headr: %s, body: %s", addr_list_num, header, body);

    for (int i = 0; i < addr_list_num; ++i) {
        uint8_t addr[6] = {0x0};
        mlink_mac_str2hex(addr_list_json[i], addr);
        MDF_FREE(addr_list_json[i]);

        ret = mwifi_write(addr, &mwifi_type, body, strlen(body), true);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> mwifi_write", mdf_err_to_name(ret));
    }

EXIT:

    MDF_FREE(header);
    MDF_FREE(body);
    MDF_FREE(addr_list_json);
    return MDF_OK;
}
#endif /**< CONFIG_LIGHT_BLE_GATEWAY */

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

            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_start(sta_netif);
            }

            light_driver_breath_stop();

#ifdef CONFIG_LIGHT_NETWORKING_TIME_OPTIMIZE_ENABLE

            if (esp_mesh_is_root_fixed()) {
                /**
                 * TODO：Fix the problem that esp_mesh does not update at the bottom,
                 *       IE does not update. This is a temporary solution. This code
                 *       needs to be deleted after esp-idf is fixed.
                 */
                extern mesh_assoc_t g_mesh_ie;
                g_mesh_ie.rc_rssi = mwifi_get_parent_rssi();

                esp_mesh_fix_root(false);
                ESP_ERROR_CHECK(esp_mesh_set_self_organized(true, false));
            }

            mwifi_node_type_t mesh_type = MWIFI_MESH_IDLE;

            if (esp_mesh_is_root()) {
                mesh_type = MWIFI_MESH_ROOT;
            } else {
                mesh_type = MWIFI_MESH_IDLE;
            }

            mwifi_config_t ap_config  = {0x0};
            mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t));

            if (esp_mesh_get_type() != MESH_LEAF && esp_mesh_get_layer() == CONFIG_MWIFI_MAX_LAYER) {
                ESP_ERROR_CHECK(esp_mesh_set_type(MESH_LEAF));
            }

            if (ap_config.mesh_type != mesh_type) {
                ap_config.mesh_type = mesh_type;
                mdf_info_save("ap_config", &ap_config, sizeof(mwifi_config_t));
            }

#endif /**< CONFIG_LIGHT_NETWORKING_TIME_OPTIMIZE_ENABLE */

#ifdef CONFIG_LIGHT_BLE_GATEWAY

            if (!esp_mesh_is_root()) {
                mlink_ble_config_t config = {
                    .company_id = MCOMMON_ESPRESSIF_ID,
                    /**
                     * @brief  This custom_data is for iBeacon definitions. https://developer.apple.com/ibeacon/
                     *         Espressif WeChat official account can be found using WeChat "Yao Yi Yao Zhou Bian"
                     */
                    .custom_data = {
                        0x02, 0x01, 0x06, 0x1A, 0xFF, 0x4C, 0x00, 0x02,
                        0x15, 0xFD, 0xA5, 0x06, 0x93, 0xA4, 0xE2, 0x4F,
                        0xB1, 0xAF, 0xCF, 0xC6, 0xEB, 0x07, 0x64, 0x78,
                        0x25, 0x27, 0xB7, 0xF2, 0x06, 0xC5
                    },
                    .custom_size = 30,
                };

                memcpy(config.name, mlink_device_get_name(), sizeof(config.name) - 1);
                MDF_ERROR_ASSERT(mlink_ble_init(&config));
                mlink_ble_set_cb(NULL, mlink_ble_write);
            } else {
                MDF_ERROR_ASSERT(mlink_ble_deinit());
            }

#endif /**< CONFIG_LIGHT_BLE_GATEWAY */

            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            /** When the root node switches, sometimes no disconnected packets are received */
            ret = mlink_notice_deinit();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_deinit", mdf_err_to_name(ret));

            ret = mlink_httpd_stop();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_stop", mdf_err_to_name(ret));

            if (esp_mesh_is_root()) {
                ret = mwifi_post_root_status(false);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_post_root_status", mdf_err_to_name(ret));
            }

            break;

        case MDF_EVENT_MWIFI_FIND_NETWORK: {
            MDF_LOGI("the root connects to another router with the same SSID");
            mwifi_config_t ap_config  = {0x0};
            wifi_second_chan_t second = 0;

            mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t));
            esp_wifi_get_channel(&ap_config.channel, &second);
            esp_mesh_get_parent_bssid((mesh_addr_t *)ap_config.router_bssid);
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
                xTaskCreate(root_read_task, "root_read", 4 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_read_task_handle);
            }

            ret = mwifi_post_root_status(true);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_post_root_status", mdf_err_to_name(ret));

            break;
        }

        case MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED:
            light_driver_breath_start(128, 128, 0); /**< yellow blink */
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            light_driver_breath_start(255, 128, 0); /**< orange blink */
            break;

        case MDF_EVENT_MCONFIG_BLUFI_FINISH:
            g_config_from_blufi_flag = true;
            __attribute__((fallthrough));

        case MDF_EVENT_MCONFIG_CHAIN_FINISH:
            light_driver_breath_start(0, 255, 0); /**< green blink */
            break;

        case MDF_EVENT_MUPGRADE_STARTED:
            MDF_LOGI("Enter upgrade mode");
            light_driver_breath_start(0, 0, 128); /**< blue blink */
            vTaskDelay(pdMS_TO_TICKS(3000));
            light_driver_breath_stop();
            break;

        case MDF_EVENT_MUPGRADE_STATUS: {
            MDF_LOGI("The upgrade progress is: %d%%", (int)ctx);
            mwifi_data_type_t mwifi_type = {
                .protocol = MLINK_PROTO_NOTICE,
            };
            ret = mwifi_write(NULL, &mwifi_type, "ota_status", strlen("ota_status"), true);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
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
            /**
             * @brief Waiting for adjacent packets to be processed, avoiding loops
             */
            vTaskDelay(pdMS_TO_TICKS(50));

            /**
             * @brief Trigger handler
             */
            ret = mlink_trigger_handle(MLINK_COMMUNICATE_MESH);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_trigger_handle", mdf_err_to_name(ret));

            break;

        case MDF_EVENT_MLINK_BUFFER_FULL: {
            MDF_LOGI("Receive data from sniffer");

            /**
             * @brief Notify the APP to actively request the sniffer data received by the device
             */
            mwifi_data_type_t mwifi_type = {
                .protocol = MLINK_PROTO_NOTICE,
            };
            ret = mwifi_write(NULL, &mwifi_type, "sniffer", strlen("sniffer"), true);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
            break;
        }

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
    mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();
    mdebug_log_config_t log_config = {
        .log_uart_enable = true,
        .log_espnow_enable = true,
    };

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
    if (restart_count_get() >= LIGHT_RESTART_COUNT_FALLBACK) {
        mupgrade_version_fallback();
        MDF_LOGW("Fall back to the previous version");
    } else if (restart_count_get() >= LIGHT_RESTART_COUNT_RESET) {
        MDF_LOGW("Erase information saved in flash");
        mdf_info_erase(MDF_SPACE_NAME);
    }

    /**
     * @brief Light driver initialization
     */
    MDF_ERROR_ASSERT(light_driver_init(&driver_config));
    light_driver_set_switch(true);

    if (mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        if (restart_is_exception()) {
            light_driver_set_rgb(255, 0, 0); /**< red */
        } else {
            light_driver_set_switch(true);   /**< turn on */
        }
    } else {
        light_driver_breath_start(255, 255, 0); /**< yellow blink */
    }

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
     * @brief Add debug function, you can use serial command and wireless debugging.
     *      1. Initialize console module
     */
    MDF_ERROR_ASSERT(mdebug_console_init());
    MDF_ERROR_ASSERT(mdebug_log_set_config(&log_config));
    mdebug_cmd_register_common();

    /**
     * @brief   1.Get Mwifi initialization configuration information and Mwifi AP configuration information from nvs flash.
     *          2.If there is no network configuration information in the nvs flash,
     *              obtain the network configuration information through the blufi or mconfig chain.
     *          3.Indicate the status of the device by means of a light
     */
    if (mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) != MDF_OK) {
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config, LIGHT_TID, LIGHT_NAME));
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));

#ifdef CONFIG_LIGHT_NETWORKING_TIME_OPTIMIZE_ENABLE

        if (g_config_from_blufi_flag) {
            ap_config.mesh_type = MESH_ROOT;
        }

#endif /**< CONFIG_LIGHT_NETWORKING_TIME_OPTIMIZE_ENABLE */

        /**
         * @brief Save configuration information to nvs flash.
         */
        mdf_info_save("ap_config", &ap_config, sizeof(mwifi_config_t));
    }

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
    /**< When using a BLE gateway, you must ensure that the BLE host stack is not released */
#ifndef CONFIG_LIGHT_BLE_GATEWAY
    esp_bt_mem_release(ESP_BT_MODE_BLE);
#endif /**< CONFIG_LIGHT_BLE_GATEWAY */

    /**
     * @brief Configure MLink (LAN communication module)
     *          1.add device
     *          2.add characteristic of device
     *          3.add characteristic handle for get/set value of characteristic.
     */
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    snprintf(name, sizeof(name), LIGHT_NAME "_%02x%02x", sta_mac[4], sta_mac[5]);
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
    MDF_ERROR_ASSERT(mlink_set_handle("get_tsf_time", light_get_tsf_time));

#ifndef CONFIG_LIGHT_BLE_GATEWAY
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
#endif
    /**
     * @brief Initialize and start esp-mesh network according to network configuration information.
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    mwifi_print_config();

    /**
     * @brief Add a default group for the meshkit_button to control all devices
     */
    const uint8_t default_group_id[6] = {LIGHT_TID};
    esp_mesh_set_group_id((mesh_addr_t *)default_group_id, 1);

    /**
     * @brief Handling data between wifi mesh devices.
     */
    xTaskCreate(node_handle_task, "node_handle", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    /**
     * @brief Periodically print system information.
     */
    TimerHandle_t timer = xTimerCreate("show_system_info", pdMS_TO_TICKS(10000),
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);
}
