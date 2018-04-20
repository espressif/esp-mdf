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
#ifndef __MDF_ESPNOW_H__
#define __MDF_ESPNOW_H__

#include "freertos/portmacro.h"
#include "errno.h"
#include "esp_log.h"
#include "esp_now.h"

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MDF_ESPNOW_INTERFACE ESP_IF_WIFI_STA

#ifndef CONFIG_MDF_ESPNOW_PMK
#define CONFIG_MDF_ESPNOW_PMK "pmk1234567890123"
#endif /**< CONFIG_MDF_ESPNOW_PMK */
#define MDF_ESPNOW_PMK CONFIG_MDF_ESPNOW_PMK

#ifndef CONFIG_MDF_ESPNOW_LMK
#define CONFIG_MDF_ESPNOW_LMK "lmk1234567890123"
#endif /**< CONFIG_MDF_ESPNOW_LMK */
#define MDF_ESPNOW_LMK CONFIG_MDF_ESPNOW_LMK

/**< TODO mdf_espnow_debug module use these MACRO too.
    change ets_printf to ESP_LOG* or other log module
    when transpservert to other project */
#define MDF_ESPNOW_CHECK(con, err, format, ...) do { \
        if (con) { \
            ets_printf(format , ##__VA_ARGS__); \
            if(errno) { ets_printf("errno: %d, errno_str: %s\n", errno, strerror(errno)); } \
            return err; \
        } \
    }while (0)

#define MDF_ESPNOW_CONTINUE(con, format, ...) do { \
        if (con) { \
            ets_printf(format , ##__VA_ARGS__); \
            if(errno) { ets_printf("errno: %d, errno_str: %s\n", errno, strerror(errno)); } \
            continue; \
        } \
    }while (0)

#define MDF_ESPNOW_GOTO(con, label, format, ...) do { \
        if (con) { \
            ets_printf(format , ##__VA_ARGS__); \
            if(errno) { ets_printf("errno: %d, errno_str: %s\n", errno, strerror(errno)); } \
            goto label; \
        } \
    }while (0)

#define MDF_ESPNOW_INFO(con, format, ...) do { \
        if (con) { \
            ets_printf(format , ##__VA_ARGS__); \
            if(errno) { ets_printf("errno: %d, errno_str: %s\n", errno, strerror(errno)); } \
        } \
    }while (0)

#define mdf_espnow_malloc(size) ({void *ptr = malloc(size); \
        if(!ptr){ ets_printf("[%s, %d] malloc size: %d, ptr: %p, heap free: %d\n", \
                                 __FUNCTION__, __LINE__ , size, ptr, esp_get_free_heap_size()); assert(ptr); } ptr;})

#define mdf_espnow_calloc(n, size) ({void *ptr = calloc(n, size); \
        if(!ptr){ ets_printf("[%s, %d] calloc size: %d, ptr: %p, heap free: %d\n", \
                                 __FUNCTION__, __LINE__ , (n) * (size), ptr, esp_get_free_heap_size()); assert(ptr);} ptr;})

#define mdf_espnow_free(ptr) {if(ptr){ free(ptr); ptr = NULL;}}while(0)

typedef enum mdf_espnow_type {
    MDF_ESPNOW_NONE = 0,    /**< espnow debug pkt: log, coredump, ack */
    MDF_ESPNOW_DEBUG,       /**< espnow control pkt: set/get light color, properties, u/m/b cast to others */
    MDF_ESPNOW_CONTROL,     /**< espnow netconfig pkt */
    MDF_ESPNOW_NETCONFIG,   /**< reserve for other function */
    MDF_ESPNOW_RESERVED,
    MDF_ESPNOW_MAX,
} mdf_espnow_pkt_type_t;

/**
 * @brief  espnow add peer. it is convenient use simplify MACRO.
 *         if default_encrypt is true, use default lmk to encrypt espnow data;
 *         if default_encrypt is false and lmk is set, use customer lmk to encrypt espnow data;
 *         otherwise, not encrypt.
 *
 * @param  dest_addr       peer mac address
 * @param  default_encrypt if encrypt
 * @param  lmk             local master key
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_add_peer_base(const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                                   bool default_encrypt, const uint8_t lmk[ESP_NOW_KEY_LEN]);
#define mdf_espnow_add_peer(dest_addr, lmk)            mdf_espnow_add_peer_base(dest_addr, false, lmk)
#define mdf_espnow_add_peer_no_encrypt(dest_addr)      mdf_espnow_add_peer_base(dest_addr, false, NULL)
#define mdf_espnow_add_peer_default_encrypt(dest_addr) mdf_espnow_add_peer_base(dest_addr, true, NULL)

/**
 * @brief  espnow delete peer
 *
 * @param  dest_addr peer mac address
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_del_peer(const uint8_t dest_addr[ESP_NOW_ETH_ALEN]);

/**
 * @brief  read data from espnow
 * @attention: when the data received from sourcer is encrypt, the device will not received
 *             befor add the device into espnow_peer.
 *
 * @param  type        mdf espnow packet type
 * @param  src_addr    source address
 * @param  data        point to receive data buffer
 * @param  data_len    receive data len
 * @param  block_ticks block ticks
 *
 * @return data_len: received data len
 *         - ESP_FAIL: read fail
 */
size_t mdf_espnow_read(mdf_espnow_pkt_type_t type, uint8_t src_addr[ESP_NOW_ETH_ALEN],
                       void *data, size_t data_len, TickType_t block_ticks);

/**
 * @brief  send espnow pkt to destinatation
 *
 * @attention: the src_addr can be STA or AP interface of the device, and can be config in menuconfig
 *
 * @param  type        mdf espnow packet type
 * @param  dest_addr   destination address
 * @param  data        point to send data buffer
 * @param  data_len    send data len
 * @param  block_ticks block ticks
 *
 * @return - data_len: send data len
 *         - MEESH_FAIL: write fail
 */
size_t mdf_espnow_write(mdf_espnow_pkt_type_t type, const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                        const void *data, size_t data_len, TickType_t block_ticks);

/**
 * @brief  deinit espnow in mdf
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_deinit(void);

/**
 * @brief  init espnow in mdf
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_init(void);

/**
 * @brief  enable given type of espnow queue
 *
 * @param  type espnow queue type
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_enable(mdf_espnow_pkt_type_t type);

/**
 * @brief  disable given type of espnow queue
 *
 * @param  type espnow queue type
 *
 * @return
 *          - ESP_OK
 *          - ESP_FAIL
 */
esp_err_t mdf_espnow_disable(mdf_espnow_pkt_type_t type);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_H__ */
