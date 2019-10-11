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

#ifndef __MLINK_ESPNOW_H__
#define __MLINK_ESPNOW_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MLINK_ESPNOW_PAYLOAD_LEN  (226)
#define MLINK_ESPNOW_COMMUNICATE_UNICAST 0
#define MLINK_ESPNOW_COMMUNICATE_GROUP   1


/**
 * @brief count the number of function parameters
 */
#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)
#define COUNT_PARMS2(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _, ...) _
#define COUNT_PARMS(...)\
    COUNT_PARMS2(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)


/**
 * @brief Mlink espnow configuration
 */
typedef struct {
    uint8_t channel;         /**< The channel on which the device is located */
    uint8_t parent_bssid[6]; /**< Forwarding device address */
} mlink_espnow_config_t;

/**
 * @brief Send a packet
 *
 * @param  addrs_list The address of the final destination of the packet
 * @param  addrs_num  Number of destination addresses
 * @param  data       Pointer to a sending espnow packet
 * @param  size       The length of the data
 * @param  type       The type of the data
 * @param  wait_ticks wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t __mlink_espnow_write(const uint8_t *addrs_list, size_t addrs_num, const void *data,
                               size_t size, uint32_t type, TickType_t wait_ticks);
#define mlink_espnow_write_6(addrs_list,addrs_num,data,size,type,wait_ticks) __mlink_espnow_write(addrs_list,addrs_num,data,size,type,wait_ticks)
#define mlink_espnow_write_5(addrs_list,addrs_num,data,size,wait_ticks) mlink_espnow_write_6(addrs_list,addrs_num,data,size,0,wait_ticks)
#define mlink_espnow_write(...)\
    __PASTE(mlink_espnow_write_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)


/**
 * @brief Receive a packet
 *
 * @param  addrs_list The address of the final source of the packet
 * @param  addrs_num  Number of source addresses
 * @param  data       Pointer to a sending espnow packet
 * @param  size       The length of the data
 * @param  type       The type of the data
 * @param  wait_ticks wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t __mlink_espnow_read(uint8_t **addrs_list, size_t *addrs_num, uint8_t **data,
                              size_t *size, uint32_t *type, TickType_t wait_ticks);
#define mlink_espnow_read_6(addrs_list,addrs_num,data,size,type,wait_ticks) __mlink_espnow_read(addrs_list,addrs_num,data,size,type,wait_ticks)
#define mlink_espnow_read_5(addrs_list,addrs_num,data,size,wait_ticks) mlink_espnow_read_6(addrs_list,addrs_num,data,size,NULL,wait_ticks)
#define mlink_espnow_read(...)\
    __PASTE(mlink_espnow_read_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Initialize the use of ESP-NOW
 *
 * @param  config Mlink espnow initialization configuration.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_espnow_init(mlink_espnow_config_t *config);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_ESPNOW_H__ */
