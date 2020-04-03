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

#ifndef __ALIYUN_OTA_H__
#define __ALIYUN_OTA_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

typedef struct aliyun_ota_info {
    char url[ALIYUN_OTA_URL_MAXLEN];
    uint32_t size;
    char version[ALIYUN_FIRMWARE_VERSION_MAXLEN];
    char sign[ALIYUN_SIGN_MAXLEN_STR];
    char sign_method[ALIYUN_SIGN_METHOD_MAXLEN];
} aliyun_ota_info_t;

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_ota_init(const char *product_key, const char *device_name, aliyun_ota_info_t *ota_info);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_OTA_H__ */
