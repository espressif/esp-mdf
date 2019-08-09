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

#ifndef __MDF_MEM_H__
#define __MDF_MEM_H__

#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#ifdef CONFIG_MDF_MEM_DEBUG
#define MDF_MEM_DEBUG true
#else
#define MDF_MEM_DEBUG false
#endif /**< CONFIG_MDF_MEM_DEBUG */

#ifndef CONFIG_MDF_MEM_DBG_INFO_MAX
#define CONFIG_MDF_MEM_DBG_INFO_MAX     (128)
#endif  /**< CONFIG_MDF_MEM_DBG_INFO_MAX */
#define MDF_MEM_DBG_INFO_MAX CONFIG_MDF_MEM_DBG_INFO_MAX

/**
 * @brief Add to memory record
 *
 * @param ptr  Memory pointer
 * @param size Memory size
 * @param tag  Description tag
 * @param line Line number
 */
void mdf_mem_add_record(void *ptr, int size, const char *tag, int line);

/**
 * @brief Remove from memory record
 *
 * @param ptr  Memory pointer
 * @param tag  Description tag
 * @param line Line number
 */
void mdf_mem_remove_record(void *ptr, const char *tag, int line);

/**
 * @brief Print the all allocation but not released memory
 *
 * @attention Must configure CONFIG_MDF_MEM_DEBUG == y annd esp_log_level_set(mdf_mem, ESP_LOG_INFO);
 */
void mdf_mem_print_record(void);

/**
 * @brief Print memory and free space on the stack
 */
void mdf_mem_print_heap(void);

/**
 * @brief  Malloc memory
 *
 * @param  size  Memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
#define MDF_MALLOC(size) ({ \
        void *ptr = malloc(size); \
        if(!ptr) { \
            MDF_LOGE("<ESP_ERR_NO_MEM> Malloc size: %d, ptr: %p, heap free: %d", (int)size, ptr, esp_get_free_heap_size()); \
            assert(ptr); \
        } \
        if (MDF_MEM_DEBUG) { \
            mdf_mem_add_record(ptr, size, TAG, __LINE__); \
        } \
        ptr; \
    })

/**
 * @brief  Calloc memory
 *
 * @param  n     Number of block
 * @param  size  Block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
#define MDF_CALLOC(n, size) ({ \
        void *ptr = calloc(n, size); \
        if(!ptr) { \
            MDF_LOGE("<ESP_ERR_NO_MEM> Calloc size: %d, ptr: %p, heap free: %d", (int)(n) * (size), ptr, esp_get_free_heap_size()); \
            assert(ptr); \
        } \
        if (MDF_MEM_DEBUG) { \
            mdf_mem_add_record(ptr, (n) * (size), TAG, __LINE__); \
        } \
        ptr; \
    })

/**
 * @brief  Reallocate memory
 *
 * @param  ptr   Memory pointer
 * @param  size  Block memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
#define MDF_REALLOC(ptr, size) ({ \
        void *new_ptr = realloc(ptr, size); \
        if(!new_ptr) { \
            MDF_LOGE("<ESP_ERR_NO_MEM> Realloc size: %d, new_ptr: %p, heap free: %d", (int)size, new_ptr, esp_get_free_heap_size()); \
            assert(new_ptr); \
        } \
        if (MDF_MEM_DEBUG) { \
            mdf_mem_remove_record(ptr, TAG, __LINE__); \
            mdf_mem_add_record(new_ptr, size, TAG, __LINE__); \
        } \
        new_ptr; \
    })

/**
 * @brief  Free memory
 *
 * @param  ptr  Memory pointer
 */
#define MDF_FREE(ptr) { \
        if(ptr) { \
            free(ptr); \
            if (MDF_MEM_DEBUG) { \
                mdf_mem_remove_record(ptr, TAG, __LINE__); \
            } \
            ptr = NULL; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_MEM_H__ */
