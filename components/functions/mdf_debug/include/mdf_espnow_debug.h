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
#ifndef __MDF_ESPNOW_DEBUG_H__
#define __MDF_ESPNOW_DEBUG_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef struct {
    uint8_t type;
    uint8_t oprt;
    uint8_t params;
    uint16_t seq;
    size_t size;
    uint8_t data[0];
} mdf_espnow_debug_pkt_t;

#define DEBUG_PKT_STRUCT_LEN  sizeof(mdf_espnow_debug_pkt_t)

/**
 * @brief  get coredump partition info
 *
 * @param  length coredump data len, if length is "-1" means no coredump data
 *
 * @return        const pointer to coredump partition
 */
const esp_partition_t *mdf_coredump_get_info(size_t *length);

/**
 * @brief  erase coredump partition
 *
 * @return   ESP_OK: erase success
 *           ESP_FAIL: erase fail
 */
esp_err_t mdf_coredump_erase(void);

/**
 * @brief  set espnow debug module
 *
 * @param  espnow_config espnow config params
 *
 * @return   ESP_OK
 *           ESP_FAIL
 */
esp_err_t mdf_espnow_set_config(const mdf_espnow_debug_pkt_t *espnow_config);

/**
 * @brief  init espnow debug module
 *
 * @return   ESP_OK
 *           ESP_FAIL
 */
esp_err_t mdf_espnow_debug_init(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_DEBUG_H__ */
