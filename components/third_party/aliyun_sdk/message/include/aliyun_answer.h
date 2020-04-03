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

#ifndef __ALIYUN_ANSWER_H__
#define __ALIYUN_ANSWER_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

//改为可配置
#define ALIYUN_ANSWER_DEFAULT_RESENDS_NUM 3
#define ALIYUN_ANSWER_DEFAULT_TIMEOUT_MS 10000

typedef enum {
    ALIYUN_ANSWER_STATUS_SUCC,
    ALIYUN_ANSWER_STATUS_FAIL,
    ALIYUN_ANSWER_STATUS_WAIT,
    ALIYUN_ANSWER_STATUS_TIMEOUT,
} aliyun_answer_status_t;

typedef enum {
    ALIYUN_ANSWER_TYPE_UPDATE_TOKEN,
    ALIYUN_ANSWER_TYPE_UPDATE_RESET,
    ALIYUN_ANSWER_TYPE_CIPHER_GET,
    ALIYUN_ANSWER_TYPE_MAX,
} aliyun_answer_type_t;

/**
 * @brief Set the message ID of this message type to wait for a response.
 *
 * @param  type    Message type
 * @param  msg_id  Message id
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_set_wait(aliyun_answer_type_t type, uint32_t msg_id);

/**
 * @brief Set the message ID of this message type to be successful.
 *
 * @param  type    Message type
 * @param  msg_id  Message id
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_set_success(aliyun_answer_type_t type, uint32_t msg_id);

/**
 * @brief Set the message ID of this message type to fail.
 *
 * @param  type    Message type
 * @param  msg_id  Message id
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_set_fail(aliyun_answer_type_t type, uint32_t msg_id);

/**
 * @brief Check if this message type needs to be resent.
 *
 * @param  type    Message type
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_check_resends(aliyun_answer_type_t type);

/**
 * @brief Set to resend this message type.
 *
 * @param  type    Message type
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_set_resends_flag(aliyun_answer_type_t type);

/**
 * @brief Check if the message ID of this message type times out.
 *
 * @param  type    Message type
 * @param  msg_id  Message id
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_check_timeout(aliyun_answer_type_t type, uint32_t msg_id);

/**
 * @brief Refresh response timing with the latest timestamp.
 *
 * @param  utc_ms time
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_answer_update_utc_ms(int64_t utc_ms);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_ANSWER_H__ */
