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

#ifndef __MDF_COMMON_H__
#define __MDF_COMMON_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_partition.h"
#include "esp_system.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/i2c.h"
#include "sys/param.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#ifndef CONFIG_MDF_TASK_DEFAULT_PRIOTY
#define CONFIG_MDF_TASK_DEFAULT_PRIOTY ((tskIDLE_PRIORITY) + 6)
#endif /**< CONFIG_MDF_TASK_DEFAULT_PRIOTY */
#define MDF_TASK_DEFAULT_PRIOTY CONFIG_MDF_TASK_DEFAULT_PRIOTY

#ifndef CONFIG_MDF_LOG_LEVEL
#define CONFIG_MDF_LOG_LEVEL ESP_LOG_DEBUG
#endif /**< CONFIG_MDF_LOG_LEVEL */

#define MDF_ETH_ALEN    6 /*!< length of MAC address */

#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL CONFIG_MDF_LOG_LEVEL

#define MDF_LOGE( format, ... ) ESP_LOGE(TAG, "[%d]:" format, __LINE__, ##__VA_ARGS__)
#define MDF_LOGW( format, ... ) ESP_LOGW(TAG, "[%d]:" format, __LINE__, ##__VA_ARGS__)
#define MDF_LOGI( format, ... ) ESP_LOGI(TAG, "[%d]:" format, __LINE__, ##__VA_ARGS__)
#define MDF_LOGD( format, ... ) ESP_LOGD(TAG, "[%d]:" format, __LINE__, ##__VA_ARGS__)
#define MDF_LOGV( format, ... ) ESP_LOGV(TAG, "[%d]:" format, __LINE__, ##__VA_ARGS__)

/**
 * @brief count the number of function parameters
 */
#define COUNT_PARMS2(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _, ...) _
#define COUNT_PARMS(...)\
    COUNT_PARMS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/**
 * @brief check the return value
 */
#define MDF_ERROR_CHECK(con, err, format, ...) do { \
        if (con) { \
            MDF_LOGE(format , ##__VA_ARGS__); \
            if(errno) { MDF_LOGE("errno: %d, errno_str: %s\n", errno, strerror(errno)); } \
            return err; \
        } \
    }while (0)

#define MDF_ERROR_GOTO(con, lable, format, ...) do { \
        if (con) { \
            MDF_LOGE(format , ##__VA_ARGS__); \
            if(errno) { MDF_LOGE("errno: %d, errno_str: %s", errno, strerror(errno)); } \
            goto lable; \
        } \
    }while (0)

#define MDF_ERROR_CONTINUE(con, format, ...) { \
        if (con) { \
            MDF_LOGW(format , ##__VA_ARGS__); \
            if(errno) { MDF_LOGE("errno: %d, errno_str: %s", errno, strerror(errno)); } \
            continue; \
        } \
    }

#define MDF_ERROR_BREAK(con, format, ...) { \
        if (con) { \
            MDF_LOGW(format , ##__VA_ARGS__); \
            if(errno) { MDF_LOGE("errno: %d, errno_str: %s", errno, strerror(errno)); } \
            break; \
        } \
    }

/**
 * @brief check param
 */
#define MDF_PARAM_CHECK(con) do { \
        if (!(con)) { MDF_LOGE("parameter error: !(%s)", #con); return ESP_FAIL; } \
    } while (0)

/**
 * @brief Set breakpoint
 */
#define MDF_ASSERT(con) do { \
        if (!(con)) { MDF_LOGE("assert errno:%d, errno_str: !(%s)", errno, strerror(errno)); assert(0 && #con); } \
    } while (0)

#define mdf_malloc(size) ({void *ptr = malloc(size); \
        for (int i = 0; i < 5 && !ptr; ++i) { \
            MDF_LOGE("malloc fail delay 200ms, [%d], size: %d, ptr: %p, heap free: %d", i, size, ptr, esp_get_free_heap_size()); \
            vTaskDelay(200 / portTICK_RATE_MS); \
            ptr = malloc(size); \
        } \
        if(!ptr){MDF_LOGE("malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size());assert(ptr);} \
        MDF_LOGV("malloc size: %d, ptr: %p, heap free: %d", size, ptr, esp_get_free_heap_size()); ptr;})

#define mdf_calloc(n, size) ({void *ptr = calloc(n, size); \
        for (int i = 0; i < 5 && !ptr; ++i) { \
            MDF_LOGE("calloc fail delay 200ms, [%d], size: %d, ptr: %p, heap free: %d", i, (n*size), ptr, esp_get_free_heap_size()); \
            vTaskDelay(200 / portTICK_RATE_MS); \
            ptr = calloc(n, size); \
        } \
        if(!ptr){MDF_LOGE("calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size());assert(ptr);} \
        MDF_LOGV("calloc size: %d, ptr: %p, heap free: %d", (n) * (size), ptr, esp_get_free_heap_size()); ptr;})

#define mdf_free(ptr) {if(ptr){ free(ptr); \
            MDF_LOGV("free ptr: %p, heap free: %d", ptr, esp_get_free_heap_size()); ptr = NULL;}}while(0)

/**
 * @brief convert mac address from ap to sta
 */
#define ADDR_AP2STA(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) - 1); \
    } while(0)

/**
 * @brief convert mac address from sta to ap
 */
#define ADDR_STA2AP(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) + 1); \
    } while(0)

/**
 * @brief convert mac address from bt to sta
 */
#define ADDR_BT2STA(mac) do { \
        *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) - 2); \
    } while(0)

/**
 * @brief  convert mac from string format to binary
 */
static inline uint8_t *str2mac(const char *mac_str, uint8_t *mac)
{
    uint32_t mac_data[6] = {0};

    sscanf(mac_str, "%02x%02x%02x%02x%02x%02x",
           mac_data, mac_data + 1, mac_data + 2, mac_data + 3, mac_data + 4, mac_data + 5);

    for (int i = 0; i < 6; i++) {
        mac[i] = mac_data[i];
    }

    return mac;
}

/**
 * @brief  convert mac from binary format to string
 */
static inline char *mac2str(const uint8_t *mac, char *mac_str)
{
    sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac));

    return mac_str;
}

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_COMMON_H__ */
