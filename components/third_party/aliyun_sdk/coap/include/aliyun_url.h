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

#ifndef __ALIYUN_URL_H__
#define __ALIYUN_URL_H__

#include "aliyun_defs.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

extern const char ALIYUN_URL_SYS_AWSS_EVENT_CONNECTAP_NOTIFY[];
// extern const char ALIYUN_URL_SYS_AWSS_DEVICE_CONNECTAP_INFO_GET[];
#define ALIYUN_URL_SYS_AWSS_DEVICE_CONNECTAP_INFO_GET       "sys/awss/device/connectap/info/get"

#define ALIYUN_URL_SYS_DEVICE_INFO_NOTIFY                    "sys/device/info/notify"
#define ALIYUN_URL_SYS_DEVICE_INFO_GET                       "sys/device/info/get"

extern const char ALIYUN_URL_DEV_CORE_SERVICE_DEV[];

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_URL_H__ */
