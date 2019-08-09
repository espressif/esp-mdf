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