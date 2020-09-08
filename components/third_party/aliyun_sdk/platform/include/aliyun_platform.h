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

#ifndef __ALIYUN_PLATFORM_H__
#define __ALIYUN_PLATFORM_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
#include "aliyun_mdf.h"
#else
#include "aliyun_idf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_platform_check_json(const char *json);

mdf_err_t aliyun_platform_hmacsha1(const char *msg, int msg_len, char *digest, const char *key, int key_len);

uint32_t aliyun_platform_get_msg_id(void);

int64_t aliyun_platform_get_utc_ms(void);

mdf_err_t aliyun_platform_set_utc_ms(int64_t utc_ms);

mdf_err_t aliyun_platform_get_ip_str(char ip_str[ALIYUN_NETWORK_ADDR_MAXLEN_STR]);

mdf_err_t aliyun_platform_get_device_mac(uint8_t mac[ALIYUN_MAC_ADDRS_MAXLEN]);

mdf_err_t aliyun_platform_get_device_mac_str(char mac_str[ALIYUN_MAC_ADDRS_MAXLEN_STR]);

mdf_err_t aliyun_platform_get_token_str(char token[ALIYUN_TOKEN_ADDR_MAXLEN_STR]);

mdf_err_t aliyun_platform_mqtt_init(void);

mdf_err_t aliyun_platform_mqtt_deinit(void);

mdf_err_t aliyun_platform_mqtt_read(aliyun_buffer_t **mqtt_buf, uint32_t wait_ticks);

mdf_err_t aliyun_platform_mqtt_write(const aliyun_buffer_t *mqtt_buf, uint32_t wait_ticks);

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
#define aliyun_platform_get_is_gateway() aliyun_mdf_get_is_gateway()
#define aliyun_platform_set_cloud_connect_status(status) aliyun_mdf_set_cloud_connect_status(status)
#define aliyun_platform_get_cloud_connect_status() aliyun_mdf_get_cloud_connect_status()
#define aliyun_platform_gateway_read(src_addr, type, data, size, wait_ticks) aliyun_mdf_gateway_read(src_addr, type, data, size, wait_ticks)
#define aliyun_platform_gateway_write(dest_addrs, type, buf, buf_len) aliyun_mdf_gateway_write(dest_addrs, 1, type, buf, buf_len)
#define aliyun_platform_subdevice_read(type, data, size, wait_ticks) aliyun_mdf_subdevice_read(type, data, size, wait_ticks)
#define aliyun_platform_subdevice_write(type, buf, buf_len) aliyun_mdf_subdevice_write(type, buf, buf_len)
#define aliyun_platform_get_routing_table_size() aliyun_mdf_get_routing_table_size()
#define aliyun_platform_get_routing_table(routing_table, table_size) aliyun_mdf_get_routing_table(routing_table, table_size)
#define aliyun_platform_ota_start(ota_version, ota_size) aliyun_mdf_ota_start(ota_version, ota_size)
#define aliyun_platform_ota_write(buffer, length) aliyun_mdf_ota_write(buffer, length)
#define aliyun_platform_ota_stop() aliyun_mdf_ota_stop()
#define aliyun_platform_event_loop_send(event_id, ctx) aliyun_mdf_event_loop_send(event_id, ctx)

#else
#define aliyun_platform_get_is_gateway() (true)
#define aliyun_platform_set_cloud_connect_status(status) aliyun_idf_set_cloud_connect_status(status)
#define aliyun_platform_get_cloud_connect_status() aliyun_idf_get_cloud_connect_status()
#define aliyun_platform_gateway_read(src_addr, type, data, size, wait_ticks) aliyun_idf_gateway_read(src_addr, type, data, size, wait_ticks)
#define aliyun_platform_gateway_write(dest_addrs, type, buf, buf_len) aliyun_idf_gateway_write(dest_addrs, type, buf, buf_len)
#define aliyun_platform_subdevice_read(type, data, size, wait_ticks) aliyun_idf_subdevice_read(type, data, size, wait_ticks)
#define aliyun_platform_subdevice_write(type, buf, buf_len) aliyun_idf_subdevice_write(type, buf, buf_len)
#define aliyun_platform_get_routing_table_size() (0)
#define aliyun_platform_get_routing_table(routing_table, table_size)
#define aliyun_platform_ota_start(ota_version, ota_size) aliyun_idf_ota_start(ota_version, ota_size)
#define aliyun_platform_ota_write(buffer, length) aliyun_idf_ota_write(buffer, length)
#define aliyun_platform_ota_stop() aliyun_idf_ota_stop()
#define aliyun_platform_event_loop_send(event_id, ctx) aliyun_idf_event_post(event_id, ctx)
#endif

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_PLATFORM_H__ */
