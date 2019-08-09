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

#ifndef __MESPNOW_H__
#define __MESPNOW_H__

#include "esp_now.h"
#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MESPNOW_PAYLOAD_LEN  (238)

/**
 * @brief Divide espnnow data into multiple pipes
 */
typedef enum {
    MESPNOW_TRANS_PIPE_DEBUG,      /**< debug packet: log, coredump, espnow_debug config, ack */
    MESPNOW_TRANS_PIPE_CONTROL,    /**< control packet*/
    MESPNOW_TRANS_PIPE_MCONFIG,    /**< network configuration packet */
    MESPNOW_TRANS_PIPE_RESERVED,   /**< reserved for other functiond */
    MESPNOW_TRANS_PIPE_MAX,
} mespnow_trans_pipe_e;

#define MDF_EVENT_MESPNOW_RECV (MDF_EVENT_MESPNOW_BASE + 0x200)
#define MDF_EVENT_MESPNOW_SEND (MDF_EVENT_MESPNOW_BASE + 0x201)

/**
 * @brief  add a peer to espnow peer list based on esp_now_add_peer(...).
 *         It is convenient to use simplified MACRO follows.
 *
 * @param  ifx  Wi-Fi interface that peer uses to send/receive ESPNOW data
 * @param  addr peer mac address
 * @param  lmk  local master key. If lmk is NULL, ESPNOW data that this peer sends/receives isn't encrypted
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_add_peer(wifi_interface_t ifx, const uint8_t *addr, const uint8_t *lmk);

/**
 * @brief  delete a peer from espnow peer list.
 *
 * @param  addr peer mac address
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_del_peer(const uint8_t *addr);

/**
 * @brief  read data from espnow
 *
 * @param  pipe     mespnow packet type
 * @param  src_addr source address
 * @param  data     point to received data buffer
 * @param  *size    A non-zero pointer to the variable holding the length of out_value.
 *                  In case out_value is not zero, will be set to the actual length of the value written.
 * @param  wait_ticks wait time if a packet isn't immediately available
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_read(mespnow_trans_pipe_e pipe, uint8_t *src_addr,
                       void *data, size_t *size, TickType_t wait_ticks);

/**
 * @brief  write date package to espnow.
 *         1. It is necessary to add device to espnow_peer befor send data to dest_addr.
 *         2. When data_len to write is too long, it may fail duration some package and
 *         and the return value is the data len that actually sended.
 *
 * @param  pipe       Pipe of data from espnnow
 * @param  dest_addr  Destination address
 * @param  data       Point to send data buffer
 * @param  size       send data len
 * @param  wait_ticks wait time if a packet isn't immediately available
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_write(mespnow_trans_pipe_e pipe, const uint8_t *dest_addr,
                        const void *data, size_t size, TickType_t wait_ticks);

/**
 * @brief  deinit mespnow
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_deinit(void);

/**
 * @brief  init mespnow
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mespnow_init(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MESPNOW_H__ */
