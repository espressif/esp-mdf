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

#ifndef __ALIYUN_COAP_H__
#define __ALIYUN_COAP_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief Send coap message.
 *
 * @param  coap_url   Destination url sent
 * @param  data  Pointer to a sends data
 * @param  size  sends data size.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_coap_send_app(const char *coap_url, const uint8_t *data, const size_t size);

/**
 * @brief coap service init.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_coap_service_init();

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_COAP_H__ */
