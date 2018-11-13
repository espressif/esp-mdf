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
