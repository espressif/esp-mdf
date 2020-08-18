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

#define MDEBUG_LOG_MAX_SIZE   CONFIG_MDEBUG_LOG_PACKET_MAX_SIZE  /**< Set log length size */
#define MDEBUG_LOG_STORE_KEY  "mdebug_log"
#define MDEBUG_LOG_QUEUE_SIZE (30)
#define MDEBUG_LOG_TIMEOUT_MS (30 * 1000)
#define MDEBUG_LOG_QUEUE_BUFFER_MAX_SIZE   (10 * 1024)

static xQueueHandle g_log_queue            = NULL;
static mdebug_log_config_t *g_log_config   = NULL;
static TaskHandle_t g_log_send_task_handle = NULL;
static const char *TAG  = "mdebug_log";
static uint32_t g_log_queue_buffer_size    = 0;

mdf_err_t mdebug_log_get_config(mdebug_log_config_t *config)
{
    return mdf_info_load(MDEBUG_LOG_STORE_KEY, config, sizeof(mdebug_log_config_t));
}

mdf_err_t mdebug_log_set_config(const mdebug_log_config_t *config)
{
    mdf_err_t ret = MDF_OK;

    mdebug_log_init();

    if (config) {
        if (config->log_flash_enable) { /**< Set log flash enable */
            mdebug_flash_init();
        } else {
            mdebug_flash_deinit();
        }

        if (config->log_espnow_enable) { /**< Set log espnow enable */
            mdebug_espnow_init();
        } else {
            mdebug_espnow_deinit();
        }

        memcpy(g_log_config, config, sizeof(mdebug_log_config_t));
        ret = mdf_info_save(MDEBUG_LOG_STORE_KEY, config, sizeof(mdebug_log_config_t));
    } else {
        ret = mdf_info_erase(MDEBUG_LOG_STORE_KEY);
    }

    return ret;
}

static ssize_t mdebug_log_vprintf(const char *fmt, va_list vp)
{
    size_t log_data_size         = 0;
    mdebug_log_queue_t *log_data = NULL;

    size_t log_size = MDEBUG_LOG_MAX_SIZE;

    /**
     * @brief Get the enable mode through configuration
     * */
    if (g_log_config->log_uart_enable) {
        log_size = vprintf(fmt, vp);
    }

    if (!g_log_config
            || ((MDEBUG_ADDR_IS_EMPTY(g_log_config->dest_addr)
                 || !g_log_config->log_espnow_enable)
                && !g_log_config->log_flash_enable)) {
        goto EXIT;
    }

    /**
     * @brief As log transferred via ESP-NOW will influence performance of MDF network,
     *        therefore if the log is too long, it will be divided and only the first
     *        MDEBUG_LOG_MAX_SIZE Bytes will be transferred.
     */
    if (log_size > MDEBUG_LOG_MAX_SIZE) {
        log_size = MDEBUG_LOG_MAX_SIZE;
    }

    if (g_log_queue_buffer_size > MDEBUG_LOG_QUEUE_BUFFER_MAX_SIZE) {
        goto EXIT;
    }

    log_data_size = sizeof(mdebug_log_queue_t) + log_size + 1;
    log_data = malloc(log_data_size);
    MDF_ERROR_GOTO(!log_data, EXIT, "");

    log_data->size = log_size;
    log_data->type = 0;

    if (g_log_config->log_espnow_enable && !MDEBUG_ADDR_IS_EMPTY(g_log_config->dest_addr)) {
        log_data->type |= MDEBUG_LOG_TYPE_ESPNOW;
    }

    if (g_log_config->log_flash_enable) {
        log_data->type |= MDEBUG_LOG_TYPE_FLASH;
    }

    vsnprintf((char *)log_data->data, log_size + 1, fmt, vp);

    g_log_queue_buffer_size += log_size;

    if (xQueueSend(g_log_queue, &log_data, 0) == pdFALSE) {
        free(log_data);
        g_log_queue_buffer_size -= log_size;
        goto EXIT;
    }

EXIT:

    return log_size;
}

static void mdebug_log_send_task(void *arg)
{
    mdebug_log_queue_t *log_data = NULL;

    for (; g_log_config;) {
        if (xQueueReceive(g_log_queue, &log_data, pdMS_TO_TICKS(MDEBUG_LOG_TIMEOUT_MS)) == pdPASS) {
            /**
             * @brief Control log data type and use param MDEBUG_LOG_TYPE_ESPNOW and param MDEBUG_LOG_TYPE_FLASH.
             */
            if (log_data->type & MDEBUG_LOG_TYPE_ESPNOW) {
                mdebug_espnow_write(g_log_config->dest_addr, log_data->data,
                                    log_data->size, MDEBUG_ESPNOW_LOG, pdMS_TO_TICKS(MDEBUG_LOG_TIMEOUT_MS));
            }

            if (log_data->type & MDEBUG_LOG_TYPE_FLASH && log_data->size > 0) { /**< Valid data only after the data reaches 14 */
                /**
                 * @brief Clear color related characters.
                 */
                char *data  = log_data->data;
                size_t size = log_data->size;

                /**
                 * @brief Remove the header and tail that appear in the string in the log
                 *
                 */
                if (log_data->data[0] == '\033' && log_data->size > 7) {
                    data = log_data->data + 7;
                    size = log_data->size - 7;

                    char *p = strstr(data, LOG_RESET_COLOR);

                    if (p != NULL) {
                        *p++ = '\n';
                        size = p - data;
                    }
                }

                MDEBUG_PRINTF("src_size: %d, size: %d, strlen: %d, data: %02x, data: %.*s\n",
                              log_data->size, size, strlen(data), data[0], size, data);
                mdebug_flash_write(data, size); /**< Write log data to flash */
            }

            g_log_queue_buffer_size -= log_data->size;
            MDF_FREE(log_data);
        }
    }

    g_log_send_task_handle = NULL;
    vTaskDelete(NULL);
}

mdf_err_t mdebug_log_init()
{
    static bool log_init_flag = false;

    if (log_init_flag) {
        return 0;
    }

    if (!g_log_config) {
        g_log_config = MDF_CALLOC(1, sizeof(mdebug_log_config_t));
        MDF_ERROR_CHECK(!g_log_config, MDF_ERR_NO_MEM, "");
    }

    if (mdebug_log_get_config(g_log_config) != MDF_OK) {
        g_log_config->log_uart_enable = true;
    }

    /**< Register espnow log redirect function */
    esp_log_set_vprintf(mdebug_log_vprintf);

    if (!g_log_queue) {
        g_log_queue = xQueueCreate(MDEBUG_LOG_QUEUE_SIZE, sizeof(mdebug_log_queue_t *));
        MDF_ERROR_CHECK(!g_log_queue, ESP_FAIL, "g_log_queue create fail");
    }

    if (!g_log_send_task_handle) {
        xTaskCreatePinnedToCore(mdebug_log_send_task, "mdebug_log_send", 3 * 1024,
                                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY - 2,
                                &g_log_send_task_handle, CONFIG_MDF_TASK_PINNED_TO_CORE);
    }

    MDF_LOGW("Mdebug log initialized successfully");
    log_init_flag = true;

    return ESP_OK;
}

mdf_err_t mdebug_log_deinit()
{
    if (g_log_config) {
        mdebug_log_queue_t *log_data = NULL;

        while (xQueueReceive(g_log_queue, &log_data, 0) == pdPASS) {
            MDF_FREE(log_data);
        }

        g_log_queue_buffer_size = 0;

        MDF_FREE(g_log_config);
        g_log_config = NULL;
    }

    return ESP_OK;
}
