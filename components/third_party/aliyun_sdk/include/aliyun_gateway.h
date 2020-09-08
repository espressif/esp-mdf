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

#ifndef __ALIYUN_GATEWAY_H__
#define __ALIYUN_GATEWAY_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#define ALIYUN_PROCESS_SIZE (10) /**< Maximum handle size once process*/

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  aliyun geteway init
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_gateway_init(void);

/**
 * @brief  aliyun geteway deinit
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_gateway_deinit(void);

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
/**
 * @brief  aliyun geteway refresh subdevice
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_gateway_refresh_subdevice(void);
#endif

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_GATEWAY_H__ */
