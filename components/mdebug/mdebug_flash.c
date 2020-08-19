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

#include "esp_wifi.h"
#include "esp_console.h"

#include "mdf_common.h"
#include "mespnow.h"
#include "mdebug.h"

#define MDEBUG_FLASH_FILE_MAX_NUM   2                                 /**< Create several files */
#define MDEBUG_LOG_FILE_MAX_SIZE    CONFIG_MDEBUG_LOG_FILE_MAX_SIZE   /**< File storage size */
#define MDEBUG_FLASH_STORE_KEY      "mdebug_log_info"

/**
 * @brief Create files configuration
 */
typedef struct {
    size_t size; /**< Files size */
    int offset;  /**< Set file pointer offset */
    uint32_t addr;
} flash_log_info_t;

static int g_log_index                   = 0;
static bool g_mdebug_flash_init_flag     = false;
static const esp_partition_t *g_log_part = NULL;
static flash_log_info_t g_log_info[MDEBUG_FLASH_FILE_MAX_NUM] = {0};
static const char *TAG = "mdebug_flash";

mdf_err_t mdebug_flash_init()
{
    if (g_mdebug_flash_init_flag) {
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;

    esp_partition_iterator_t part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA,
                                         ESP_PARTITION_SUBTYPE_ANY, CONFIG_MDEBUG_LOG_PARTITION_LABEL);
    MDF_ERROR_CHECK(!part_itra, MDF_ERR_NOT_SUPPORTED, "partition no find, subtype: 0x%x, label: %s",
                    ESP_PARTITION_SUBTYPE_ANY, CONFIG_MDEBUG_LOG_PARTITION_LABEL);

    g_log_part = esp_partition_get(part_itra);

    MDF_ERROR_CHECK(!g_log_part, MDF_ERR_NOT_SUPPORTED, "esp_partition_get");
    MDF_ERROR_CHECK(g_log_part->size < MDEBUG_LOG_FILE_MAX_SIZE, MDF_ERR_NOT_SUPPORTED,
                    "Log file (%d Byte) size must be smaller than partition size (%d Byte).",
                    MDEBUG_LOG_FILE_MAX_SIZE, g_log_part->size);
    MDF_ERROR_CHECK(MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM % 4096 != 0, MDF_ERR_NOT_SUPPORTED,
                    "The size of the log partition must be an integer of %d KB.", MDEBUG_FLASH_FILE_MAX_NUM * 4);

    if (mdf_info_load(MDEBUG_FLASH_STORE_KEY, g_log_info,
                      sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM) != MDF_OK) {
        ret = esp_partition_erase_range(g_log_part, CONFIG_MDEBUG_LOG_PARTITION_OFFSET, MDEBUG_LOG_FILE_MAX_SIZE);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_partition_erase_range");
    }

    /**< Create two files */
    for (size_t i = 0, min_size = MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        g_log_info[i].addr = CONFIG_MDEBUG_LOG_PARTITION_OFFSET + MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM * i;

        /**< Whether file is full */
        if (g_log_info[i].size < MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM && g_log_info[i].size < min_size) {
            min_size = g_log_info[i].size;
            g_log_index  = i;  /**< File index */
        }
    }

    g_mdebug_flash_init_flag = true;
    MDF_LOGI("Mdebug flash initialized successfully");
    MDF_LOGI("Log save partition subtype: label: %s, addr: 0x%d, offset: %d, size: %d",
             CONFIG_MDEBUG_LOG_PARTITION_LABEL, g_log_part->address, CONFIG_MDEBUG_LOG_PARTITION_OFFSET, g_log_part->size);

    return MDF_OK;
}

mdf_err_t mdebug_flash_deinit()
{
    if (!g_mdebug_flash_init_flag) {
        return MDF_FAIL;
    }

    g_mdebug_flash_init_flag = false;

    MDF_LOGD("Mdebug flash de-initialized successfully");

    return MDF_OK;
}

