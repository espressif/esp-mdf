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

#ifndef __LIGHT_HANDLE_H__
#define __LIGHT_HANDLE_H__

#include "mdf_common.h"
#include "mwifi.h"
#include "mlink.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The value of the cid corresponding to each attribute of the light
 */
enum light_cid {
    LIGHT_CID_STATUS            = 0,
    LIGHT_CID_HUE               = 1,
    LIGHT_CID_SATURATION        = 2,
    LIGHT_CID_VALUE             = 3,
    LIGHT_CID_COLOR_TEMPERATURE = 4,
    LIGHT_CID_BRIGHTNESS        = 5,
    LIGHT_CID_MODE              = 6,
};

enum light_status {
    LIGHT_STATUS_OFF               = 0,
    LIGHT_STATUS_ON                = 1,
    LIGHT_STATUS_SWITCH            = 2,
    LIGHT_STATUS_HUE               = 3,
    LIGHT_STATUS_BRIGHTNESS        = 4,
    LIGHT_STATUS_COLOR_TEMPERATURE = 5,
};

/**
 * @brief Show layer of node by light.
 *
 * @param handle_data pointer of mlink_handle_data_t
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t light_show_layer(mlink_handle_data_t *handle_data);

/**
 * @brief Set light characteristic
 *
 * @param cid characteristic id
 * @param arg arguments
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t mlink_set_value(uint16_t cid, void *arg);

/**
 * @brief Get light characteristic
 *
 * @param cid characteristic id
 * @param arg arguments
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t mlink_get_value(uint16_t cid, void *arg);

#ifdef __cplusplus
}
#endif

#endif /**< __LIGHT_HANDLE_H__ */