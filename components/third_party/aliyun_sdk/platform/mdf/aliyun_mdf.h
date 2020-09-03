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

#ifndef __ALIYUN_MDF_H__
#define __ALIYUN_MDF_H__

#include "aliyun_defs.h"
#include "freertos/FreeRTOS.h"
#include "mdf_err.h"
#include "mdf_event_loop.h"

#define ALIYUN_MDF_PROTOCL (2)

/**
 * @brief Http server data type
 */
typedef union {
    uint32_t value;
    struct {
        aliyun_msg_type_t type : 8; /**<  */
        uint8_t version : 4;        /**< Http body data format */
        uint32_t reserve : 20;      /**< Received */
    } mesh;
} aliyun_mesh_t;

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

bool aliyun_mdf_get_is_gateway(void);

mdf_err_t aliyun_mdf_set_cloud_connect_status(bool status);

bool aliyun_mdf_get_cloud_connect_status(void);

mdf_err_t aliyun_mdf_gateway_write(const uint8_t *dest_addrs, uint32_t dest_addrs_num,
                                   aliyun_msg_type_t type, const void *data, size_t size);

mdf_err_t aliyun_mdf_gateway_read(uint8_t *src_addr, aliyun_msg_type_t *type,
                                  uint8_t *data, size_t *size, TickType_t wait_ticks);

mdf_err_t aliyun_mdf_subdevice_write(aliyun_msg_type_t type, const void *data, size_t size);

mdf_err_t aliyun_mdf_subdevice_read(aliyun_msg_type_t *type,
                                    void **data, size_t *size, TickType_t wait_ticks);

int aliyun_mdf_get_routing_table_size(void);

mdf_err_t aliyun_mdf_get_routing_table(uint8_t *routing_table, int *table_size);

mdf_err_t aliyun_mdf_ota_start(char *ota_version, size_t ota_size);

mdf_err_t aliyun_mdf_ota_write(uint8_t *buffer, size_t length);

mdf_err_t aliyun_mdf_ota_stop(void);

mdf_err_t aliyun_mdf_event_loop_send(mdf_event_loop_t event_id, void *ctx);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_MDF_H__ */
