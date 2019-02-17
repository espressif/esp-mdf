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
#include "mupgrade.h"
#include "mwifi.h"

typedef struct {
    uint8_t src_addr[MWIFI_ADDR_LEN];
    size_t size;
    uint8_t data[0];
} mupgrade_queue_t;

static const char *TAG = "mupgrade_root";
static mupgrade_config_t *g_upgrade_config = NULL;
static bool g_mupgrade_send_running_flag   = false;
static SemaphoreHandle_t g_mupgrade_send_exit_sem = NULL;

mdf_err_t mupgrade_firmware_init(const char *name, size_t size)
{
    MDF_PARAM_CHECK(name);
    MDF_PARAM_CHECK(size > 0);

    mdf_err_t ret = MDF_OK;
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update  = esp_ota_get_next_update_partition(NULL);

    MDF_ERROR_CHECK(!running || !update, MDF_ERR_MUPGRADE_FIRMWARE_PARTITION,
                    "No partition is found or flash read operation failed");
    MDF_ERROR_CHECK(size != OTA_SIZE_UNKNOWN && size > update->size,
                    MDF_ERR_INVALID_ARG, "The size of the firmware is wrong");

    MDF_LOGI("Running partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             running->label, running->type, running->subtype, running->address);
    MDF_LOGI("Update partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             update->label, update->type, update->subtype, update->address);

    if (!g_upgrade_config) {
        g_upgrade_config = MDF_CALLOC(1, sizeof(mupgrade_config_t));
    }

    g_upgrade_config->start_time          = xTaskGetTickCount();
    g_upgrade_config->queue               = xQueueCreate(3, sizeof(void *));
    g_upgrade_config->partition           = update;
    g_upgrade_config->status.total_size   = size;
    g_upgrade_config->status.written_size = 0;
    memcpy(g_upgrade_config->status.name, name, sizeof(g_upgrade_config->status.name));
    g_upgrade_config->status.error_code   = esp_ota_begin(update, g_upgrade_config->status.total_size,
                                            &g_upgrade_config->handle);
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != MDF_OK, ret, "esp_ota_begin failed");

    return MDF_OK;
}

mdf_err_t mupgrade_firmware_download(const void *data, size_t size)
{
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);
    MDF_ERROR_CHECK(!g_upgrade_config, MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT,
                    "Mupgrade firmware is not initialized");
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != MDF_OK,
                    g_upgrade_config->status.error_code, "mupgrade_firmware_init");

    if (g_upgrade_config->status.written_size == 0) {
        mdf_event_loop_send(MDF_EVENT_MUPGRADE_STARTED, NULL);
    }

    g_upgrade_config->status.error_code = esp_ota_write(g_upgrade_config->handle, data, size);
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != ESP_OK, g_upgrade_config->status.error_code,
                    "esp_ota_write failed, error_code: %x", g_upgrade_config->status.error_code);

    g_upgrade_config->status.written_size += size;
    MDF_LOGD("Firmware download size: %d, progress rate: %d%%",
             g_upgrade_config->status.written_size,
             g_upgrade_config->status.written_size * 100 / g_upgrade_config->status.total_size);

    if (g_upgrade_config->status.written_size == g_upgrade_config->status.total_size) {
        return mupgrade_firmware_download_finished(g_upgrade_config->status.total_size);
    }

    return MDF_OK;
}

mdf_err_t mupgrade_firmware_download_finished(size_t total_size)
{
    MDF_ERROR_CHECK(!g_upgrade_config, MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT,
                    "Mupgrade firmware is not initialized");

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    g_upgrade_config->status.total_size = total_size;
    g_upgrade_config->status.error_code = esp_ota_end(g_upgrade_config->handle);
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != ESP_OK,
                    MDF_ERR_MUPGRADE_FIRMWARE_INVALID, "esp_ota_end");

    g_upgrade_config->status.error_code = mupgrade_firmware_check(update_partition);
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != ESP_OK,
                    g_upgrade_config->status.error_code, "mupgrade_firmware_check");

    g_upgrade_config->status.error_code = MDF_ERR_MUPGRADE_FIRMWARE_FINISH;

    return MDF_OK;
}

