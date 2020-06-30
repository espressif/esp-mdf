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
#include "mespnow.h"
#include "esp_wifi.h"
#include "esp_console.h"

#include "mdebug.h"

typedef struct {
    uint8_t version;
    uint8_t type;
    uint8_t data[0];
} mdebug_espnow_data_t;

#define MDEBUG_ESPNOW_TIMEOUT_MS (30 * 1000)
#define MDEBUG_ESPNOW_STORE_KEY  "mdebug_espnow"

static const char *TAG  = "mdebug_espnow";
static bool g_mdebug_espnow_is_running        = false;
static TaskHandle_t g_espnow_send_task_handle = NULL;

mdf_err_t mdebug_espnow_write(const uint8_t *dest_addr, const void *data, size_t size,
                              mdebug_espnow_t type, TickType_t wait_ticks)
{
    if (!g_mdebug_espnow_is_running) {
        return MDF_ERR_NOT_INIT;
    }

    static SemaphoreHandle_t s_espnow_write_lock = NULL;

    if (!s_espnow_write_lock) {
        s_espnow_write_lock = xSemaphoreCreateMutex();
    }

    mdf_err_t ret = MDF_OK;
    mdebug_espnow_data_t *espnow_data = MDF_MALLOC(sizeof(mdebug_espnow_data_t) + size);
    MDF_ERROR_CHECK(!espnow_data, MDF_ERR_NO_MEM, "");

    espnow_data->type = type;
    memcpy(espnow_data->data, data, size);

    /**< Wait for other tasks to be sent before send ESP-WIFI-MESH data */
    if (!xSemaphoreTake(s_espnow_write_lock, wait_ticks)) {
        MDF_FREE(espnow_data);
        return MDF_ERR_TIMEOUT;
    }

    mespnow_add_peer(ESP_IF_WIFI_STA, dest_addr, NULL);
    ret = mespnow_write(MESPNOW_TRANS_PIPE_DEBUG, dest_addr, espnow_data,
                        size + sizeof(mdebug_espnow_data_t), wait_ticks);
    mespnow_del_peer(dest_addr);

    /**< ESP-WIFI-MESH send completed, release send lock */
    xSemaphoreGive(s_espnow_write_lock);

    MDF_FREE(espnow_data);

    return ret;
}

mdf_err_t mdebug_espnow_read(uint8_t *src_addr, void *data, size_t *size,
                             mdebug_espnow_t *type, TickType_t wait_ticks)
{
    if (!g_mdebug_espnow_is_running) {
        return MDF_ERR_NOT_INIT;
    }

    mdf_err_t ret = MDF_OK;
    mdebug_espnow_data_t *espnow_data = MDF_MALLOC(sizeof(mdebug_espnow_data_t) + *size);
    MDF_ERROR_CHECK(!espnow_data, MDF_ERR_NO_MEM, "");

    ret = mespnow_read(MESPNOW_TRANS_PIPE_DEBUG, src_addr, espnow_data, size, wait_ticks);

    if (ret == MDF_ERR_TIMEOUT) {
        goto EXIT;
    }

    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "mespnow_read");

    MDF_LOGV("size: %d, data: %s", *size, espnow_data->data);

    *type = espnow_data->type;
    *size -= sizeof(mdebug_espnow_data_t);
    memcpy(data, espnow_data->data, *size);

EXIT:
    MDF_FREE(espnow_data);

    return ret;
}

static void mdebug_espnow_send_task(void *arg)
{
    mdf_err_t ret             = ESP_OK;
    uint8_t src_addr[6]       = { 0 };
    size_t recv_size          = 0;
    uint8_t *recv_data        = MDF_REALLOC_RETRY(NULL, MESPNOW_PAYLOAD_LEN);
    mdebug_espnow_t recv_type = 0;

    for (; g_mdebug_espnow_is_running;) {
        recv_size = MESPNOW_PAYLOAD_LEN - 1;

        if (mdebug_espnow_read(src_addr, recv_data, &recv_size, &recv_type,
                               pdMS_TO_TICKS(MDEBUG_ESPNOW_TIMEOUT_MS)) == MDF_OK) {
            MDF_LOGV("recv_size: %d, recv_data: %s", recv_size, recv_data);

            switch (recv_type) {
                case MDEBUG_ESPNOW_CONSOLE: {
                    recv_data[recv_size] = '\0';
                    esp_err_t err = esp_console_run((char *)recv_data, &ret);

                    if (err == ESP_ERR_NOT_FOUND) {
                        MDF_LOGE("Unrecognized command");
                    } else if (err == ESP_ERR_INVALID_ARG) {
                        /**< Command was empty */
                    } else if (err == ESP_OK && ret != ESP_OK) {
                        MDF_LOGE("Command returned non-zero error code: 0x%x", ret);
                    } else if (err != ESP_OK) {
                        MDF_LOGE("Internal error: 0x%x", err);
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }

    MDF_FREE(recv_data);
    g_espnow_send_task_handle = NULL;
    vTaskDelete(NULL);
}

mdf_err_t mdebug_espnow_init()
{
    if (g_mdebug_espnow_is_running) {
        return MDF_OK;
    }

    g_mdebug_espnow_is_running = true;

    mdf_err_t ret             = MDF_OK;
    uint8_t channel           = 1;
    wifi_second_chan_t second = 0;

    ret = mespnow_init();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mespnow_init");

    ESP_ERROR_CHECK(esp_wifi_get_channel(&channel, &second));

    ret = mdebug_log_init();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mdebug_log_init");

    if (channel == 0) {
        ESP_ERROR_CHECK(esp_wifi_set_channel(13, WIFI_SECOND_CHAN_NONE));
    }

    if (!g_espnow_send_task_handle) {
        xTaskCreatePinnedToCore(mdebug_espnow_send_task, "mdebug_espnow_send", 3 * 1024,
                                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY - 1,
                                &g_espnow_send_task_handle, CONFIG_MDF_TASK_PINNED_TO_CORE);
    }

    return ESP_OK;
}

mdf_err_t mdebug_espnow_deinit()
{
    if (g_mdebug_espnow_is_running) {
        g_mdebug_espnow_is_running = false;
    }

    return ESP_OK;
}
