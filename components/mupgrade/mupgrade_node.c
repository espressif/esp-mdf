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

#include "mupgrade.h"

#define MUPGRADE_STORE_CONFIG_KEY "mupugrad_config"

static const char *TAG = "mupgrade_node";
static mupgrade_config_t *g_upgrade_config = NULL;

static mdf_err_t mupgrade_status(const mupgrade_status_t *status, size_t size)
{
    mdf_err_t ret               = MDF_OK;
    size_t response_size        = sizeof(mupgrade_status_t);
    mwifi_data_type_t data_type = {
        .upgrade = true
    };

    if (!g_upgrade_config) {
        size_t config_size = sizeof(mupgrade_config_t) + MUPGRADE_PACKET_MAX_NUM / 8;
        g_upgrade_config   = MDF_CALLOC(1, config_size);
        g_upgrade_config->start_time = xTaskGetTickCount();

        mdf_info_load(MUPGRADE_STORE_CONFIG_KEY, g_upgrade_config, &config_size);
    }

    /**< If g_upgrade_config->status has been created and
         once again upgrade the same name bin, just return MDF_OK */
    if (!memcmp(g_upgrade_config->status.name, status->name,
                sizeof(g_upgrade_config->status.name))) {
        goto EXIT;
    }

    memcpy(&g_upgrade_config->status, status, sizeof(mupgrade_status_t));
    memset(&g_upgrade_config->status.progress_array, 0, MUPGRADE_PACKET_MAX_NUM / 8);
    g_upgrade_config->status.written_size = 0;

    if (esp_mesh_get_type() == MESH_ROOT) {
        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        ret = esp_ota_set_boot_partition(update_partition);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "esp_ota_set_boot_partition");

        g_upgrade_config->status.written_size = g_upgrade_config->status.total_size;
        memset(g_upgrade_config->status.progress_array, 0xff, MUPGRADE_PACKET_MAX_NUM / 8);

        mdf_event_loop_send(MDF_EVENT_MUPGRADE_FINISH, NULL);
        MDF_LOGI("MESH_ROOT update finish");
        goto EXIT;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update  = esp_ota_get_next_update_partition(NULL);

    ret = MDF_ERR_MUPGRADE_FIRMWARE_PARTITION;
    MDF_ERROR_GOTO(!running || !update, EXIT,
                   "No partition is found or flash read operation failed");
    MDF_LOGD("Running partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             running->label, running->type, running->subtype, running->address);
    MDF_LOGD("Update partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             update->label, update->type, update->subtype, update->address);

    g_upgrade_config->partition  = update;
    g_upgrade_config->start_time = xTaskGetTickCount();

    /**< In some case, the time-cost of flash erase in esp_ota_begin (called by
         mupgrade_start) can be too long to expire the mdf connection */
    uint32_t assoc_expire = esp_mesh_get_ap_assoc_expire();
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));

    ret = esp_ota_begin(update, g_upgrade_config->status.total_size, &g_upgrade_config->handle);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "esp_ota_begin failed");

    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(assoc_expire));
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "mupgrade_start, ret: %d", ret);

    ret = mdf_info_save(MUPGRADE_STORE_CONFIG_KEY, g_upgrade_config,
                        sizeof(mupgrade_status_t) + MUPGRADE_PACKET_MAX_NUM / 8);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "info_store_save, ret: %d", ret);

    mdf_event_loop_send(MDF_EVENT_MUPGRADE_STARTED, NULL);

EXIT:

    if (g_upgrade_config->status.written_size
            && g_upgrade_config->status.written_size != g_upgrade_config->status.total_size) {
        response_size += MUPGRADE_PACKET_MAX_NUM / 8;
        ESP_LOG_BUFFER_CHAR_LEVEL(TAG, g_upgrade_config->status.progress_array,
                                  MUPGRADE_PACKET_MAX_NUM / 8, ESP_LOG_VERBOSE);
    }

    g_upgrade_config->status.type       = MUPGRADE_TYPE_DATA;
    g_upgrade_config->status.error_code = ret;
    MDF_LOGD("Response mupgrade status, written_size: %d, response_size: %d",
             g_upgrade_config->status.written_size, response_size);
    ret = mwifi_write(NULL, &data_type, &g_upgrade_config->status, response_size, true);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mwifi_write");

    return MDF_OK;
}