mdf_err_t mdebug_flash_write(const char *data, size_t size)
{
    time_t now           = 0;
    mdf_err_t ret        = MDF_OK;
    struct tm timeinfo   = {0};
    char strtime_buf[32] = {0};
    static bool flag_timestamp = true;

    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size > 0);

    if (!g_mdebug_flash_init_flag) {
        return MDF_FAIL;
    }

    /**
     * @brief Get the address of the pointer to the next file and clear the file to be written
     * and clear offset size.
     */
    if (g_log_info[g_log_index].size + size > MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM) {
        g_log_index = (g_log_index + 1) % MDEBUG_FLASH_FILE_MAX_NUM;
        g_log_info[g_log_index].size   = 0;
        g_log_info[g_log_index].offset = 0;

        ret = esp_partition_erase_range(g_log_part, g_log_info[g_log_index].addr, MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_partition_erase_range, addr: %x", g_log_info[g_log_index].addr);
    }

    /**
     * @brief Need to know if the file is full, if it is full, it will report the file size
     */
    static uint32_t s_event_send_tick = 0;

    if (mdebug_flash_size() > MDEBUG_LOG_FILE_MAX_SIZE / MDEBUG_FLASH_FILE_MAX_NUM
            && (xTaskGetTickCount() - s_event_send_tick > 30000 || !s_event_send_tick)) {
        s_event_send_tick = xTaskGetTickCount();
        mdf_event_loop_send(MDF_EVENT_MDEBUG_FLASH_FULL, NULL);
    }

    MDEBUG_PRINTF("esp_partition_write, addr: %d, offset: %d, log_index: %d, size: %d\n",
                  g_log_info[g_log_index].addr, g_log_info[g_log_index].size, g_log_index, size);

    if (flag_timestamp) {
        flag_timestamp = false;

        /**< Get the current timestamp */
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strtime_buf, 32, "[%Y-%m-%d %H:%M:%S] ", &timeinfo);

        /**
         * @brief Change the write file address, then wirte timestamp data
         */
        ret = esp_partition_write(g_log_part, g_log_info[g_log_index].addr + g_log_info[g_log_index].size, strtime_buf, strlen(strtime_buf));
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_partition_write");
        g_log_info[g_log_index].size += strlen(strtime_buf);
    }

    /**
     * @brief Change the write file address, then wirte log data after the timestamp.
     *
     * @note First need to get the length of the timestamp, then write the log data after the timestamp
     */
    ret = esp_partition_write(g_log_part, g_log_info[g_log_index].addr + g_log_info[g_log_index].size, data, size);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_partition_write");
    g_log_info[g_log_index].size += size;

    /**
     * @brief Here in order to read the data address written by the file,Get where the data address is written.
     *
     */
    mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);

    if (data[size - 1] == '\n') {
        flag_timestamp = true;
    }

    return MDF_OK;
}

mdf_err_t mdebug_flash_read(char *data, size_t *size)
{
    mdf_err_t ret    = MDF_OK;
    size_t read_size = 0;
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size && *size > 0);

    if (!g_mdebug_flash_init_flag) {
        *size = 0;
        return MDF_FAIL;
    }

    /**
     * @brief First find Target file index address, then read the this files data
     *
     * @note The file address read is different from the file address written, so that the same address
     * can be prevented from being written and read.
     */
    for (int i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM && *size > read_size; ++i, data += read_size) {
        flash_log_info_t *log_info = g_log_info + ((g_log_index + 1 + i) % MDEBUG_FLASH_FILE_MAX_NUM);
        ssize_t data_size = MIN(*size - read_size, log_info->size - log_info->offset);

        if (data_size > 0) {
            ret = esp_partition_read(g_log_part, log_info->addr + log_info->offset, data, data_size);

            if (ret != MDF_OK) {
                MDF_LOGW("<%s> fread packet, ret: %d, *size: %d, read_size: %d, log_info->offset: %d, log_info->size: %d, index: %d",
                         strerror(errno), ret, *size, read_size, log_info->offset, log_info->size, i);
                log_info->offset = log_info->size = 0;
                break;
            }

            read_size += data_size;
            log_info->offset += data_size;
            MDEBUG_PRINTF("esp_partition_read, data_size: %d, read_size: %d, log_info->offset: %d, log_info->size: %d, ret: %d, index: %d, data: %.*s\n",
                          data_size, read_size, log_info->offset, log_info->size, ret, i, data_size, data);
            mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);
        }
    }

    *size = read_size;
    return read_size > 0 ? MDF_OK : MDF_FAIL;
}

mdf_err_t mdebug_flash_erase()
{
    if (!g_mdebug_flash_init_flag) {
        return MDF_FAIL;
    }

    mdf_err_t ret = MDF_OK;

    mdf_info_erase(MDEBUG_FLASH_STORE_KEY);

    ret = esp_partition_erase_range(g_log_part, CONFIG_MDEBUG_LOG_PARTITION_OFFSET, MDEBUG_LOG_FILE_MAX_SIZE);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_partition_erase_range");

    return MDF_OK;
}

size_t mdebug_flash_size()
{
    if (!g_mdebug_flash_init_flag) {
        return 0;
    }

    size_t size = 0;

    for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        if (g_log_info[i].size >= g_log_info[i].offset) {
            size += g_log_info[i].size - g_log_info[i].offset;
        }
    }

    return size;
}
