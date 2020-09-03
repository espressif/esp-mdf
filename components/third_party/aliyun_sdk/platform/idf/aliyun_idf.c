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

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_ota_ops.h"
#include "esp_wifi.h"

#include "aliyun_defs.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_idf";

static bool gateway_connect_status = false;

static QueueHandle_t g_idf_gateway_read_queue = NULL;
static QueueHandle_t g_idf_gateway_write_queue = NULL;

ESP_EVENT_DEFINE_BASE(ALIYUN_EVENT_BASE);

typedef struct {
    esp_ota_handle_t handle; /**< OTA handle */
    const esp_partition_t *partition; /**< Pointer to partition structure obtained using
                                           esp_partition_find_first or esp_partition_get. */
    esp_err_t status; /**< Upgrade status */
} upgrade_config_t;

static upgrade_config_t *g_upgrade_config = NULL;

typedef struct aliyun_idf_buf {
    uint8_t addr[ALIYUN_SUBDEVICE_ADDRS_MAXLEN];
    uint32_t type;
    uint8_t data[CONFIG_ALIYUN_PAYLOAD_SIZE];
    size_t size;
} aliyun_idf_buf_t;

mdf_err_t aliyun_idf_set_cloud_connect_status(bool status)
{
    gateway_connect_status = status;
    return MDF_OK;
}

bool aliyun_idf_get_cloud_connect_status(void)
{
    return gateway_connect_status;
}

mdf_err_t aliyun_idf_gateway_init(void)
{
    if (g_idf_gateway_read_queue == NULL) {
        g_idf_gateway_read_queue = xQueueCreate(2, sizeof(aliyun_idf_buf_t *));
    }

    if (g_idf_gateway_write_queue == NULL) {
        g_idf_gateway_write_queue = xQueueCreate(2, sizeof(aliyun_idf_buf_t *));
    }

    return MDF_OK;
}

mdf_err_t aliyun_idf_gateway_deinit(void)
{
    if (g_idf_gateway_read_queue != NULL) {
        vQueueDelete(g_idf_gateway_read_queue);
        g_idf_gateway_read_queue = NULL;
    }

    if (g_idf_gateway_write_queue != NULL) {
        vQueueDelete(g_idf_gateway_write_queue);
        g_idf_gateway_write_queue = NULL;
    }

    return MDF_OK;
}

