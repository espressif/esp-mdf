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

#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#define DEVICE_MASTER_NETWORK_CONFIG_DURATION_MS    (60000)
#define DEVICE_RESTART_TIMEOUT_MS                   (3000)
#define DEVICE_STORE_RESTART_COUNT_KEY              "restart_count"

static const char *TAG                  = "mesh_utils";

void restart_count_erase_timercb(void *timer)
{
    if (!xTimerStop(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerStop timer: %p", timer);
    }

    if (!xTimerDelete(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerDelete timer: %p", timer);
    }

    mdf_info_erase(DEVICE_STORE_RESTART_COUNT_KEY);
    MDF_LOGD("Erase restart count");
}

int restart_count_get()
{
    mdf_err_t ret          = MDF_OK;
    TimerHandle_t timer    = NULL;
    uint32_t restart_count = 0;

    /**< If the device restarts within the instruction time,
         the event_mdoe value will be incremented by one */
    mdf_info_load(DEVICE_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    restart_count++;
    ret = mdf_info_save(DEVICE_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Save the number of restarts within the set time");

    timer = xTimerCreate("restart_count_erase", DEVICE_RESTART_TIMEOUT_MS / portTICK_RATE_MS,
                         false, NULL, restart_count_erase_timercb);
    MDF_ERROR_CHECK(!timer, ret, "xTaskCreate, timer: %p", timer);

    xTimerStart(timer, 0);

    return restart_count;
}

bool restart_is_exception()
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

/**
 * @brief Initialize wifi.
 */
mdf_err_t wifi_init()
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

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

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

/**
 * @brief   1.Get Mwifi initialization configuration information and Mwifi AP configuration information
 *      through the blufi or network configuration chain.
 */
mdf_err_t get_network_config(mwifi_init_config_t *init_config, mwifi_config_t *ap_config, uint16_t tid, char *name_prefix)
{
    MDF_PARAM_CHECK(init_config);
    MDF_PARAM_CHECK(ap_config);
    MDF_PARAM_CHECK(tid);
    MDF_PARAM_CHECK(name_prefix);

    mconfig_data_t *mconfig_data        = NULL;
    mconfig_blufi_config_t blufi_config = {
        .company_id = MCOMMON_ESPRESSIF_ID, /**< Espressif Incorporated */
        .tid        = tid,
    };

    /**
     * @brief Network configuration chain slave initialization for obtaining network configuration information from master.
     */
    MDF_ERROR_ASSERT(mconfig_chain_slave_init());

    uint8_t sta_mac[6] = {0};
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    sprintf(blufi_config.name, "%s_%02x%02x", name_prefix, sta_mac[4], sta_mac[5]);
    MDF_LOGI("BLE name: %s", blufi_config.name);

    /**
     * @brief Initialize Bluetooth network configuration
     */
    MDF_ERROR_ASSERT(mconfig_blufi_init(&blufi_config));

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

    memcpy(ap_config, &mconfig_data->config, sizeof(mwifi_config_t));
    memcpy(init_config, &mconfig_data->init_config, sizeof(mwifi_init_config_t));

    /**
     * @brief Save configuration information to nvs flash.
     */
    mdf_info_save(MDF_MWIFI_INIT_CONFIG_KEY, init_config, sizeof(mwifi_init_config_t));
    mdf_info_save(MDF_MWIFI_CONFIG_KEY, ap_config, sizeof(mwifi_config_t));

    /**
     * @brief Switch to network configuration chain master mode to configure the network for other devices(slave), according to the white list.
     */
    if (mconfig_data->whitelist_size > 0) {
        for (int i = 0; i < mconfig_data->whitelist_size / sizeof(mconfig_whitelist_t); ++i) {
            MDF_LOGD("count: %d, data: " MACSTR,
                     i, MAC2STR((uint8_t *)mconfig_data->whitelist_data + i * sizeof(mconfig_whitelist_t)));
        }

        MDF_ERROR_ASSERT(mconfig_chain_master(mconfig_data, DEVICE_MASTER_NETWORK_CONFIG_DURATION_MS / portTICK_RATE_MS));
    }

    MDF_FREE(mconfig_data);

    return MDF_OK;
}
