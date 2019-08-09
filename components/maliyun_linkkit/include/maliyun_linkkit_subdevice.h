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

#ifndef __MALIYUN_LINKKIT_SUBDEVICE_H__
#define __MALIYUN_LINKKIT_SUBDEVICE_H__

#include "mdf_common.h"
#include "iot_export_linkkit.h"

#define MALIYUN_LINKKIT_GATEWAY_CONNECT_SUCC         "connect succ"
#define MALIYUN_LINKKIT_GATEWAY_CONNECT_FAIL         "connect fail"
#define MALIYUN_LINKKIT_GATEWAY_DISCONNECTED         "disconnect"

typedef enum {

    MALIYUN_LINKKIT_MSG_POST_PROPERTY                  = ITM_MSG_POST_PROPERTY,                     /**< post property value to cloud */
    MALIYUN_LINKKIT_MSG_DEVICEINFO_UPDATE              = ITM_MSG_DEVICEINFO_UPDATE,                 /**< post device info update message to cloud */
    MALIYUN_LINKKIT_MSG_DEVICEINFO_DELETE              = ITM_MSG_DEVICEINFO_DELETE,                 /**< post device info delete message to cloud */
    MALIYUN_LINKKIT_MSG_POST_RAW_DATA                  = ITM_MSG_POST_RAW_DATA,                     /**< post raw data to cloud */
    MALIYUN_LINKKIT_MSG_QUERY_TIMESTAMP                = ITM_MSG_QUERY_TIMESTAMP,                   /**< query ntp time from cloud */
    MALIYUN_LINKKIT_MSG_REPORT_SUBDEV_FIRMWARE_VERSION = ITM_MSG_REPORT_SUBDEV_FIRMWARE_VERSION,    /**< report subdev's firmware version */

    MALIYUN_LINKKIT_MSG_MAX                            = IOTX_LINKKIT_MSG_MAX,                      /**< linkkit msg max */
} maliyun_linkkit_msg_type_t;

typedef enum {
    MALIYUN_LINKKIT_SUB_AWSS,                       /**< maliyun linkkit subscribe awss message form device.*/
    MALIYUN_LINKKIT_SUB_LOGIN,                      /**< maliyun linkkit subscribe login message form cloud. */
    MALIYUN_LINKKIT_SUB_LOGOUT,                     /**< maliyun linkkit subscribe logout message form cloud. */
    MALIYUN_LINKKIT_SUB_RESET,                      /**< maliyun linkkit subscribe reset message form cloud. */
    MALIYUN_LINKKIT_SUB_GATEWAY_STATUS,             /**< maliyun linkkit subscribe gateway status message form gateway. */
    MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED,            /**< maliyun linkkit subscribe rawdata arrived message form gateway. */
    MALIYUN_LINKKIT_SUB_PROPERTY_SET,               /**< maliyun linkkit subscribe property set message form gateway. */
    MALIYUN_LINKKIT_SUB_REPORT_REPLY,               /**< maliyun linkkit subscribe report reply message form gateway. */
    MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY,        /**< maliyun linkkit subscribe trigger eveny reply message form gateway. */
    MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY,            /**< maliyun linkkit subscribe timestamp reply message form gateway. */

    MALIYUN_LINKKIT_SUB_TYPE_MAX,                   /**< maliyun linkkit subscribe type max */
} maliyun_subscribe_type_t;


