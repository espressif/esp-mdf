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

#ifndef __MLINK_NOTICE_H__
#define __MLINK_NOTICE_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  Initialize mlink mdns and udp
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mlink_notice_init();

/**
 * @brief  Deinitialize mlink mdns and udp
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mlink_notice_deinit();

/**
 * @brief Inform Mesh-App to initiate a request
 *
 * @param  message Type of message requested
 * @param  size    Length of the message
 * @param  addr    Address of the device to be requested
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t mlink_notice_write(const char *message, size_t size, const uint8_t *addr);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_NOTICE_H__ */
