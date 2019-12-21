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

static const char *TAG = "mcommom_examples";

/**
 * @brief Store data in flash
 */
static void log_test()
{
    /**
     * @brief Set log level for given tag
     *        You can set the log level for different files for debugging.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief 1. The difference between MDF_LOGX and ESP_LOGX:
     *           - Added line number
     *           - Add TAG parameters by default
     *           - Separate log enabled macro
     *        2. Support log input and redirection, can receive wirelessly via `wireless_debug`
     */
    MDF_LOGV("Bigger chunks of debugging information, or frequent messages which can potentially flood the output");
    MDF_LOGD("Extra information which is not necessary for normal use (values, pointers, sizes, etc)");
    MDF_LOGI("Information messages which describe normal flow of events");
    MDF_LOGW("Error conditions from which recovery measures have been taken");
    MDF_LOGE("Critical errors, software module can not recover on its own");
}

/**
 * @brief Store data in flash
 */
static void info_store_test()
{
    const char *key = "test"; /**< least 15 characters */
    uint32_t value  = 0;

    /**
     * @brief Initialize the default NVS partition
     */
    MDF_ERROR_ASSERT(mdf_info_init());

    /**
     * @brief Save the information with given key
     *
     * @note Ensure that key is less than 15 characters.
     */
    value = 10;
    MDF_ERROR_ASSERT(mdf_info_save(key, &value, sizeof(uint32_t)));

    /**
     * @brief Load the information
     */
    value = 0;
    MDF_ERROR_ASSERT(mdf_info_load(key, &value, sizeof(uint32_t)));
    MDF_LOGI("value: %d", value);

    /**
     * @brief  Erase the information with given key
     */
    MDF_ERROR_ASSERT(mdf_info_erase(key));
}

/**
 * @brief Location of memory problems
 */
static void memory_test()
{
    uint8_t *data = NULL;

    /**
     * @brief Leak checking: find memory which is allocated and never freed
     */
    for (int i = 0; i < 10; ++i) {
        data = (uint8_t *)MDF_MALLOC(10);
    }

    /**
     * @brief Print memory and free space on the stack
     */
    mdf_mem_print_heap();

    /**
    * @brief Print all MDF_XALLOC assignments but not free memory
    */
    mdf_mem_print_record();

    /**
     * @brief Print the state of tasks in the system
     */
    mdf_mem_print_task();

    /**
     * @brief Heap use analysis: show all functions that are allocating/freeing
     *        memory while the trace is running.
     *        CONFIG_HEAP_POISONING_COMPREHENSIVE=y
     *        CONFIG_HEAP_TRACING=y
     *        CONFIG_HEAP_TASK_TRACKING=y
     *
     * @note Enabling “Comprehensive” detection has a substantial runtime performance
     *       impact (as all memory needs to be set to the allocation patterns each time
     *       a malloc/free completes, and the memory also needs to be checked each time.)
     *       However it allows easier detection of memory corruption bugs which are much
     *       more subtle to find otherwise. It is recommended to only enable this mode
     *       when debugging, not in production.
     */

#ifndef CONFIG_SPIRAM_SUPPORT

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    data[10] = 0;

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

#endif

    // MDF_FREE(data);
}

/**
 * @brief Event handler
 */
#define EVENT_TEST       (MDF_EVENT_CUSTOM_BASE + 1)
#define EVENT_DELAY_TEST (MDF_EVENT_CUSTOM_BASE + 2)

static mdf_err_t event_loop_test_cb(mdf_event_loop_t event, void *ctx)
{
    switch (event) {
        case EVENT_TEST:
            MDF_LOGI("EVENT_TEST, value: %d", (int)ctx);
            break;

        case EVENT_DELAY_TEST:
            MDF_LOGI("EVENT_DELAY_TEST, value: %d", (int)ctx);
            break;

        default:
            break;
    }

    return MDF_OK;
}

static void event_loop_test()
{
    int value = 1;

    /**
     * @brief Esp-mdf All events are maintained by a unified event_loop_task,
     *        you can send custom events as follows
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_test_cb));
    MDF_ERROR_ASSERT(mdf_event_loop_send(EVENT_TEST, (void *)value));
    MDF_ERROR_ASSERT(mdf_event_loop_delay_send(EVENT_DELAY_TEST, (void *)value, 1000 / portTICK_RATE_MS));
}

/**
 * @brief Log level setting
 */
void app_main()
{
    MDF_LOGI("***************************************************");
    MDF_LOGI("                   Log output                      ");
    MDF_LOGI("***************************************************");
    log_test();

    MDF_LOGI("***************************************************");
    MDF_LOGI("                   info store                      ");
    MDF_LOGI("***************************************************");
    info_store_test();

    MDF_LOGI("***************************************************");
    MDF_LOGI("                   event loop                      ");
    MDF_LOGI("***************************************************");
    event_loop_test();

    MDF_LOGI("***************************************************");
    MDF_LOGI("                   memory debug                    ");
    MDF_LOGI("***************************************************");
    memory_test();
}