/**
 * @brief  maliyun linkkit subdevice init
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_task_init(void);

/**
 * @brief  maliyun linkkit subdevice deinit
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_task_deinit(void);

/**
 * @brief  maliyun linkkit login
 *
 * @param  timeout_ms Timeout period for waiting for a return result after the login information is sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_login(uint32_t timeout_ms);

/**
 * @brief  maliyun linkkit logout
 *
 * @param  timeout_ms Timeout period after waiting for the return result to wait after the exit message is sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_logout(uint32_t timeout_ms);

/**
 * @brief  maliyun linkkit reset
 *
 * @param  timeout_ms Timeout period after waiting for the return result to wait after the exit message is sent.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_reset(uint32_t timeout_ms);

/**
 * @brief  maliyun linkkit report
 *
 * @param  msssage_type maliyun linkkit msssage type.
 * @param  payload Pointer to a sending aliyun linkkit packet.
 * @param  payload_len The length of the payload.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_report(maliyun_linkkit_msg_type_t msssage_type, uint8_t *payload, size_t payload_len);

/**
 * @brief  maliyun linkkit query
 *
 * @param  msssage_type maliyun linkkit msssage type.
 * @param  payload Pointer to a sending aliyun linkkit packet.
 * @param  payload_len The length of the payload.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_query(maliyun_linkkit_msg_type_t msssage_type, uint8_t *payload, size_t payload_len);

/**
 * @brief  maliyun linkkit trigger event
 *
 * @param  eventid The id number of the event.
 * @param  eventid_len The length of the eventid.
 * @param  payload Pointer to a sending aliyun linkkit packet.
 * @param  payload_len The length of the payload.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_trigger_event(char *eventid, size_t eventid_len, char *payload, size_t payload_len);

/**
 * @brief  maliyun linkkit start awss config nerwork
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_start_awss(void);

/**
 * @brief  maliyun linkkit get login status
 *
 * @return
 *    - true
 *    - false
 */
bool maliyun_linkkit_get_login_status(void);

/**
 * @brief  maliyun linkkit set gateway status
 *
 * @param  status Set the status of the gateway.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_set_gateway_status(bool status);

/**
 * @brief  maliyun linkkit get gateway status
 *
 * @return
 *    - true
 *    - false
 */
bool maliyun_linkkit_get_gateway_status(void);

/**
 * @brief  maliyun linkkit subscribe set
 *
 * @param[in]  type maliyun linkkit subscribe callback function type.
 * @param[in]  *func Pointer to the callback function.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t maliyun_linkkit_subscribe_set(const maliyun_subscribe_type_t type, const void *func);

/**
 * @brief  maliyun linkkit awss event callback function
 *
 * @param  event Event type defined in this file
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_awss_cb_t)(int event);

/**
 * @brief  maliyun linkkit login callback function
 *
 * @param  status Login returned status
 * @param  product_key Login to the device's product_key
 * @param  device_name Login to the device's device_name
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_login_cb_t)(const int status, const char *product_key, const char *device_name);

/**
 * @brief  maliyun linkkit logout callback function
 *
 * @param  status Logout returned status
 * @param  product_key Logout to the device's product_key
 * @param  device_name Logout to the device's device_name
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_logout_cb_t)(int status, const char *product_key, const char *device_name);

/**
 * @brief  maliyun linkkit reset callback function
 *
 * @param  status Reset returned status
 * @param  product_key Reset to the device's product_key
 * @param  device_name Reset to the device's device_name
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_reset_cb_t)(int status, const char *product_key, const char *device_name);

/**
 * @brief  maliyun linkkit gateway status callback function
 *
 * @param  status gateway status
 * @param  status_len gateway status size
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_gateway_status_cb_t)(char *status, size_t status_len);

/**
 * @brief  maliyun linkkit rawdata arrived callback function
 *
 * @param  payload rawdata arrived payload
 * @param  payload_len rawdata arrived payload size
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_rawdata_arrived_cb_t)(const unsigned char *payload, size_t payload_len);

/**
 * @brief  maliyun linkkit request set callback function
 *
 * @param  request request data string
 * @param  request_len request data string size
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_propery_set_cb_t)(const char *request, const size_t request_len);

/**
 * @brief  maliyun linkkit report reply callback function
 *
 * @param  msgid Message ID
 * @param  code  Message status code
 * @param  reply Reply data string
 * @param  reply_len Reply data string size
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_report_reply_cb_t)(int msgid, int code, const char *reply, size_t reply_len);

/**
 * @brief  maliyun linkkit trigger event reply callback function
 *
 * @param  msgid Message ID
 * @param  code  Message status code
 * @param  eventid Event id string
 * @param  eventid_len  Event id string size
 * @param  message Message data string
 * @param  message_len  Message data string size
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_trigger_event_reply_cb_t)(int msgid, int code, const char *eventid,
        size_t eventid_len, const char *message, size_t message_len);

/**
 * @brief  maliyun linkkit timestamp reply callback function
 *
 * @param  timestamp Current timestamp string
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*maliyun_linkkit_timestamp_reply_cb_t)(const char *timestamp);

#endif