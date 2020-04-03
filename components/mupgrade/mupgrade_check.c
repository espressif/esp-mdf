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

#include "rom/rtc.h"

#include "mupgrade.h"

#ifndef CONFIG_MUPGRADE_FIRMWARE_FLAG
#define CONFIG_MUPGRADE_FIRMWARE_FLAG   "** MUPGRADE_FIRMWARE_FLAG **"
#endif

#define MUPGRADE_FIRMWARE_FLAG           CONFIG_MUPGRADE_FIRMWARE_FLAG
#define MUPGRADE_FIRMWARE_FLAG_SIZE      32
#define MUPGRADE_STORE_RESTART_COUNT_KEY "mupgrade_count"

static const char *TAG = "mupgrade_check";

mdf_err_t mupgrade_version_fallback()
{
    mdf_err_t ret = MDF_OK;
    const esp_partition_t *partition = NULL;

#ifdef CONFIG_MUPGRADE_VERSION_FALLBACK_FACTORY

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                         ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

#endif /**< CONFIG_MUPGRADE_VERSION_FALLBACK_FACTORY */

    if (partition == NULL) {
        partition = esp_ota_get_next_update_partition(NULL);
    }

    ret = mupgrade_firmware_check(partition);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mupgrade_firmware_check failed!");

    ret = esp_ota_set_boot_partition(partition);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ota_set_boot_partition failed!");

    MDF_LOGI("The next reboot will fall back to the previous version");

    return MDF_OK;
}

#ifdef CONFIG_MUPGRADE_VERSION_FALLBACK_RESTART

