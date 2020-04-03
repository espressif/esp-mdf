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

#ifndef __ALIYUN_PARSE_H__
#define __ALIYUN_PARSE_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#define ALIYUN_DEVICE_NAME_MAXLEN_STR "32"
#define ALIYUN_PRODUCT_KEY_MAXLEN_STR "20"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

mdf_err_t aliyun_parse_thing_topo_add_reply(const char *data, int data_len);

mdf_err_t aliyun_parse_thing_topo_delete_reply(const char *data, int data_len);

mdf_err_t aliyun_parse_session_login_reply(const char *data, int data_len);

mdf_err_t aliyun_parse_session_logout_reply(const char *data, int data_len);

mdf_err_t aliyun_parse_thing_reset_reply(const char *data, uint32_t *msg_id, uint32_t *code);

mdf_err_t aliyun_parse_thing_awss_enrollee_match_reply(const char *data, uint32_t *msg_id, uint32_t *code);

mdf_err_t aliyun_parse_gateway_reset_notify(const char *data);

mdf_err_t aliyun_parse_thing_event_property_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_model_up_raw_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_proprrty_set(const char *data, size_t data_len, uint32_t *msg_id, char **params);

mdf_err_t aliyun_parse_thing_deviceinfo_update_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_deviceinfo_delete_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_config_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_config_push_reply(const char *data, size_t data_len, uint32_t *msg_id, char **params);

mdf_err_t aliyun_parse_thing_event_post_identifier(const char *data, size_t data_len, char **identifier);

mdf_err_t aliyun_parse_thing_event_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_service_request(const char *data, size_t data_len, uint32_t *msg_id, char **identifier, char **params);

mdf_err_t aliyun_parse_thing_event_pack_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_ntp_response(const char *data, size_t data_len);

mdf_err_t aliyun_parse_thing_dsltemplate_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_property_desired_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_thing_property_desired_delete_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params);

mdf_err_t aliyun_parse_mqtt_data(char *topic, int topic_len, char *data, int data_len);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_PARSE_H__ */
