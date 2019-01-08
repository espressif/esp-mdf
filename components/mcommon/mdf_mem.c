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
#include "mdf_mem.h"

#define MDF_MEM_DBG_INFO_MAX     128

typedef struct {
    void *ptr;
    int size;
    const char *tag;
    int line;
    uint32_t timestamp;
} mdf_mem_info_t;

static const char *TAG            = "mdf_mem";
static uint32_t g_mem_count       = 0;
static mdf_mem_info_t *g_mem_info = NULL;

void mdf_mem_add_record(void *ptr, int size, const char *tag, int line)
{
    if (!ptr || !size || !tag) {
        return;
    }

    MDF_LOGV("<%s : %d> Alloc ptr: %p, size: %d, heap free: %d", tag, line,
             ptr, (int)size, esp_get_free_heap_size());

    if (!g_mem_info) {
        g_mem_info = calloc(MDF_MEM_DBG_INFO_MAX, sizeof(mdf_mem_info_t));
    }

    if (g_mem_count >= MDF_MEM_DBG_INFO_MAX) {
        MDF_LOGE("The buffer space of the memory record is full");
        mdf_mem_print_record();
        return ;
    }

    for (int i = 0; i < MDF_MEM_DBG_INFO_MAX; i++) {
        if (!g_mem_info[i].ptr) {
            g_mem_info[i].ptr  = ptr;
            g_mem_info[i].size = size;
            g_mem_info[i].tag  = tag;
            g_mem_info[i].line = line;
            g_mem_info[i].timestamp = esp_log_timestamp();
            g_mem_count++;
            break;
        }
    }
}

void mdf_mem_remove_record(void *ptr, const char *tag, int line)
{
    if (!ptr) {
        return;
    }

    MDF_LOGV("<%s : %d> Free ptr: %p, heap free: %d", tag, line, ptr, esp_get_free_heap_size());

    if (!g_mem_info) {
        g_mem_info = calloc(MDF_MEM_DBG_INFO_MAX, sizeof(mdf_mem_info_t));
    }

    for (int i = 0; i < MDF_MEM_DBG_INFO_MAX; i++) {
        if (g_mem_info[i].ptr == ptr) {
            memset(g_mem_info + i, 0, sizeof(mdf_mem_info_t));
            g_mem_count--;
            break;
        }
    }
}

void mdf_mem_print_record(void)
{
    size_t total_size = 0;

    if (!MDF_MEM_DEBUG) {
        MDF_LOGE("Please enable memory record");
    }

    if (!g_mem_count || !g_mem_info) {
        MDF_LOGE("Memory record is empty");
        return ;
    }

    for (int i = 0; i < MDF_MEM_DBG_INFO_MAX; i++) {
        if (g_mem_info[i].ptr || g_mem_info[i].size != 0) {
            MDF_LOGW("(%d) <%s: %d> ptr: %p, size: %d", g_mem_info[i].timestamp, g_mem_info[i].tag, g_mem_info[i].line,
                     g_mem_info[i].ptr, g_mem_info[i].size);
            total_size += g_mem_info[i].size;
        }
    }

    MDF_LOGW("Memory record, num: %d, size: %d", g_mem_count, total_size);
}

void mdf_mem_print_heap(void)
{
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    char *task_list_buffer = MDF_MALLOC(uxTaskGetNumberOfTasks() * 64);
    vTaskList(task_list_buffer);

    MDF_LOGW("Task Lists:\nTask Name\tStatus\tPrio\tHWM\tTask\n%s\nCurrent task, Name: %s, HWM: %d\n",
             task_list_buffer, pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));

    MDF_FREE(task_list_buffer);
#endif /**< ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 */


#ifndef CONFIG_SPIRAM_SUPPORT
    MDF_LOGW("Free heap, current: %d, minimum: %d",
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#else
    MDF_LOGW("Free heap, internal current: %d, minimum: %d, total current: %d, minimum: %d",
             heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#endif /**< CONFIG_SPIRAM_SUPPORT */
}
