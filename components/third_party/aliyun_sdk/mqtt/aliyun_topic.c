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


#include "aliyun_topic.h"

const char ALIYUN_TOPIC_SYS_PREFIX[]                         = "/sys/%s/%s/";
const char ALIYUN_TOPIC_EXT_SESSION_PREFIX[]                 = "/ext/session/%s/%s/";
const char ALIYUN_TOPIC_EXT_NTP_PREFIX[]                     = "/ext/ntp/%s/%s/";
const char ALIYUN_TOPIC_EXT_ERROR_PREFIX[]                   = "/ext/error/%s/%s";
const char ALIYUN_TOPIC_REPLY_SUFFIX[]                       = "_reply";

/* From Cloud To Local Request And Response*/
const char ALIYUN_TOPIC_THING_MODEL_DOWN_RAW[]               = "thing/model/down_raw";
const char ALIYUN_TOPIC_THING_MODEL_DOWN_RAW_REPLY[]         = "thing/model/down_raw_reply";

/* From Local To Cloud Request And Response*/
const char ALIYUN_TOPIC_THING_MODEL_UP_RAW[]                 = "thing/model/up_raw";
const char ALIYUN_TOPIC_THING_MODEL_UP_RAW_REPLY[]           = "thing/model/up_raw_reply";

const char ALIYUN_TOPIC_RRPC_REQUEST_WILDCARD[]              = "rrpc/request/+";

/* From Cloud To Local Request And Response*/
const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET[]         = "thing/service/property/set";
const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET_REPLY[]   = "thing/service/property/set_reply";
const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_GET[]         = "thing/service/property/get";
const char ALIYUN_TOPIC_THING_SERVICE_PROPERTY_GET_REPLY[]   = "thing/service/property/get_reply";
const char ALIYUN_TOPIC_THING_SERVICE_REQUEST_WILDCARD[]     = "thing/service/+";
const char ALIYUN_TOPIC_THING_SERVICE_REQUEST_WILDCARD2[]    = "thing/service/#";
const char ALIYUN_TOPIC_THING_SERVICE_REQUEST[]              = "thing/service/%s";
const char ALIYUN_TOPIC_THING_SERVICE_RESPONSE[]             = "thing/service/%s_reply";

const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET[]         = "thing/property/desired/get";
const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE[]      = "thing/property/desired/delete";
const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET_REPLY[]   = "thing/property/desired/get_reply";
const char ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE_REPLY[]   = "thing/property/desired/delete_reply";

/* From Local To Cloud Request And Response*/
const char ALIYUN_TOPIC_THING_LOG_POST[]                     = "thing/log/post";

const char ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST[]          = "thing/event/property/post";
const char ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST_REPLY[]    = "thing/event/property/post_reply";
const char ALIYUN_TOPIC_THING_EVENT_POST[]                   = "thing/event/%s/post";
const char ALIYUN_TOPIC_THING_EVENT_POST_REPLY[]             = "thing/event/%s/post_reply";
const char ALIYUN_TOPIC_THING_EVENT_POST_REPLY_WILDCARD[]    = "thing/event/+/post_reply";
const char ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE[]            = "thing/deviceinfo/update";
const char ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE_REPLY[]      = "thing/deviceinfo/update_reply";
const char ALIYUN_TOPIC_THING_DEVICEINFO_DELETE[]            = "thing/deviceinfo/delete";
const char ALIYUN_TOPIC_THING_DEVICEINFO_DELETE_REPLY[]      = "thing/deviceinfo/delete_reply";
const char ALIYUN_TOPIC_THING_DSLTEMPLATE_GET[]              = "thing/dsltemplate/get";
const char ALIYUN_TOPIC_THING_DSLTEMPLATE_GET_REPLY[]        = "thing/dsltemplate/get_reply";
const char ALIYUN_TOPIC_THING_DYNAMICTSL_GET[]               = "thing/dynamicTsl/get";
const char ALIYUN_TOPIC_THING_DYNAMICTSL_GET_REPLY[]         = "thing/dynamicTsl/get_reply";
const char ALIYUN_TOPIC_NTP_REQUEST[]                        = "request";
const char ALIYUN_TOPIC_NTP_RESPONSE[]                       = "response";

