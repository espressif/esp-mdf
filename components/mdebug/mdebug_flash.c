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
#include "esp_spiffs.h"
#include "esp_console.h"

#include "mdf_common.h"
#include "mespnow.h"
#include "mdebug.h"

#define MDEBUG_FLASH_FILE_MAX_NUM     2                                 /**< Create several files */
#define MDEBUG_FLASH_FILE_MAX_SIZE    CONFIG_MDEBUG_FLASH_FILE_MAX_SIZE /**< File storage size */
#define MDEBUG_FLASH_STORE_KEY        "mdebug_flash"
#define MDEBUG_FILE_OPERATION_TIMEOUT 10000

/**
 * @brief Create files configuration
 */
typedef struct {
    size_t size; /**< Files size */
    int offset;  /**< Set file pointer offset */
    FILE *fp;    /**< Create files */
} flash_log_info_t;

static int g_log_index               = 0;
static bool g_mdebug_flash_init_flag = false;
static SemaphoreHandle_t g_file_operation_lock = NULL;
static flash_log_info_t g_log_info[MDEBUG_FLASH_FILE_MAX_NUM] = {0};
static const char *TAG = "mdebug_flash";

mdf_err_t mdebug_flash_init()
{
    if (g_mdebug_flash_init_flag) {
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;
    size_t total  = 0;
    size_t used   = 0;
    size_t log_min_size = MDEBUG_FLASH_FILE_MAX_SIZE;

    if (!g_file_operation_lock) {
        g_file_operation_lock = xSemaphoreCreateMutex();
    }

    /**< Create spiffs file and number of files */
    esp_vfs_spiffs_conf_t conf = {
        .base_path       = "/spiffs",
        .partition_label = NULL,
        .max_files       = MDEBUG_FLASH_FILE_MAX_NUM,
        .format_if_mount_failed = true
    };

    /**
     * @brief Use settings defined above to initialize and mount SPIFFS filesystem.
     *
     * @note: esp_vfs_spiffs_register is an all-in-one convenience function.
     */
    ret = esp_vfs_spiffs_register(&conf);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Mount or format filesystem");

    ret = esp_spiffs_info(NULL, &total, &used);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Get SPIFFS partition information");
    MDF_LOGD("SPIFFS partition information, total: %d, used: %d", total, used);

    if (used) {
        mdf_info_load(MDEBUG_FLASH_STORE_KEY, g_log_info,
                      sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);
    }

    /**
     * @brief Use POSIX and C standard library functions to work with files.
     *        First create a file.
     */
    MDF_LOGD("Opening file");

    /**< Create two files */
    for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        struct stat file_stat   = {0};
        char const *file_mode = NULL;
        char file_name[32]    = {0};

        sprintf(file_name, "/spiffs/log_%d.text", i); /**< Create multiple files name */
        stat(file_name, &file_stat);                  /**< Get file status */

        /**< Reading and writing of subsequent files by setting the file output mode */
        file_mode = !stat(file_name, &file_stat) ? "r+" : "w+";

        g_log_info[i].fp = fopen(file_name, file_mode);
        MDF_ERROR_CHECK(!g_log_info[i].fp, errno, "Get SPIFFS partition information");

        /**< Whether file is full */
        if (g_log_info[i].size < MDEBUG_FLASH_FILE_MAX_SIZE && g_log_info[i].size < log_min_size) {
            log_min_size = g_log_info[i].size;
            g_log_index  = i;  /**< File index */
        }

        printf("g_log_info[i].size: %d, offset: %d, file_stat.size: %d\n",
               g_log_info[i].size, g_log_info[i].offset, (int)file_stat.st_size);
        g_log_info[i].size = MIN(g_log_info[i].size, file_stat.st_size);
    }

    g_mdebug_flash_init_flag = true;
    MDF_LOGI("Mdebug flash initialized successfully");

    return MDF_OK;
}

mdf_err_t mdebug_flash_deinit()
{
    if (!g_mdebug_flash_init_flag) {
        return MDF_OK;
    }

    g_mdebug_flash_init_flag = false;

    for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        fclose(g_log_info[i].fp);
    }

    if (g_file_operation_lock) {
        vSemaphoreDelete(g_file_operation_lock);
        g_file_operation_lock = NULL;
    }

    /**< All done, unmount partition and disable SPIFFS */
    esp_vfs_spiffs_unregister(NULL);
    MDF_LOGD("SPIFFS unmounted");
    MDF_LOGD("Mdebug flash de-initialized successfully");

    return MDF_OK;
}

