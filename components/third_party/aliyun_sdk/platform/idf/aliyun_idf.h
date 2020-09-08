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

#ifndef __ALIYUN_IDF_H__
#define __ALIYUN_IDF_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_idf_set_cloud_connect_status(bool status);

bool aliyun_idf_get_cloud_connect_status(void);

mdf_err_t aliyun_idf_gateway_init(void);

mdf_err_t aliyun_idf_gateway_deinit(void);

mdf_err_t aliyun_idf_gateway_write(const uint8_t *dest_addrs,
                                   aliyun_msg_type_t type, const void *data, size_t size);

mdf_err_t aliyun_idf_gateway_read(uint8_t *src_addr, aliyun_msg_type_t *type,
                                  uint8_t *data, size_t *size, uint32_t wait_ticks);

mdf_err_t aliyun_idf_subdevice_read(aliyun_msg_type_t *type,
                                    void **data, size_t *size, uint32_t wait_ticks);

mdf_err_t aliyun_idf_subdevice_write(aliyun_msg_type_t type,
                                     const void *data, size_t size);

mdf_err_t aliyun_idf_ota_start(char *ota_version, size_t ota_size);

mdf_err_t aliyun_idf_ota_write(uint8_t *buffer, size_t length);

mdf_err_t aliyun_idf_ota_stop(void);

mdf_err_t aliyun_idf_event_post(uint32_t event_id, void *ctx);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_IDF_H__ */
