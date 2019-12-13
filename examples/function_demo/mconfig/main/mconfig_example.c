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

#include "esp_bt.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

static const char *TAG = "mconfig_example";

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

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MDF_EVENT_MWIFI_STARTED");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("MDF_EVENT_PARENT_CONNECTED");
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
            break;

        case MDF_EVENT_MCONFIG_BLUFI_CONNECTED:
            MDF_LOGI("MDF_EVENT_MCONFIG_BLUFI_CONNECTED");
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            MDF_LOGI("MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED");
            break;

        /**< Add a custom communication process */
        case MDF_EVENT_MCONFIG_BLUFI_RECV: {
            mconfig_blufi_data_t *blufi_data = (mconfig_blufi_data_t *)ctx;
            MDF_LOGI("recv data: %.*s", blufi_data->size, blufi_data->data);

            // ret = mconfig_blufi_send(blufi_data->data, blufi_data->size);
            // MDF_ERROR_BREAK(ret != MDF_OK, "<%> mconfig_blufi_send", mdf_err_to_name(ret));
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

mdf_err_t get_network_config(const char *name, mwifi_config_t *mwifi_config, char custom_data[32])
{
    MDF_PARAM_CHECK(name);
    MDF_PARAM_CHECK(mwifi_config);
    MDF_PARAM_CHECK(custom_data);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .tid = 1, /**< Type of device. Used to distinguish different products,
                       APP can display different icons according to this tid. */
        .company_id = MCOMMON_ESPRESSIF_ID, /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    };

    strncpy(blufi_config.name, name, sizeof(blufi_config.name) - 1);
    MDF_LOGI("BLE name: %s", name);

    /**
     * @brief Initialize Bluetooth network configuration
     */
    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));

    /**
     * @brief Network configuration chain slave initialization for obtaining network configuration information from master.
     */
    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    /**
     * @brief Get Network configuration information from blufi or network configuration chain.
     *      When blufi or network configuration chain complete, will send configuration information to config_queue.
     */
    MDF_ERROR_ASSERT(mconfig_queue_read(&mconfig_data, portMAX_DELAY));

    /**
     * @brief Deinitialize Bluetooth network configuration and Network configuration chain.
     */
    MDF_ERROR_ASSERT(mconfig_chain_slave_deinit());
    MDF_ERROR_ASSERT(mconfig_blufi_deinit());

    memcpy(mwifi_config, &mconfig_data->config, sizeof(mwifi_config_t));
    memcpy(custom_data, &mconfig_data->custom, sizeof(mconfig_data->custom));

    /**
     * @brief Switch to network configuration chain master mode to configure the network for other devices(slave), according to the white list.
     */
    if (mconfig_data->whitelist_size > 0) {
        MDF_ERROR_ASSERT(mconfig_chain_master(mconfig_data, pdMS_TO_TICKS(60000)));
    }

    MDF_FREE(mconfig_data);

    return MDF_OK;
}

void app_main()
{
    char name[28]        = {0x0};
    char custom_data[32] = {0x0};
    mwifi_config_t mwifi_config = {0x0};
    mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mconfig_blufi", ESP_LOG_DEBUG);
    esp_log_level_set("mconfig_chain", ESP_LOG_DEBUG);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(name, "ESP-WIFI-MESH_%02x%02x", sta_mac[4], sta_mac[5]);

    /**
     * @note `custom_data` is used for specific application custom data, non-essential fields.
     *       can be passed through the APP distribution network (ESP-WIFI-MESH Config (The bottom
     *       right corner of the configured page) > Custom Data)
     */
    MDF_ERROR_ASSERT(get_network_config(name, &mwifi_config, custom_data));

    MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR ", custom: %.*s",
             mwifi_config.router_ssid, mwifi_config.router_password,
             MAC2STR(mwifi_config.mesh_id), sizeof(custom_data), custom_data);

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    MDF_ERROR_ASSERT(esp_bt_mem_release(ESP_BT_MODE_BLE));

    /**
     * @brief Initialize and start esp-mesh network according to network configuration information.
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&mwifi_config));
    MDF_ERROR_ASSERT(mwifi_start());
}