mdf_err_t mdebug_flash_write(const char *data, size_t size)
{
    time_t now           = 0;
    mdf_err_t ret        = MDF_OK;
    struct tm timeinfo   = {0};
    char strtime_buf[32] = {0};

    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size > 0);

    if (!g_mdebug_flash_init_flag) {
        return MDF_OK;
    }

    /**< Get the current timestamp */
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strtime_buf, 32, "\n[%Y-%m-%d %H:%M:%S] ", &timeinfo);

    /**
     * @brief Get the address of the pointer to the next file and clear the file to be written
     * and clear offset size.
     */
    if (g_log_info[g_log_index].size >= MDEBUG_FLASH_FILE_MAX_SIZE) {
        g_log_index = (g_log_index + 1) % MDEBUG_FLASH_FILE_MAX_NUM;
        g_log_info[g_log_index].size   = 0;
        g_log_info[g_log_index].offset = 0;
    }

    /**
     * @brief Need to know if the file is full, if it is full, it will report the file size
     */
    static uint32_t s_event_send_tick = 0;

    if (mdebug_flash_size() > MDEBUG_FLASH_FILE_MAX_SIZE
            && (xTaskGetTickCount() - s_event_send_tick > 30000 || !s_event_send_tick)) {
        s_event_send_tick = xTaskGetTickCount();
        mdf_event_loop_send(MDF_EVENT_MDEBUG_FLASH_FULL, NULL);
    }

    /**
     * @brief Change the write file address, then wirte timestamp data
     */
    if (!xSemaphoreTake(g_file_operation_lock, pdMS_TO_TICKS(MDEBUG_FILE_OPERATION_TIMEOUT))) {
        return MDF_ERR_TIMEOUT;
    }

    fseek(g_log_info[g_log_index].fp, g_log_info[g_log_index].size, SEEK_SET);
    ret = fwrite(strtime_buf, 1, strlen(strtime_buf), g_log_info[g_log_index].fp);

    xSemaphoreGive(g_file_operation_lock);

    MDF_ERROR_CHECK(ret <= 0, errno, "fwrite packet");
    g_log_info[g_log_index].size += ret;

    /**
     * @brief Change the write file address, then wirte log data after the timestamp.
     *
     * @note First need to get the length of the timestamp, then write the log data after the timestamp
     */
    if (!xSemaphoreTake(g_file_operation_lock, pdMS_TO_TICKS(MDEBUG_FILE_OPERATION_TIMEOUT))) {
        return MDF_ERR_TIMEOUT;
    }

    fseek(g_log_info[g_log_index].fp, g_log_info[g_log_index].size, SEEK_SET);
    ret = fwrite(data, 1, size, g_log_info[g_log_index].fp);
    xSemaphoreGive(g_file_operation_lock);

    MDF_ERROR_CHECK(ret <= 0, errno, "fwrite packet");
    g_log_info[g_log_index].size += ret;
    /**
     * @brief Here in order to read the data address written by the file,Get where the data address is written.
     *
     */
    mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);

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
        return MDF_OK;
    }

    /**
     * @brief First find Target file index address, then read the this files data
     *
     * @note The file address read is different from the file address written, so that the same address
     * can be prevented from being written and read.
     */
    for (int i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM && *size > read_size; ++i, data += read_size) {
        flash_log_info_t *log_info = g_log_info + ((g_log_index + 1 + i) % MDEBUG_FLASH_FILE_MAX_NUM);

        if (log_info->size > 0 && log_info->offset < log_info->size) {
            if (!xSemaphoreTake(g_file_operation_lock, pdMS_TO_TICKS(MDEBUG_FILE_OPERATION_TIMEOUT))) {
                return MDF_ERR_TIMEOUT;
            }

            fseek(log_info->fp, log_info->offset, SEEK_SET);
            ret = fread(data, 1, MIN(*size - read_size, log_info->size - log_info->offset), log_info->fp);

            xSemaphoreGive(g_file_operation_lock);

            if (ret <= 0) {
                MDF_LOGW("<%s> fread packet, ret: %d, *size: %d, read_size: %d, log_info->offset: %d, log_info->size: %d, i: %d",
                         strerror(errno), ret, *size, read_size, log_info->offset, log_info->size, i);
                log_info->offset = log_info->size = 0;
                break;
            }

            read_size += ret;
            log_info->offset += ret;
            MDEBUG_PRINTF("*size: %d, read_size: %d, log_info->offset: %d, log_info->size: %d, ret: %d, i: %d\n",
                          *size, read_size, log_info->offset, log_info->size, ret, i);
            mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);
        }
    }

    *size = read_size;
    return read_size > 0 ? MDF_OK : MDF_FAIL;
}

mdf_err_t mdebug_flash_erase()
{
    for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        g_log_index          = 0;
        g_log_info[i].size   = 0;
        g_log_info[i].offset = 0;
        mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);
    }

    return MDF_OK;
}

size_t mdebug_flash_size()
{
    size_t size = 0;

    for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
        if (g_log_info[i].size >= g_log_info[i].offset) {
            size += g_log_info[i].size - g_log_info[i].offset;
        }
    }

    return size;
}
