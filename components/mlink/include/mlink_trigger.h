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

#include "mdf_common.h"
#include "mlink_json.h"
#include "mlink_utils.h"
#include "mlink_notice.h"
#include "mlink_httpd.h"
#include "mlink_handle.h"

#ifndef __MLINK_TRIGGER_H__
#define __MLINK_TRIGGER_H__

/**
 * @brief The method of sending the package when the event is triggered
 */
typedef enum {
    MLINK_COMMUNICATE_NONE,   /**< Invalid transmission method */
    MLINK_COMMUNICATE_MESH,   /**< Transmission using MESH */
    MLINK_COMMUNICATE_ESPNOW, /**< Transmission using ESPNOW */
} mlink_communicate_t;

/**
 * @brief Initialize event handler
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_trigger_init();

/**
 * @brief Whether the handler exists
 *
 * @return [description]
 */
bool mlink_trigger_is_exist();

/**
 * @brief Whether the query event is triggered
 *
 * @param  communicate The method of sending the package when the event is triggered
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_trigger_handle(mlink_communicate_t communicate);

/**
 * @brief Add event handler
 *
 * @param  trigger_raw_data [description]
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_trigger_add(const char *trigger_raw_data);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_TRIGGER_H__ */