mdf_err_t mupgrade_root_handle(const uint8_t *addr, const void *data, size_t size)
{
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);
    MDF_ERROR_CHECK(!g_upgrade_config, MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT,
                    "Mupgrade firmware is not initialized");
    MDF_ERROR_CHECK(!g_mupgrade_send_running_flag, MDF_ERR_NOT_SUPPORTED,
                    "Mupgrade has stopped running");

    mupgrade_queue_t *q_data = MDF_MALLOC(sizeof(mupgrade_queue_t) + size);

    q_data->size = size;
    memcpy(q_data->src_addr, addr, MWIFI_ADDR_LEN);
    memcpy(q_data->data, data, size);
    MDF_LOGD("addr: " MACSTR ", size: %d", MAC2STR(addr), size);

    if (!xQueueSend(g_upgrade_config->queue, &q_data,
                    CONFIG_MUPGRADE_WAIT_RESPONSE_TIMEOUT / portTICK_RATE_MS)) {
        MDF_LOGW("xQueueSend failed");
        MDF_FREE(q_data);
        return MDF_ERR_TIMEOUT;
    }

    return MDF_OK;
}

static bool addrs_remove(uint8_t *addrs_list, size_t *addrs_num, const uint8_t *addr)
{
    for (int i = 0; i < *addrs_num; i++, addrs_list += MWIFI_ADDR_LEN) {
        if (!memcmp(addrs_list, addr, MWIFI_ADDR_LEN)) {
            if (--(*addrs_num)) {
                memcpy(addrs_list, addrs_list + MWIFI_ADDR_LEN, (*addrs_num - i) * MWIFI_ADDR_LEN);
            }

            return true;
        }
    }

    return false;
}

