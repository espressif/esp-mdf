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

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "mdf_common.h"
#include "mdf_ota.h"

static esp_ota_handle_t update_handle          = 0;
static ssize_t binary_file_length              = 0;
static int ota_start_time                      = 0;
static const esp_partition_t *update_partition = NULL;
static const char *TAG                         = "mdf_ota";

esp_err_t mdf_ota_start(void)
{
    esp_err_t ret                 = ESP_OK;
    const esp_partition_t *running = esp_ota_get_running_partition();
    update_partition               = esp_ota_get_next_update_partition(NULL);

    MDF_ASSERT(running && update_partition);

    MDF_LOGD("running partition, type %d, subtype %d, at offset: %x",
             running->type, running->subtype, running->address);
    MDF_LOGD("boot partition, type %d, subtype %d, at offset: %x",
             update_partition->type, update_partition->subtype, update_partition->address);

    /**< the image size is not yet known, pass OTA_SIZE_UNKNOWN */
    ret = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ota_begin failed, ret: %x", ret);

    /**< record ota time from esp_ota_begin() to esp_ota_end() */
    ota_start_time = xTaskGetTickCount();

    MDF_LOGI("mdf_ota_start succeeded");

    return ESP_OK;
}

esp_err_t __mdf_ota_write(const char *buf, uint32_t size, ssize_t offset)
{
    MDF_PARAM_CHECK(size);
    MDF_PARAM_CHECK(buf);
    MDF_PARAM_CHECK(offset < 1024 * 2048);

    esp_err_t ret = ESP_OK;

    if (!update_partition) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        MDF_ASSERT(update_partition);
    }

    if (offset == MDF_OTA_OFFSET_NONE) {
        ret = esp_ota_write(update_handle, (const void *)buf, size);
    } else {
        ret = spi_flash_write(update_partition->address + offset, (const void *)buf, size);
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ota_write failed, ret: %x", ret);

    binary_file_length += size;
    MDF_LOGV("write size: %d, binary_file_length: %d", size, binary_file_length);

    return ESP_OK;
}

esp_err_t mdf_ota_stop(void)
{
    esp_err_t ret = ESP_OK;

    MDF_LOGI("total write binary data length: %d, spend time: %ds",
             binary_file_length, (xTaskGetTickCount() - ota_start_time) * portTICK_RATE_MS / 1000);

    /**< if ESP32 was reset duration OTA, and after restart, the update_handle will be invalid,
         but it still can switch boot partition and reboot successful */
    ret = esp_ota_end(update_handle);
    MDF_LOGD("esp_ota_end, ret: %x, binary_file_length: %d", ret, binary_file_length);

    /**< in case of chip reseted during waiting for ota reboot */
    if (!update_partition) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        MDF_ASSERT(update_partition);
    }

    ret = esp_ota_set_boot_partition(update_partition);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ota_set_boot_partition failed! ret: %x", ret);

    MDF_LOGI("ota set boot partition, wait for reboot");

    return ESP_OK;
}
