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

#ifndef __ALIYUN_PUBLISH_H__
#define __ALIYUN_PUBLISH_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_publish_sub_topo_add(const char *product_key, const char *device, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_sub_topo_delete(const char *product_key, const char *device, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_sub_combine_login(const char *product_key, const char *device, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_sub_combine_logout(const char *product_key, const char *device, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_awss_enrollee_match(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_reset(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_cipher_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_event_property_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_service_property_set_reply(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_event_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_service_response(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_deviceinfo_update(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_deviceinfo_delete(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_config_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_config_push_reply(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_ota_device_inform(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_ota_device_request(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_ota_device_progress(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_ntp_request(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_dsltemplate_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_property_desired_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_property_desired_delete(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

mdf_err_t aliyun_publish_thing_log_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_PUBLISH_H__ */