static mdf_err_t mupgrade_request_status(uint8_t *progress_array, mupgrade_result_t *result)
{
    mdf_err_t ret                      = MDF_OK;
    mupgrade_queue_t *q_data           = NULL;
    size_t request_num                 = 0;
    uint8_t *request_addrs             = NULL;
    mupgrade_status_t *response_status = NULL;
    mupgrade_status_t request_status   = {0x0};
    mwifi_data_type_t data_type        = {
        .upgrade = true,
        .communicate = MWIFI_COMMUNICATE_MULTICAST
    };

    result->requested_num = 0;
    MDF_FREE(result->requested_addr);

    while (xQueueReceive(g_upgrade_config->queue, &q_data, CONFIG_MUPGRADE_WAIT_RESPONSE_TIMEOUT / portTICK_RATE_MS)) {
        if (((mupgrade_status_t *)q_data->data)->written_size == ((mupgrade_status_t *)q_data->data)->total_size) {
            ret = addrs_remove(result->unfinished_addr, &result->unfinished_num, q_data->src_addr);
            MDF_ERROR_CONTINUE(ret != true, "The device has been removed from the list waiting for the upgrade");

            result->successed_num++;
            result->successed_addr = MDF_REALLOC(result->successed_addr,
                                                 result->successed_num * MWIFI_ADDR_LEN);
            memcpy(result->successed_addr + (result->successed_num - 1) * MWIFI_ADDR_LEN,
                   q_data->src_addr, MWIFI_ADDR_LEN);
        }

        MDF_FREE(q_data);

        if (result->unfinished_num == 0) {
            return MDF_OK;
        }
    }

    request_num   = result->unfinished_num;
    request_addrs = MDF_MALLOC(result->unfinished_num * MWIFI_ADDR_LEN);

    memcpy(request_addrs, result->unfinished_addr, MWIFI_ADDR_LEN * request_num);
    memset(progress_array, 0xFF, MUPGRADE_PACKET_MAX_NUM / 8);

    memcpy(&request_status, &g_upgrade_config->status, sizeof(mupgrade_status_t));
    request_status.type = MUPGRADE_TYPE_STATUS;

    for (int i = 0; i < 3 && request_num > 0; ++i) {
        if (mwifi_root_write(request_addrs, request_num, &data_type,
                             &request_status, sizeof(mupgrade_status_t), true) != MDF_OK) {
            MDF_LOGW("Request devices upgrade status");
        }

        while (request_num > 0) {
            ret = xQueueReceive(g_upgrade_config->queue, &q_data,
                                CONFIG_MUPGRADE_WAIT_RESPONSE_TIMEOUT / portTICK_RATE_MS);

            if (ret != pdTRUE) {
                MDF_LOGD("xQueueReceive failed");
                break;
            }

            response_status = (mupgrade_status_t *)q_data->data;
            ret = (response_status->error_code != MDF_OK) ? response_status->error_code : MDF_OK;

            MDF_LOGD("Response, src_addr: " MACSTR ", request_num: %d, total_size: %d, written_size: %d, error_code: %s",
                     MAC2STR(q_data->src_addr), request_num, response_status->total_size,
                     response_status->written_size, mdf_err_to_name(response_status->error_code));

            /**< Delete the device whose upgrade data has been received */
            if (!addrs_remove(request_addrs, &request_num, q_data->src_addr)) {
                MDF_FREE(q_data);
                continue;
            }

            if (response_status->written_size != response_status->total_size) {
                result->requested_num++;
                result->requested_addr = MDF_REALLOC(result->requested_addr,
                                                     result->requested_num * MWIFI_ADDR_LEN);
                memcpy(result->requested_addr + (result->requested_num - 1) * MWIFI_ADDR_LEN,
                       q_data->src_addr, MWIFI_ADDR_LEN);
            }

            if (response_status->written_size == 0) {
                memset(progress_array, 0x0, MUPGRADE_PACKET_MAX_NUM / 8);
            } else if (response_status->written_size == response_status->total_size) {
                ret = addrs_remove(result->unfinished_addr, &result->unfinished_num, q_data->src_addr);
                MDF_ERROR_CONTINUE(ret != true, "The device has been removed from the list waiting for the upgrade");

                result->successed_num++;
                result->successed_addr = MDF_REALLOC(result->successed_addr,
                                                     result->successed_num * MWIFI_ADDR_LEN);
                memcpy(result->successed_addr + (result->successed_num - 1) * MWIFI_ADDR_LEN,
                       q_data->src_addr, MWIFI_ADDR_LEN);
            } else {
                for (int i = 0; i < MUPGRADE_PACKET_MAX_NUM / 8; i++) {
                    progress_array[i] &= response_status->progress_array[i];
                }
            }

            MDF_FREE(q_data);
        }
    }

    ret = MDF_OK;

    if (request_num > 0) {
        MDF_LOGD("MDF_ERR_MUPGRADE_SEND_PACKET_LOSS");
        ret = MDF_ERR_MUPGRADE_SEND_PACKET_LOSS;
    }

    for (int i = 0; i < MUPGRADE_PACKET_MAX_NUM / 8; ++i) {
        if (progress_array[i] != 0xFF) {
            ret = MDF_ERR_MUPGRADE_FIRMWARE_INCOMPLETE;
            MDF_LOGD("MDF_ERR_MUPGRADE_FIRMWARE_INCOMPLETE");
            break;
        }
    }

    MDF_FREE(request_addrs);
    return ret;
}

mdf_err_t mupgrade_result_free(mupgrade_result_t *result)
{
    MDF_PARAM_CHECK(result);

    result->unfinished_num = 0;
    result->requested_num  = 0;
    result->successed_num  = 0;
    MDF_FREE(result->unfinished_addr);
    MDF_FREE(result->requested_addr);
    MDF_FREE(result->successed_addr);

    return MDF_OK;
}

