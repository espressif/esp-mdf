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
#include "mdf_mem.h"

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
            MDF_LOGI("(%d) <%s: %d> ptr: %p, size: %d", g_mem_info[i].timestamp, g_mem_info[i].tag, g_mem_info[i].line,
                     g_mem_info[i].ptr, g_mem_info[i].size);
            total_size += g_mem_info[i].size;
        }
    }

    MDF_LOGI("Memory record, num: %d, size: %d", g_mem_count, total_size);
}

void mdf_mem_print_heap(void)
{
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    char *task_list_buffer = MDF_MALLOC(uxTaskGetNumberOfTasks() * 64);
    vTaskList(task_list_buffer);

    MDF_LOGI("Task Lists:\nTask Name\tStatus\tPrio\tHWM\tTask\n%s\nCurrent task, Name: %s, HWM: %d\n",
             task_list_buffer, pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));

    MDF_FREE(task_list_buffer);
#endif /**< ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 */


#ifndef CONFIG_SPIRAM_SUPPORT
    MDF_LOGI("Free heap, current: %d, minimum: %d",
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#else
    MDF_LOGI("Free heap, internal current: %d, minimum: %d, total current: %d, minimum: %d",
             heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#endif /**< CONFIG_SPIRAM_SUPPORT */
}
