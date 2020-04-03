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

#ifndef __ALIYUN_PACK_H__
#define __ALIYUN_PACK_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define ALIYUN_MSG_KEY_ID "id"
#define ALIYUN_MSG_KEY_VERSION "version"
#define ALIYUN_MSG_KEY_METHOD "method"
#define ALIYUN_MSG_KEY_PARAMS "params"
#define ALIYUN_MSG_KEY_CODE "code"
#define ALIYUN_MSG_KEY_DATA "data"
#define ALIYUN_MSG_KEY_MESSAGE "message"

#define ALIYUN_MSG_VERSION "1.0"

#define ALIYUN_MSG_KEY_PRODUCT_KEY "productKey"
#define ALIYUN_MSG_KEY_PRODUCT_SECRET "productSecret"
#define ALIYUN_MSG_KEY_DEVICE_NAME "deviceName"
#define ALIYUN_MSG_KEY_DEVICE_SECRET "deviceSecret"

#define ALIYUN_SIGN_METHOD_SHA256 "Sha256"
#define ALIYUN_SIGN_METHOD_HMACMD5 "hmacMd5"
#define ALIYUN_SIGN_METHOD_HMACSHA1 "hmacSha1"
#define ALIYUN_SIGN_METHOD_HMACSHA256 "hmacSha256"

char *aliyun_pack_coap_connectapp_notify(uint32_t msg_id);

char *aliyun_pack_coap_connectapp_info_get(uint32_t msg_id);

char *aliyun_pack_coap_device_info_notify(uint32_t msg_id);

char *aliyun_pack_coap_device_info_get(uint32_t msg_id);

char *aliyun_pack_thing_sub_topo_add(uint32_t msg_id, const aliyun_device_meta_t *sub_meta);

char *aliyun_pack_thing_sub_topo_delete(uint32_t msg_id, const aliyun_device_meta_t *sub_meta);

char *aliyun_pack_thing_sub_combine_login(uint32_t msg_id, const aliyun_device_meta_t *sub_meta);

char *aliyun_pack_thing_sub_combine_logout(uint32_t msg_id, const aliyun_device_meta_t *sub_meta);

char *aliyun_pack_thing_awss_enrollee_match(uint32_t msg_id);

char *aliyun_pack_thing_reset(uint32_t msg_id);

char *aliyun_pack_thing_cipher_get(uint32_t msg_id);

char *aliyun_pack_thing_event_property_post(uint32_t id, const char *params);

char *aliyun_pack_thing_event_property_pack_post(uint32_t msg_id, const char *params);

char *aliyun_pack_thing_event_identifier_post(uint32_t id, const char *identifier, const char *params);

char *aliyun_pack_thing_id_code_reply(uint32_t msg_id, int code, const char *identifier, const char *data);

char *aliyun_pack_thing_event_deviceinfo_update(uint32_t id, const char *params);

char *aliyun_pack_thing_event_deviceinfo_delete(uint32_t id, const char *params);

char *aliyun_pack_ota_device_request(uint32_t id, const char *version);

char *aliyun_pack_ota_device_inform(uint32_t id, const char *version);

char *aliyun_pack_ota_device_progress(uint32_t id, int step, const char *desc);

char *aliyun_pack_thing_dsltemplate_get(uint32_t id);

char *aliyun_pack_thing_event_property_desired_get(uint32_t id, const char *params);

char *aliyun_pack_thing_event_property_desired_delete(uint32_t id, const char *params);

char *aliyun_pack_thing_config_get(uint32_t id);

char *aliyun_pack_ntp_request(int64_t timestamp);

char *aliyun_pack_thing_log_post(uint32_t id, const char *params);

char *aliyun_pack_thing_update_sdk_version(uint32_t msg_id, const char *sdk_version);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_COAP_H__ */
