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

static const char *TAG  = "util";

#define MALIYUN_LINKKIT_RESTART_TIMEOUT_MS      (3000)
#define MALIYUN_LINKKIT_STORE_RESTART_COUNT_KEY "restart_count"

static void restart_count_erase_timercb(void *timer)
{
    if (!xTimerStop(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerStop timer: %p", timer);
    }

    if (!xTimerDelete(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerDelete timer: %p", timer);
    }

    mdf_info_erase(MALIYUN_LINKKIT_STORE_RESTART_COUNT_KEY);
    MDF_LOGD("Erase restart count");
}

int restart_count_get(void)
{
    mdf_err_t ret             = MDF_OK;
    TimerHandle_t timer       = NULL;
    uint32_t restart_count    = 0;
    RESET_REASON reset_reason = rtc_get_reset_reason(0);

    mdf_info_load(MALIYUN_LINKKIT_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));

    if (reset_reason != POWERON_RESET && reset_reason != RTCWDT_RTC_RESET) {
        restart_count = 0;
        MDF_LOGW("restart reason: %d", reset_reason);
    }

    /**< If the device restarts within the instruction time,
         the event_mdoe value will be incremented by one */
    restart_count++;
    ret = mdf_info_save(MALIYUN_LINKKIT_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Save the number of restarts within the set time");

    timer = xTimerCreate("restart_count_erase", MALIYUN_LINKKIT_RESTART_TIMEOUT_MS / portTICK_RATE_MS,
                         false, NULL, restart_count_erase_timercb);
    MDF_ERROR_CHECK(!timer, ret, "xTaskCreate, timer: %p", timer);

    xTimerStart(timer, 0);

    return restart_count;
}

bool restart_is_exception(void)
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
    ret = esp_partition_erase_range(coredump_part, 0, coredump_part->size);
    MDF_ERROR_CHECK(ret, false, "<%s> esp_partition_erase_range fail", mdf_err_to_name(ret));

    return true;
}

/**
 * @brief Timed printing system information
 */
void show_system_info_timercb(void *timer)
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

//#define CONFIG_MALIYUN_MEMORY_DEBUG
#ifdef CONFIG_MALIYUN_MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
#endif /**< CONFIG_MALIYUN_MEMORY_DEBUG */
}

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