mdf_err_t mupgrade_firmware_send(const uint8_t *addrs_list, size_t addrs_num,
                                 mupgrade_result_t *res)
{
    MDF_PARAM_CHECK(addrs_list);
    MDF_PARAM_CHECK(addrs_num);
    MDF_PARAM_CHECK(!MWIFI_ADDR_IS_EMPTY(addrs_list));
    MDF_PARAM_CHECK(addrs_num > 0 && addrs_num <= esp_mesh_get_routing_table_size());
    MDF_ERROR_CHECK(!g_upgrade_config, MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT,
                    "Mupgrade firmware is not initialized");
    MDF_ERROR_CHECK(g_upgrade_config->status.error_code != MDF_ERR_MUPGRADE_FIRMWARE_FINISH,
                    MDF_ERR_MUPGRADE_FIRMWARE_INCOMPLETE, "mupgrade_firmware_download");

    mdf_err_t ret             = MDF_OK;
    mwifi_data_type_t type    = {.upgrade = true, .communicate = MWIFI_COMMUNICATE_MULTICAST};
    mupgrade_packet_t *packet = MDF_MALLOC(sizeof(mupgrade_packet_t));
    uint8_t *progress_array   = MDF_MALLOC(MUPGRADE_PACKET_MAX_NUM / 8);
    mupgrade_result_t *result = MDF_CALLOC(1, sizeof(mupgrade_result_t));
    g_mupgrade_send_running_flag = true;

    if (MWIFI_ADDR_IS_ANY(addrs_list) || MWIFI_ADDR_IS_BROADCAST(addrs_list)) {
        result->unfinished_num  = esp_mesh_get_routing_table_size();
        result->unfinished_addr = MDF_MALLOC(result->unfinished_num * MWIFI_ADDR_LEN);
        ret = esp_mesh_get_routing_table((mesh_addr_t *)result->unfinished_addr,
                                         result->unfinished_num * MWIFI_ADDR_LEN,
                                         (int *)&result->unfinished_num);

        if (MWIFI_ADDR_IS_BROADCAST(addrs_list)) {
            uint8_t root_mac[6] = {0x0};
            ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));
            addrs_remove(result->unfinished_addr, &result->unfinished_num, root_mac);
            ret = result->unfinished_num > 2048 || result->unfinished_num <= 0 ? MDF_ERR_MUPGRADE_DEVICE_NO_EXIST : MDF_OK;
            MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "addrs_num: %d", result->unfinished_num);
        }

        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Get routing table of this device's sub-network");
    } else {
        result->unfinished_num  = addrs_num;
        result->unfinished_addr = MDF_MALLOC(result->unfinished_num * MWIFI_ADDR_LEN);
        memcpy(result->unfinished_addr, addrs_list, result->unfinished_num * MWIFI_ADDR_LEN);
    }

    uint16_t packet_num = (g_upgrade_config->status.total_size + MUPGRADE_PACKET_MAX_SIZE - 1) / MUPGRADE_PACKET_MAX_SIZE;
    uint16_t last_packet_size = g_upgrade_config->status.total_size % MUPGRADE_PACKET_MAX_SIZE;
    last_packet_size = (!last_packet_size) ? MUPGRADE_PACKET_MAX_SIZE : last_packet_size;
    packet->type = MUPGRADE_TYPE_DATA;
    packet->size = MUPGRADE_PACKET_MAX_SIZE;
    g_upgrade_config->status.written_size = 0;
    MDF_LOGD("packet_num: %d, total_size: %d", packet_num, g_upgrade_config->status.total_size);

    for (int i = 0; i < CONFIG_MUPGRADE_RETRY_COUNT && result->unfinished_num > 0 && g_mupgrade_send_running_flag; ++i) {
        if ((ret = mupgrade_request_status(progress_array, result)) == ESP_OK) {
            break;
        }

        MDF_LOGD("Mupgrade_firmware_send unfinished_num: %d", result->unfinished_num);

        for (int i = 0; i < result->unfinished_num; ++i) {
            MDF_LOGD("Count: %d, addr: " MACSTR, i, MAC2STR(result->unfinished_addr + i * MWIFI_ADDR_LEN));
        }

        for (packet->seq = 0; result->requested_num > 0 && packet->seq < packet_num && g_mupgrade_send_running_flag; ++packet->seq) {
            if (!MUPGRADE_GET_BITS(progress_array, packet->seq)) {
                packet->size = (packet->seq == packet_num - 1) ? last_packet_size : MUPGRADE_PACKET_MAX_SIZE;
                ret = esp_partition_read(g_upgrade_config->partition, packet->seq * MUPGRADE_PACKET_MAX_SIZE,
                                         packet->data, packet->size);
                MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "<%s> Read data from Flash", mdf_err_to_name(ret));

                for (mupgrade_queue_t *q_data = NULL; xQueueReceive(g_upgrade_config->queue, &q_data, 0);) {
                    if (((mupgrade_status_t *)q_data->data)->written_size == ((mupgrade_status_t *)q_data->data)->total_size) {
                        ret = addrs_remove(result->unfinished_addr, &result->unfinished_num, q_data->src_addr);
                        MDF_ERROR_CONTINUE(ret != true, "The device has been removed from the list waiting for the upgrade");
                        ret = addrs_remove(result->requested_addr, &result->requested_num, q_data->src_addr);
                        MDF_ERROR_CONTINUE(ret != true, "The device has been removed from the list of data sent for upgrade");

                        result->successed_num++;
                        result->successed_addr = MDF_REALLOC(result->successed_addr,
                                                             result->successed_num * MWIFI_ADDR_LEN);
                        memcpy(result->successed_addr + (result->successed_num - 1) * MWIFI_ADDR_LEN,
                               q_data->src_addr, MWIFI_ADDR_LEN);
                    }

                    MDF_FREE(q_data);

                    if (!result->unfinished_num) {
                        goto EXIT;
                    }
                }

                if ((MWIFI_ADDR_IS_ANY(addrs_list) || MWIFI_ADDR_IS_BROADCAST(addrs_list))
                        && result->successed_num < 2 && addrs_num == 1) {
                    MDF_LOGD("seq: %d, size: %d, addrs_num: %d", packet->seq, packet->size, addrs_num);

                    if (MWIFI_ADDR_IS_ANY(addrs_list) && result->successed_num == 1) {
                        uint8_t broadcast_addr[] = MWIFI_ADDR_BROADCAST;
                        ret = mwifi_root_write(broadcast_addr, addrs_num, &type,
                                               packet, sizeof(mupgrade_packet_t), true);
                    } else {
                        ret = mwifi_root_write(addrs_list, addrs_num, &type,
                                               packet, sizeof(mupgrade_packet_t), true);
                    }
                } else {
                    MDF_LOGD("seq: %d, size: %d, addrs_num: %d", packet->seq, packet->size, result->requested_num);
                    ret = mwifi_root_write(result->requested_addr, result->requested_num, &type,
                                           packet, sizeof(mupgrade_packet_t), true);
                }

                MDF_ERROR_CONTINUE(ret != ESP_OK, "<%s> Mwifi root write", mdf_err_to_name(ret));
            }
        }
    }

