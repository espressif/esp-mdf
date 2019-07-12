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
