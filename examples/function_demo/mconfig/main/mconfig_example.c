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

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {0x0};

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mconfig_blufi", ESP_LOG_DEBUG);
    esp_log_level_set("mconfig_chain", ESP_LOG_DEBUG);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    /**
     * @brief Switch to master mode to configure the network for other devices
     */
    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(blufi_config.name, "MESH_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_LOGI("BLE name: %s", blufi_config.name);
    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));

    MDF_ERROR_ASSERT(mconfig_queue_read(&mconfig_data, portMAX_DELAY));
    MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
             mconfig_data->config.router_ssid, mconfig_data->config.router_password,
             MAC2STR(mconfig_data->config.mesh_id));

    MDF_ERROR_ASSERT(mconfig_chain_slave_deinit());
    MDF_ERROR_ASSERT(mconfig_blufi_deinit());

    /**
     * @brief Note that once BT controller memory is released, the process cannot be reversed.
     *        It means you can not use the bluetooth mode which you have released by this function.
     *        it can release the .bss, .data and other section to heap
     */
    MDF_ERROR_ASSERT(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    /**
     * @brief Enable ESP-MESH network
     */
    MDF_ERROR_ASSERT(mwifi_init(&mconfig_data->init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&mconfig_data->config));
    MDF_ERROR_ASSERT(mwifi_start());

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
}
