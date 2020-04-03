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

#ifndef __ALIYUN_SUBSCRIBE_H__
#define __ALIYUN_SUBSCRIBE_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_subscribe_subdevice_all_topic(aliyun_device_meta_t *meta, aliyun_buffer_t *buffer);

mdf_err_t aliyun_subscribe_gateway_all_topic(aliyun_device_meta_t *gateway_meta, aliyun_buffer_t *buffer);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SUBSCRIBE_H__ */