mdf_err_t aliyun_idf_gateway_read(uint8_t *src_addr, aliyun_msg_type_t *type,
                                  uint8_t *data, size_t *size, uint32_t wait_ticks)
{
    MDF_PARAM_CHECK(g_idf_gateway_read_queue);

    aliyun_idf_buf_t *mqtt_buf = NULL;

    if (xQueueReceive(g_idf_gateway_read_queue, &mqtt_buf, wait_ticks) != pdTRUE) {
        MDF_FREE(mqtt_buf);
        return MDF_ERR_TIMEOUT;
    }

    MDF_PARAM_CHECK(mqtt_buf);

    memcpy(src_addr, mqtt_buf->addr, ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
    *type = mqtt_buf->type;
    memcpy(data, mqtt_buf->data, *size > mqtt_buf->size ? *size : mqtt_buf->size);
    *size = mqtt_buf->size;
    MDF_FREE(mqtt_buf);
    return MDF_OK;
}

mdf_err_t aliyun_idf_gateway_write(const uint8_t *dest_addrs, aliyun_msg_type_t type, const void *data, size_t size)
{
    MDF_PARAM_CHECK(g_idf_gateway_write_queue);

    aliyun_idf_buf_t *mqtt_buf = MDF_MALLOC(sizeof(aliyun_idf_buf_t));
    memcpy(mqtt_buf->addr, dest_addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
    mqtt_buf->type = type;
    memcpy(mqtt_buf->data, data, size > CONFIG_ALIYUN_PAYLOAD_SIZE ? CONFIG_ALIYUN_PAYLOAD_SIZE : size);
    mqtt_buf->size = size;
    return xQueueSend(g_idf_gateway_write_queue, &mqtt_buf, portMAX_DELAY) != pdTRUE ? MDF_FAIL : MDF_OK;
}

mdf_err_t aliyun_idf_subdevice_read(aliyun_msg_type_t *type,
                                    void **data, size_t *size, uint32_t wait_ticks)
{
    MDF_PARAM_CHECK(g_idf_gateway_write_queue);
    aliyun_idf_buf_t *mqtt_buf = *data;

    if (xQueueReceive(g_idf_gateway_write_queue, &mqtt_buf, wait_ticks) != pdTRUE) {
        return MDF_ERR_TIMEOUT;
    }

    *type = mqtt_buf->type;
    *size = mqtt_buf->size;
    *data = MDF_MALLOC(mqtt_buf->size);
    memcpy(*data, mqtt_buf->data, mqtt_buf->size);
    MDF_FREE(mqtt_buf);

    return MDF_OK;
}

mdf_err_t aliyun_idf_subdevice_write(aliyun_msg_type_t type, const void *data, size_t size)
{
    MDF_PARAM_CHECK(g_idf_gateway_read_queue);

    aliyun_idf_buf_t *mqtt_buf = MDF_MALLOC(sizeof(aliyun_idf_buf_t));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mqtt_buf->addr);
    mqtt_buf->type = type;
    memcpy(mqtt_buf->data, data, size > CONFIG_ALIYUN_PAYLOAD_SIZE ? CONFIG_ALIYUN_PAYLOAD_SIZE : size);
    mqtt_buf->size = size;
    return xQueueSend(g_idf_gateway_read_queue, &mqtt_buf, portMAX_DELAY) != pdTRUE ? MDF_FAIL : MDF_OK;
}

mdf_err_t aliyun_idf_ota_start(char *ota_version, size_t ota_size)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);

    if (running == NULL || update == NULL) {
        MDF_LOGE("No partition is found or flash read operation failed");
        return MDF_FAIL;
    }

    MDF_LOGI("ota_version: %s, ota_size: %d", ota_version, ota_size);

    MDF_LOGI("Running partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             running->label, running->type, running->subtype, running->address);
    MDF_LOGI("Update partition, label: %s, type: 0x%x, subtype: 0x%x, address: 0x%x",
             update->label, update->type, update->subtype, update->address);

    if (!g_upgrade_config) {
        g_upgrade_config = calloc(1, sizeof(upgrade_config_t));
    }

    g_upgrade_config->partition = update;
    g_upgrade_config->status = esp_ota_begin(update, OTA_SIZE_UNKNOWN,
                               &g_upgrade_config->handle);

    if (g_upgrade_config->status != MDF_OK) {
        MDF_LOGE("esp_ota_begin failed");
    }

    return MDF_OK;
}

mdf_err_t aliyun_idf_ota_write(uint8_t *buffer, size_t length)
{
    if (g_upgrade_config == NULL) {
        MDF_LOGE("upgrade firmware is not initialized");
        return MDF_FAIL;
    }

    esp_err_t ret = MDF_OK;
    ret = esp_ota_write(g_upgrade_config->handle, (const void *)buffer, length);

    if (ret != MDF_OK) {
        MDF_LOGE("HAL_Firmware_Persistence_Write failed! ret=0x%2x", ret);
        return MDF_FAIL;
    }

    return MDF_OK;
}

mdf_err_t aliyun_idf_ota_stop(void)
{
    if (g_upgrade_config == NULL) {
        MDF_LOGE("upgrade firmware is not initialized");
        return MDF_FAIL;
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    g_upgrade_config->status = esp_ota_end(g_upgrade_config->handle);

    if (g_upgrade_config->status != MDF_OK) {
        MDF_LOGE("esp_ota_end failed");
        return MDF_FAIL;
    }

    g_upgrade_config->status = esp_ota_set_boot_partition(update_partition);

    if (g_upgrade_config->status != MDF_OK) {
        MDF_LOGE("esp_ota_set_boot_partition failed! ret=0x%x", g_upgrade_config->status);
        return MDF_FAIL;
    }

    free(g_upgrade_config);
    MDF_LOGI("aliyun ota task exit");

    MDF_LOGW("OTA successfully restarts the device after 3 seconds");

    vTaskDelay(pdMS_TO_TICKS(3000));

    MDF_LOGI("Restart PRO and APP CPUs");
    esp_restart();

    return MDF_OK;
}

mdf_err_t aliyun_idf_event_post(uint32_t event_id, void *ctx)
{
    printf("event_id: %d\n", event_id);
    return esp_event_post(ALIYUN_EVENT_BASE, event_id, NULL, 0, 0);
}