EXIT:

    ret = (result->unfinished_num > 0) ? MDF_ERR_MUPGRADE_FIRMWARE_INCOMPLETE : MDF_OK;
    g_mupgrade_send_running_flag = false;

    if (res) {
        memcpy(res, result, sizeof(mupgrade_result_t));
    } else {
        mupgrade_result_free(result);
    }

    for (mupgrade_queue_t *q_data = NULL; xQueueReceive(g_upgrade_config->queue, &q_data, 0);) {
        MDF_FREE(q_data);
    }

    MDF_FREE(packet);
    MDF_FREE(progress_array);
    MDF_FREE(result);

    if (g_mupgrade_send_exit_sem) {
        xSemaphoreGive(g_mupgrade_send_exit_sem);
    }

    return ret;
}

mdf_err_t mupgrade_firmware_stop()
{
    if (!g_mupgrade_send_running_flag) {
        return MDF_OK;
    }

    if (!g_mupgrade_send_exit_sem) {
        g_mupgrade_send_exit_sem = xSemaphoreCreateBinary();
    }

    g_mupgrade_send_running_flag = false;

    xSemaphoreTake(g_mupgrade_send_exit_sem, portMAX_DELAY);
    vQueueDelete(g_mupgrade_send_exit_sem);
    g_mupgrade_send_exit_sem = NULL;

    return MDF_OK;
}
