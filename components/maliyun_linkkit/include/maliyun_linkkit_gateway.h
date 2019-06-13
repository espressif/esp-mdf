/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#ifndef __MALIYUN_LINKKIT_GATEWAY_H__
#define __MALIYUN_LINKKIT_GATEWAY_H__

#include "mdf_common.h"

/**
 * @brief  maliyun linkkit geteway init
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_gateway_init(void);

/**
 * @brief  maliyun linkkit geteway deinit
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_gateway_deinit(void);

/**
 * @brief  maliyun linkkit geteway refresh subdevice
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_gateway_refresh_subdevice(void);

#endif