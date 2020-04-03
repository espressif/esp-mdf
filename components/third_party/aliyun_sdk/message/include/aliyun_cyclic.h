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

#ifndef __ALIYUN_CYCLIC_H__
#define __ALIYUN_CYCLIC_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum {
    ALIYUN_CYCLIC_TYPE_NOTIFY_BIND_TOKEN = 0,
    ALIYUN_CYCLIC_TYPE_NOTIFY_CONNETED_AP,
    ALIYUN_CYCLIC_TYPE_MAX,
} aliyun_cyclic_type_t;

/**
 * @brief Configuration message loop occurs.
 *
 * @param  type    Message type
 * @param  cyclic_num  number of cycles
 * @param  type    Cycle interval
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_cyclic_config(aliyun_cyclic_type_t type, uint32_t cyclic_num, uint32_t delay_ms);

/**
 * @brief Check if the message type needs to be sent
 *
 * @param  type    Message type
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_cyclic_check_type(aliyun_cyclic_type_t type);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_ANSWER_H__ */
