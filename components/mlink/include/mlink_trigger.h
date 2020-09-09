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
#ifndef __MLINK_TRIGGER_H__
#define __MLINK_TRIGGER_H__

#include "mdf_common.h"
#include "mlink_json.h"
#include "mlink_utils.h"
#include "mlink_notice.h"
#include "mlink_httpd.h"
#include "mlink_handle.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

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