static void restart_count_erase_timercb(void *timer)
{
    if (!xTimerStop(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerStop timer: %p", timer);
    }

    if (!xTimerDelete(timer, portMAX_DELAY)) {
        MDF_LOGE("xTimerDelete timer: %p", timer);
    }

    mdf_info_erase(MUPGRADE_STORE_RESTART_COUNT_KEY);
    MDF_LOGV("erase restart count");
}

static bool restart_trigger()
{
    mdf_err_t ret          = MDF_OK;
    TimerHandle_t timer    = NULL;
    uint32_t restart_count = 0;
    uint32_t restart_count_lenght = sizeof(uint32_t);
    RESET_REASON reset_reason     = rtc_get_reset_reason(0);

    if (reset_reason == POWERON_RESET || reset_reason == RTCWDT_RTC_RESET
            || reset_reason == DEEPSLEEP_RESET || reset_reason == RTCWDT_BROWN_OUT_RESET) {
        restart_count = 1;
    } else {
        mdf_info_load(MUPGRADE_STORE_RESTART_COUNT_KEY, &restart_count, &restart_count_lenght);
        restart_count++;
        MDF_LOGW("restart reason: %d, count: %d", reset_reason, restart_count);
    }

    /**< If the device restarts within the instruction time,
    the event_mdoe value will be incremented by one */
    ret = mdf_info_save(MUPGRADE_STORE_RESTART_COUNT_KEY, &restart_count, sizeof(uint32_t));
    MDF_ERROR_CHECK(ret != ESP_OK, false, "Save the number of restarts within the set time");

    timer = xTimerCreate("restart_count_erase", CONFIG_MUPGRADE_RESTART_TIMEOUT / portTICK_RATE_MS,
                         false, NULL, restart_count_erase_timercb);
    MDF_ERROR_CHECK(!timer, false, "xTaskCreate, timer: %p", timer);

    xTimerStart(timer, 0);

    if (restart_count < CONFIG_MUPGRADE_RESTART_COUNT) {
        ret = false;
    } else {
        mdf_info_erase(MUPGRADE_STORE_RESTART_COUNT_KEY);
        ret = true;
    }

    return ret;
}

#ifndef CONFIG_FREERTOS_UNICORE

static void mupgrade_version_fallback_task(void *arg)
{
    if (restart_trigger() && mupgrade_version_fallback() == MDF_OK) {
        esp_restart();
    }

    MDF_LOGD("version_fallback_task exit");

    vTaskDelete(NULL);
}

#endif /**< CONFIG_FREERTOS_UNICORE */

#endif /**< CONFIG_MUPGRADE_VERSION_FALLBACK_RESTART */

__attribute((constructor)) mdf_err_t mupgrade_partition_switch()
{
    const volatile uint8_t firmware_flag[MUPGRADE_FIRMWARE_FLAG_SIZE] = MUPGRADE_FIRMWARE_FLAG;

    (void)firmware_flag;
    MDF_LOGD("Add an identifier to the firmware: %s", firmware_flag);

#ifdef CONFIG_MUPGRADE_VERSION_FALLBACK_RESTART

#ifdef CONFIG_FREERTOS_UNICORE
    mdf_err_t ret = MDF_OK;

    if (restart_trigger()) {
        ret = mupgrade_version_fallback();
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ota_set_boot_partition failed!");

        esp_restart();
    }

#else
    xTaskCreate(mupgrade_version_fallback_task, "mupgrade_version_fallback", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY + 5, NULL);
#endif /**< CONFIG_FREERTOS_UNICORE */

#endif /**< CONFIG_MUPGRADE_VERSION_FALLBACK_RESTART */

    return MDF_OK;
}

#ifdef CONFIG_MUPGRADE_FIRMWARE_CHECK
/**
 * @brief Knuth–Morris–Pratt algorithm, a search algorithm
 */
static void kmp_next(const uint8_t *target, size_t size, uint8_t *next)
{
    next[0] = 0;

    for (int i = 1, j = 0; i < size; i++) {
        while (j > 0 && target[j] != target[i]) {
            j = next[j - 1];
        }

        if (target[j] == target[i]) {
            j = j + 1;
        }

        next[i] = j;
    }
}

static mdf_err_t kmp_find(const uint8_t *src, int src_size,
                          const uint8_t *target, int target_size, const uint8_t *next)
{
    for (int i = 0, next_index = 0; i <= src_size - target_size; i++) {
        while (next_index != 0 && (src[i] != target[next_index])) {
            next_index = next[next_index - 1];
        }

        if (src[i] == target[next_index]) {
            next_index++;

            if (next_index == target_size) {
                return MDF_OK;
            }
        }
    }

    return MDF_FAIL;
}

mdf_err_t mupgrade_firmware_check(const esp_partition_t *partition)
{
    MDF_PARAM_CHECK(partition);

    mdf_err_t ret            = MDF_OK;
    size_t firmware_size     = 1024;
    uint8_t *firmware_buffer = MDF_MALLOC(firmware_size);
    uint8_t firmware_flag[MUPGRADE_FIRMWARE_FLAG_SIZE]   = MUPGRADE_FIRMWARE_FLAG;
    uint8_t kmp_next_buffer[MUPGRADE_FIRMWARE_FLAG_SIZE] = {0};
    size_t flag_size = strlen((char *)firmware_flag);

    if (flag_size > MUPGRADE_FIRMWARE_FLAG_SIZE) {
        flag_size = MUPGRADE_FIRMWARE_FLAG_SIZE;
    }

    kmp_next(firmware_flag, flag_size, kmp_next_buffer);

    for (int firmware_addr = partition->address;
            firmware_addr + firmware_size < partition->address + partition->size;
            firmware_addr = firmware_addr + firmware_size - MUPGRADE_FIRMWARE_FLAG_SIZE) {

        ret = spi_flash_read(firmware_addr, firmware_buffer, firmware_size);
        MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Read data from Flash", mdf_err_to_name(ret));

        ret = kmp_find(firmware_buffer, firmware_size,
                       firmware_flag, flag_size, kmp_next_buffer);
        MDF_ERROR_BREAK(ret == ESP_OK, "");
    }

    MDF_FREE(firmware_buffer);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_ERR_MUPGRADE_FIRMWARE_INVALID,
                    "The firmware is not generated by this project");
    return MDF_OK;
}

#else

mdf_err_t mupgrade_firmware_check(const esp_partition_t *partition)
{
    return MDF_OK;
}

#endif /**< CONFIG_MUPGRADE_FIRMWARE_CHECK */