/* From Cloud To Local Request And Response*/
const char ALIYUN_TOPIC_THING_TOPO_ADD_NOTIFY[]              = "thing/topo/add/notify";
const char ALIYUN_TOPIC_THING_TOPO_ADD_NOTIFY_REPLY[]        = "thing/topo/add/notify_reply";
const char ALIYUN_TOPIC_THING_DELETE[]                       = "thing/delete";
const char ALIYUN_TOPIC_THING_DELETE_REPLY[]                 = "thing/delete_reply";
const char ALIYUN_TOPIC_THING_DISABLE[]                      = "thing/disable";
const char ALIYUN_TOPIC_THING_DISABLE_REPLY[]                = "thing/disable_reply";
const char ALIYUN_TOPIC_THING_ENABLE[]                       = "thing/enable";
const char ALIYUN_TOPIC_THING_ENABLE_REPLY[]                 = "thing/enable_reply";
const char ALIYUN_TOPIC_THING_GATEWAY_PERMIT[]               = "thing/gateway/permit";
const char ALIYUN_TOPIC_THING_GATEWAY_PERMIT_REPLY[]         = "thing/gateway/permit_reply";

/* From Local To Cloud Request And Response*/
const char ALIYUN_TOPIC_THING_SUB_REGISTER[]                 = "thing/sub/register";
const char ALIYUN_TOPIC_THING_SUB_REGISTER_REPLY[]           = "thing/sub/register_reply";
const char ALIYUN_TOPIC_THING_SUB_UNREGISTER[]               = "thing/sub/unregister";
const char ALIYUN_TOPIC_THING_SUB_UNREGISTER_REPLY[]         = "thing/sub/unregister_reply";
const char ALIYUN_TOPIC_THING_TOPO_ADD[]                     = "thing/topo/add";
const char ALIYUN_TOPIC_THING_TOPO_ADD_REPLY[]               = "thing/topo/add_reply";
const char ALIYUN_TOPIC_THING_TOPO_DELETE[]                  = "thing/topo/delete";
const char ALIYUN_TOPIC_THING_TOPO_DELETE_REPLY[]            = "thing/topo/delete_reply";
const char ALIYUN_TOPIC_THING_TOPO_GET[]                     = "thing/topo/get";
const char ALIYUN_TOPIC_THING_TOPO_GET_REPLY[]               = "thing/topo/get_reply";
const char ALIYUN_TOPIC_THING_LIST_FOUND[]                   = "thing/list/found";
const char ALIYUN_TOPIC_THING_LIST_FOUND_REPLY[]             = "thing/list/found_reply";
const char ALIYUN_TOPIC_COMBINE_LOGIN[]                      = "combine/login";
const char ALIYUN_TOPIC_COMBINE_LOGIN_REPLY[]                = "combine/login_reply";
const char ALIYUN_TOPIC_COMBINE_LOGOUT[]                     = "combine/logout";
const char ALIYUN_TOPIC_COMBINE_LOGOUT_REPLY[]               = "combine/logout_reply";

const char ALIYUN_TOPIC_OTA_DEVICE_INFORM[]                  = "/ota/device/inform/%s/%s";
const char ALIYUN_TOPIC_OTA_DEVICE_PROGRESS[]                = "/ota/device/progress/%s/%s";
const char ALIYUN_TOPIC_OTA_DEVICE_REQUEST[]                 = "/ota/device/request/%s/%s";
const char ALIYUN_TOPIC_OTA_DEVICE_UPGRADE[]                 = "/ota/device/upgrade/%s/%s";

const char ALIYUN_TOPIC_THING_CONFIG_GET[]                   = "thing/config/get";
const char ALIYUN_TOPIC_THING_CONFIG_GET_REPLY[]             = "thing/config/get_reply";
const char ALIYUN_TOPIC_THING_CONFIG_PUSH_REPLY[]            = "thing/config/push_reply";
const char ALIYUN_TOPIC_THING_CONFIG_PUSH[]                  = "thing/config/push";

const char ALIYUN_TOPIC_THING_RESET[]                        = "thing/reset";
const char ALIYUN_TOPIC_THING_RESET_REPLY[]                  = "thing/reset_reply";

const char ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP[]         = "thing/awss/device/switchap";
const char ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP_REPLY[]   = "thing/awss/device/switchap_reply";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH[]          = "thing/awss/enrollee/match";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH_REPLY[]    = "thing/awss/enrollee/match_reply";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_CHECKIN[]        = "thing/awss/enrollee/checkin";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_CHECKIN_REPLY[]  = "thing/awss/enrollee/checkin_reply";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND[]          = "thing/awss/enrollee/found";
const char ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND_REPLY[]    = "thing/awss/enrollee/found_reply";
const char ALIYUN_TOPIC_THING_CIPHER_GET[]                   = "thing/cipher/get";
const char ALIYUN_TOPIC_THING_CIPHER_GET_REPLY[]             = "thing/cipher/get_reply";