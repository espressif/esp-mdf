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

typedef enum mdf_espnow_pkt_type {
    MDF_ESPNOW_NONE = 0,
    MDF_ESPNOW_DEBUG,      /**< debug pkt: log, coredump, espnow_debug config, ack */
    MDF_ESPNOW_CONTROL,    /**< control pkt: set/get device status */
    MDF_ESPNOW_NETCONFIG,  /**< network configuration pkt */
    MDF_ESPNOW_RESERVED,   /**< reserved for other functiond */
    MDF_ESPNOW_MAX,
} mdf_espnow_pkt_type_t;

/**
 * @brief  add a peer to espnow peer list based on esp_now_add_peer(...). It is convenient to use simplified MACRO follows.
 *
 * @attention 1. If the peer is exist, it will be deleted firstly.
 * @attention 2. If default_encrypt is true, use default lmk to encrypt espnow data;
 *               if default_encrypt is false and lmk is set, use customized lmk to encrypt espnow data;
 *               otherwise, not encrypt.
 *
 * @param  dest_addr       peer mac address
 * @param  default_encrypt whether to encrypt data with default MDF_ESPNOW_LMK
 * @param  lmk             local master key
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_add_peer_base(const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                                   bool default_encrypt, const uint8_t lmk[ESP_NOW_KEY_LEN]);
#define mdf_espnow_add_peer(dest_addr, lmk)            mdf_espnow_add_peer_base(dest_addr, false, lmk)
#define mdf_espnow_add_peer_no_encrypt(dest_addr)      mdf_espnow_add_peer_base(dest_addr, false, NULL)
#define mdf_espnow_add_peer_default_encrypt(dest_addr) mdf_espnow_add_peer_base(dest_addr, true, NULL)

/**
 * @brief  delete a peer from espnow peer list.
 *
 * @param  dest_addr peer mac address
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_del_peer(const uint8_t dest_addr[ESP_NOW_ETH_ALEN]);

/**
 * @brief  read data from espnow.
 *         (1) when the data sent from source is encrypt,
 *         the device will not receive befor adding the source into espnow peer.
 *         (2) When the param data_len is shorter than received data len,
 *         the data returned will be shorted to data_len.
 *
 * @param  type        mdf espnow packet type
 * @param  src_addr    source address
 * @param  data        point to received data buffer
 * @param  data_len    received data len
 * @param  block_ticks block ticks
 *
 * @return data_len: actually received data len
 *         - ESP_FAIL: read fail
 */
size_t mdf_espnow_read(mdf_espnow_pkt_type_t type, uint8_t src_addr[ESP_NOW_ETH_ALEN],
                       void *data, size_t data_len, TickType_t block_ticks);

/**
 * @brief  write date package to espnow.
 *         1. It is necessary to add device to espnow_peer befor send data to dest_addr.
 *         2. When data_len to write is too long, it may fail duration some package and
 *         and the return value is the data len that actually sended.
 *
 * @attention:
 *
 * @param  type        mdf espnow packet type
 * @param  dest_addr   destination address
 * @param  data        point to send data buffer
 * @param  data_len    send data len
 * @param  block_ticks block ticks
 *
 * @return data_len: send data len
 *         - ESP_FAIL: write fail
 */
size_t mdf_espnow_write(mdf_espnow_pkt_type_t type, const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                        const void *data, size_t data_len, TickType_t block_ticks);

/**
 * @brief  deinit espnow in mdf
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_deinit(void);

/**
 * @brief  init espnow in mdf
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_init(void);

/**
 * @brief  enable given type of mdf espnow queue
 *
 * @param  type espnow queue type
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_enable(mdf_espnow_pkt_type_t type);

/**
 * @brief  disable given type of mdf espnow queue
 *
 * @param  type espnow queue type
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_disable(mdf_espnow_pkt_type_t type);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_H__ */
