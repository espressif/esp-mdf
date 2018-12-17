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