static mdf_err_t mupgrade_write(const mupgrade_packet_t *packet, size_t size)
{
    MDF_PARAM_CHECK(packet);
    MDF_PARAM_CHECK(size);

    mdf_err_t ret = MDF_OK;

    if (!g_upgrade_config) {
        size_t config_size = sizeof(mupgrade_config_t) + MUPGRADE_PACKET_MAX_NUM / 8;
        g_upgrade_config   = MDF_CALLOC(1, config_size);
        g_upgrade_config->start_time = xTaskGetTickCount();
        g_upgrade_config->partition  = esp_ota_get_next_update_partition(NULL);

        ret = mdf_info_load(MUPGRADE_STORE_CONFIG_KEY, g_upgrade_config, &config_size);

        if (ret != MDF_OK) {
            MDF_FREE(g_upgrade_config);
            MDF_LOGW("Upgrade configuration is not initialized");
            return MDF_ERR_MUPGRADE_NOT_INIT;
        }
    }

    /**< Received a duplicate packet */
    if (MUPGRADE_GET_BITS(g_upgrade_config->status.progress_array, packet->seq)) {
        MDF_LOGD("Received a duplicate packet, packet_seq: %d", packet->seq);
        return MDF_OK;
    }

    ret = esp_partition_write(g_upgrade_config->partition, packet->seq * MUPGRADE_PACKET_MAX_SIZE,
                              packet->data, packet->size);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_ERR_MUPGRADE_FIRMWARE_DOWNLOAD,
                    "esp_partition_write %s", esp_err_to_name(ret));

    /**< Update g_upgrade_config->status */
    MUPGRADE_SET_BITS(g_upgrade_config->status.progress_array, packet->seq);
    g_upgrade_config->status.written_size += packet->size;

    /**< Save OTA status periodically, it can be used to
         resumable data transfers from breakpoint after system reset */
    static uint32_t s_next_written_percentage = CONFIG_MUPGRADE_STATUS_REPORT_INTERVAL;
    uint32_t written_percentage = g_upgrade_config->status.written_size * 100 / g_upgrade_config->status.total_size;

    MDF_LOGD("packet_seq: %d, packet_size: %d, written_size: %d, progress: %03d%%",
             packet->seq, packet->size, g_upgrade_config->status.written_size, written_percentage);

    if (written_percentage == s_next_written_percentage) {
        MDF_LOGD("Save the data of upgrade status to flash");
        s_next_written_percentage += CONFIG_MUPGRADE_STATUS_REPORT_INTERVAL;

        mdf_info_save(MUPGRADE_STORE_CONFIG_KEY, g_upgrade_config,
                      sizeof(mupgrade_status_t) + MUPGRADE_PACKET_MAX_NUM / 8);
        mdf_event_loop_send(MDF_EVENT_MUPGRADE_STATUS, (void *)written_percentage);
    }

    if (g_upgrade_config->status.written_size == g_upgrade_config->status.total_size) {
        ESP_LOG_BUFFER_CHAR_LEVEL(TAG, g_upgrade_config->status.progress_array,
                                  MUPGRADE_PACKET_MAX_NUM / 8, ESP_LOG_VERBOSE);
        MDF_LOGI("Write total_size: %d, written_size: %d, spend time: %ds",
                 g_upgrade_config->status.total_size, g_upgrade_config->status.written_size,
                 (xTaskGetTickCount() - g_upgrade_config->start_time) * portTICK_RATE_MS / 1000);

        /**< If ESP32 was reset duration OTA, and after restart, the update_handle will be invalid,
             but it still can switch boot partition and reboot successful */
        esp_ota_end(g_upgrade_config->handle);
        mdf_info_erase(MUPGRADE_STORE_CONFIG_KEY);

        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        g_upgrade_config->status.error_code = esp_ota_set_boot_partition(update_partition);
        MDF_ERROR_CHECK(g_upgrade_config->status.error_code != MDF_OK,
                        MDF_ERR_MUPGRADE_FIRMWARE_INVALID, "esp_ota_set_boot_partition");

        mdf_event_loop_send(MDF_EVENT_MUPGRADE_FINISH, NULL);

        mwifi_data_type_t data_type = {.upgrade = true,};
        ret = mwifi_write(NULL, &data_type, &g_upgrade_config->status, sizeof(mupgrade_status_t), true);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Send the status of the upgrade to the root");
    }

    return MDF_OK;
}

mdf_err_t mupgrade_handle(const uint8_t *addr, const void *data, size_t size)
{
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);

    mdf_err_t ret     = MDF_OK;
    uint8_t data_type = ((uint8_t *)data)[0];

    switch (data_type) {
        case MUPGRADE_TYPE_STATUS:
            MDF_LOGV("MUPGRADE_TYPE_STATUS");
            ret = mupgrade_status((mupgrade_status_t *)data, size);
            break;

        case MUPGRADE_TYPE_DATA:
            MDF_LOGV("MUPGRADE_TYPE_DATA");
            ret = mupgrade_write((mupgrade_packet_t *)data, size);
            break;

        default:
            break;
    }

    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mupgrade_handle");
    return MDF_OK;
}

mdf_err_t mupgrade_get_status(mupgrade_status_t *status)
{
    MDF_PARAM_CHECK(status);
    MDF_ERROR_CHECK(!g_upgrade_config, MDF_ERR_NOT_SUPPORTED, "Mupgrade firmware is not initialized");

    memcpy(status, &g_upgrade_config->status, sizeof(mupgrade_status_t));

    return MDF_OK;
}
