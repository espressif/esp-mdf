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

#ifndef __ALIYUN_TOPIC_H__
#define __ALIYUN_TOPIC_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

extern const char ALIYUN_TOPIC_SYS_PREFIX[];
extern const char ALIYUN_TOPIC_EXT_SESSION_PREFIX[];
extern const char ALIYUN_TOPIC_EXT_NTP_PREFIX[];
extern const char ALIYUN_TOPIC_EXT_ERROR_PREFIX[];
extern const char ALIYUN_TOPIC_REPLY_SUFFIX[];
extern const char ALIYUN_TOPIC_OTA_DEVICE_INFORM[];
extern const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET[];
extern const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE[];
extern const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET_REPLY[];
extern const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE_REPLY[];
/* From Cloud To Local Request And Response*/
extern const char ALIYUN_TOPIC_THING_MODEL_DOWN_RAW[];
extern const char ALIYUN_TOPIC_THING_MODEL_DOWN_RAW_REPLY[];

/* From Local To Cloud Request And Response*/
extern const char ALIYUN_TOPIC_THING_MODEL_UP_RAW[];
extern const char ALIYUN_TOPIC_THING_MODEL_UP_RAW_REPLY[];

extern const char ALIYUN_TOPIC_RRPC_REQUEST_WILDCARD[];
/* From Cloud To Local Request And Response*/
extern const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET[];
extern const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET_REPLY[];
extern const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_GET[];
extern const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_GET_REPLY[];
extern const char ALIYUN_TOPIC_THING_SERVICE_REQUEST_WILDCARD[];
extern const char ALIYUN_TOPIC_THING_SERVICE_REQUEST_WILDCARD2[];
extern const char ALIYUN_TOPIC_THING_SERVICE_REQUEST[];
extern const char ALIYUN_TOPIC_THING_SERVICE_RESPONSE[];

/* From Local To Cloud Request And Response*/
extern const char ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST[];
extern const char ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST_REPLY[];
extern const char ALIYUN_TOPIC_THING_LOG_POST[];
extern const char ALIYUN_TOPIC_THING_EVENT_POST[];
extern const char ALIYUN_TOPIC_THING_EVENT_POST_REPLY[];
extern const char ALIYUN_TOPIC_THING_EVENT_POST_REPLY_WILDCARD[];
extern const char ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE[];
extern const char ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE_REPLY[];
extern const char ALIYUN_TOPIC_THING_DEVICEINFO_DELETE[];
extern const char ALIYUN_TOPIC_THING_DEVICEINFO_DELETE_REPLY[];
extern const char ALIYUN_TOPIC_THING_DSLTEMPLATE_GET[];
extern const char ALIYUN_TOPIC_THING_DSLTEMPLATE_GET_REPLY[];
extern const char ALIYUN_TOPIC_THING_DYNAMICTSL_GET[];
extern const char ALIYUN_TOPIC_THING_DYNAMICTSL_GET_REPLY[];
extern const char ALIYUN_TOPIC_NTP_REQUEST[];
extern const char ALIYUN_TOPIC_NTP_RESPONSE[];

extern const char ALIYUN_TOPIC_DEV_CORE_SERVICE_DEV[];

/* From Cloud To Local Request And Response*/
extern const char ALIYUN_TOPIC_THING_TOPO_ADD_NOTIFY[];
extern const char ALIYUN_TOPIC_THING_TOPO_ADD_NOTIFY_REPLY[];
extern const char ALIYUN_TOPIC_THING_DISABLE[];
extern const char ALIYUN_TOPIC_THING_DISABLE_REPLY[];
extern const char ALIYUN_TOPIC_THING_ENABLE[];
extern const char ALIYUN_TOPIC_THING_ENABLE_REPLY[];
extern const char ALIYUN_TOPIC_THING_DELETE[];
extern const char ALIYUN_TOPIC_THING_DELETE_REPLY[];
extern const char ALIYUN_TOPIC_THING_GATEWAY_PERMIT[];
extern const char ALIYUN_TOPIC_THING_GATEWAY_PERMIT_REPLY[];

/* From Local To Cloud Request And Response*/
extern const char ALIYUN_TOPIC_THING_SUB_REGISTER[];
extern const char ALIYUN_TOPIC_THING_SUB_REGISTER_REPLY[];
extern const char ALIYUN_TOPIC_THING_SUB_UNREGISTER[];
extern const char ALIYUN_TOPIC_THING_SUB_UNREGISTER_REPLY[];
extern const char ALIYUN_TOPIC_THING_TOPO_ADD[];
extern const char ALIYUN_TOPIC_THING_TOPO_ADD_REPLY[];
extern const char ALIYUN_TOPIC_THING_TOPO_DELETE[];
extern const char ALIYUN_TOPIC_THING_TOPO_DELETE_REPLY[];
extern const char ALIYUN_TOPIC_THING_TOPO_GET[];
extern const char ALIYUN_TOPIC_THING_TOPO_GET_REPLY[];
extern const char ALIYUN_TOPIC_THING_LIST_FOUND[];
extern const char ALIYUN_TOPIC_THING_LIST_FOUND_REPLY[];
extern const char ALIYUN_TOPIC_COMBINE_LOGIN[];
extern const char ALIYUN_TOPIC_COMBINE_LOGIN_REPLY[];
extern const char ALIYUN_TOPIC_COMBINE_LOGOUT[];
extern const char ALIYUN_TOPIC_COMBINE_LOGOUT_REPLY[];

extern const char ALIYUN_TOPIC_OTA_DEVICE_INFORM[];
extern const char ALIYUN_TOPIC_OTA_DEVICE_PROGRESS[];
extern const char ALIYUN_TOPIC_OTA_DEVICE_REQUEST[];
extern const char ALIYUN_TOPIC_OTA_DEVICE_UPGRADE[];

extern const char ALIYUN_TOPIC_THING_CONFIG_GET[];
extern const char ALIYUN_TOPIC_THING_CONFIG_GET_REPLY[];
extern const char ALIYUN_TOPIC_THING_CONFIG_PUSH_REPLY[];
extern const char ALIYUN_TOPIC_THING_CONFIG_PUSH[];

extern const char ALIYUN_TOPIC_THING_RESET[];
extern const char ALIYUN_TOPIC_THING_RESET_REPLY[];

extern const char ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP[];
extern const char ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP_REPLY[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH_REPLY[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_CHECKIN[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_CHECKIN_REPLY[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND[];
extern const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND_REPLY[];
extern const char ALIYUN_TOPIC_THING_CIPHER_GET[];
extern const char ALIYUN_TOPIC_THING_CIPHER_GET_REPLY[];

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SIGN_H__ */
