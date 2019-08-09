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

#ifndef __MALIYUN_LINKKIT_TYPES_PRIV_H__
#define __MALIYUN_LINKKIT_TYPES_PRIV_H__

#include "mdf_common.h"
#include "mwifi.h"

#include "iot_export_linkkit.h"
#include "dev_reset_api.h"

#define MALIYUN_DEFAULT_DEVICE_ID    (-1)
#define MALIYUN_MESH_TYPE            (2)
#define MALIYUN_YIELD_MS             (100)
#define MALIYUN_FOTA_BUF_LEN         (1024)
#define MALIYUN_SEPARATOR            (1)
#define MALIYUN_PAYLOAD_LEN          (MWIFI_PAYLOAD_LEN)
#define MALIYUN_STATUS_SUCC          (true)
#define MALIYUN_STATUS_FAIL          (false)

typedef enum {
    MALIYUN_LINKKIT_PUB_LOGIN,              /**< maliyun linkkit push login message to cloud. */
    MALIYUN_LINKKIT_PUB_LOGOUT,             /**< maliyun linkkit push logout message to cloud. */
    MALIYUN_LINKKIT_PUB_RESET,              /**< maliyun linkkit push reset message to cloud. */
    MALIYUN_LINKKIT_PUB_HEARTBEAT,          /**< maliyun linkkit push heartbeat message to gateway. */
    MALIYUN_LINKKIT_PUB_REPORT,             /**< maliyun linkkit push report message to cloud. */
    MALIYUN_LINKKIT_PUB_QUERY,              /**< maliyun linkkit push query message to cloud. */
    MALIYUN_LINKKIT_PUB_TRIGGEREVENT,       /**< maliyun linkkit push triggerevent message to cloud. */

    MALIYUN_LINKKIT_PUB_TYPE_MAX,           /**< maliyun linkkit push type max */
} maliyun_publish_type_t;

typedef struct {
    int message_id;
    int status;
    int device_id;
    int awss_notify;
    int sub_ota;
    iotx_linkkit_dev_meta_info_t meta_info;
    char version[IOTX_FIRMWARE_VERSION_LEN];
} linkit_login_t;

typedef struct {
    int message_id;
    int device_id;
    int status;
    char product_key[IOTX_PRODUCT_KEY_LEN];
    char device_name[IOTX_DEVICE_NAME_LEN];
} linkit_logout_t;

typedef struct {
    int message_id;
    int device_id;
    int status;
    iotx_linkkit_dev_meta_info_t meta_info;
} linkit_reset_t;

typedef struct {
    int message_id;
    int device_id;
    int message_type;
    size_t payload_len;
    uint8_t payload[0];
} maliyun_linkkit_report_t;

typedef struct {
    int message_id;
    int device_id;
    int message_type;
    int payload_len;
    uint8_t payload[0];
} maliyun_linkkit_query_t;

typedef struct {
    int message_id;
    int device_id;
    int eventid_len;
    int payload_len;
    char eventid_payload[0];
} maliyun_linkkit_trigger_event_t;

typedef struct  {
    int message_id;
    int code;
    int reply_len;
    char reply[0];
} maliyun_linkkit_report_reply_t;

typedef struct {
    int message_id;
    int code;
    int eventid_len;
    int message_len;
    char eventid_message[0];
} maliyun_linkkit_trigger_event_reply_t;

#endif