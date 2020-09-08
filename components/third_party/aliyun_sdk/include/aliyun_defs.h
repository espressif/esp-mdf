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

#ifndef __ALIYUN_DEFS_H__
#define __ALIYUN_DEFS_H__

#include "sdkconfig.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define ALIYUN_PRODUCT_KEY_MAXLEN (20 + 1)
#define ALIYUN_PRODUCT_SECRET_MAXLEN (64 + 1)
#define ALIYUN_DEVICE_NAME_MAXLEN (32 + 1)
#define ALIYUN_DEVICE_SECRET_MAXLEN (64 + 1)

#define ALIYUN_FIRMWARE_VERSION_MAXLEN (32 + 1)
#define ALIYUN_TIMESTAMP_MAXLEN (32 + 1)
#define ALIYUN_SIGN_MAXLEN (32)
#define ALIYUN_SIGN_METHOD_MAXLEN (20 + 1)
#define ALIYUN_SIGN_MAXLEN_STR (64 + 1)

#define ALIYUN_OTA_URL_MAXLEN (256 + 1)
#define ALIYUN_OTA_PAYLOAD_MAXLEN (1024)
#define ALIYUN_SUBDEVICE_ADDRS_MAXLEN (6)
#define ALIYUN_MAC_ADDRS_MAXLEN (6)
#define ALIYUN_MAC_ADDRS_MAXLEN_STR (17 + 1) /* MAC地址的长度 */
#define ALIYUN_NETWORK_ADDR_MAXLEN_STR (15 + 1)
#define ALIYUN_TOKEN_ADDR_MAXLEN_STR (32 + 1)

#define ALIYUN_SIGN_SOURCE_MAXLEN (256 + 1)
#define ALIYUN_SIGN_CLIENT_ID_MAXLEN (128 + 1)
#define ALIYUN_SIGN_HOSTNAME_MAXLEN (64 + 1)
#define ALIYUN_SIGN_USERNAME_MAXLEN (64 + 1)
#define ALIYUN_SIGN_PASSWORD_MAXLEN (64 + 1)

#define ALIYUN_SIGN_DEVICEID_MAXLEN (ALIYUN_PRODUCT_KEY_MAXLEN + ALIYUN_DEVICE_NAME_MAXLEN)

#define ALIYUN_SUCC_CODE (200)

#define ALIYUN_MESH_PROTOCOL_VERSION (0)

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
#define ALIYUN_EVENT_BASE (0x7000)
#define ALIYUN_EVENT_ONLINE (ALIYUN_EVENT_BASE + 0)
#define ALIYUN_EVENT_OFFLINE (ALIYUN_EVENT_BASE + 1)
#else
#include "esp_event.h"
ESP_EVENT_DECLARE_BASE(ALIYUN_EVENT_BASE);
#define ALIYUN_EVENT_ONLINE (0)
#define ALIYUN_EVENT_OFFLINE (1)
#endif

typedef struct {
    char product_key[ALIYUN_PRODUCT_KEY_MAXLEN];
    char device_name[ALIYUN_DEVICE_NAME_MAXLEN];
    char device_secret[ALIYUN_DEVICE_SECRET_MAXLEN];
} aliyun_device_meta_t;

typedef struct {
    char product_key[ALIYUN_PRODUCT_KEY_MAXLEN];
    char device_name[ALIYUN_DEVICE_NAME_MAXLEN];
    int status;
} aliyun_device_reply_t;

typedef struct {
    char product_key[ALIYUN_PRODUCT_KEY_MAXLEN];
    char device_name[ALIYUN_DEVICE_NAME_MAXLEN];
    char client_id[ALIYUN_SIGN_CLIENT_ID_MAXLEN];
    char timestamp[ALIYUN_TIMESTAMP_MAXLEN];
    char sign_method[ALIYUN_SIGN_METHOD_MAXLEN];
    char sign[ALIYUN_SIGN_PASSWORD_MAXLEN];
} aliyun_sub_sign_info_t;

typedef enum {
    ALIYUN_OTA_TYPE_NOT,
    ALIYUN_OTA_TYPE_START,
} aliyun_ota_type_t;

typedef struct {
    char *topic;
    size_t topic_len;
    uint8_t *payload;
    size_t payload_len;
    uint8_t data[0];
} aliyun_buffer_t;

#pragma pack(1)
typedef enum {
    ALIYUN_MSG_TYPE_NULL,

    ALIYUN_GATEWAY_ADD_SUBDEVICE,       /**< aliyun publish login message to cloud. */
    ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY, /**< aliyun publish login message to cloud. */

    ALIYUN_SUBDEVICE_HEARTBEAT,       /**< aliyun publish heartbeat message to gateway. */
    ALIYUN_SUBDEVICE_HEARTBEAT_REPLY, /**< aliyun publish login message to cloud. */

    ALIYUN_GATEWAY_DELETE_SUBDEVICE,       /**< aliyun publish logout message to cloud. */
    ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY, /**< aliyun publish login message to cloud. */

    ALIYUN_GATEWAY_GET_SUBDEVICE_INFO,       /**< aliyun publish heartbeat message to gateway. */
    ALIYUN_GATEWAY_GET_SUBDEVICE_INFO_REPLY, /**< aliyun publish login message to cloud. */

    ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_STATUS,       /**< aliyun publish heartbeat message to gateway. */
    ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_STATUS_REPLY, /**< aliyun publish login message to cloud. */

    ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET,       /**< aliyun publish heartbeat message to gateway. */
    ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET_REPLY, /**< aliyun publish login message to cloud. */

    /**< device send info notify. */
    ALIYUN_COAP_SYS_NOTIFY_DEVICE_INFO,    /**< device publish heartbeat message to app. */
    ALIYUN_COAP_SYS_GET_DEVICE_INFO,       /**< app publish heartbeat message to device. */
    ALIYUN_COAP_SYS_GET_DEVICE_INFO_REPLY, /**< app publish heartbeat message to device. */

    /**< device send connectap notify. */
    ALIYUN_COAP_SYS_EVENT_CONNECTAP_NOTIFY,    /**< aliyun publish login message to cloud. */
    ALIYUN_COAP_SYS_EVENT_CONNECTAP_GET,       /**< aliyun publish login message to cloud. */
    ALIYUN_COAP_SYS_EVENT_CONNECTAP_GET_REPLY, /**< aliyun publish login message to cloud. */

    /**< Unbind the device from the app. */
    ALIYUN_MQTT_PUBLISH_RESET,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_RESET_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Device switching AP. */                    //TODO
    ALIYUN_MQTT_SUBSCRIBE_AWSS_DEVICE_SWITCHAP,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_AWSS_DEVICE_SWITCHAP_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Update the token required to bind the device to the app. */
    ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_MATCH,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_MATCH_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Report the device information scanned around. */ //TODO
    ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_FOUND,              /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_FOUND_REPLY,      /**< Uplink（Alink JSON）response topic. */

    /**< Deliver network access request. */          //TODO
    ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_CHECKIN,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_CHECKIN_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Get the generated key. */          //TODO
    ALIYUN_MQTT_PUBLISH_CIPHER_GET,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_CIPHER_GET_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Allow the gateway to add child devices. */
    ALIYUN_MQTT_SUBSCRIBE_GATEWAY_PERMIT,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_GATEWAY_PERMIT_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Sub-device dynamic registration. */
    ALIYUN_MQTT_PUBLISH_SUB_REGISTER,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_SUB_REGISTER_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Sub-device dynamic unregistration. */
    ALIYUN_MQTT_PUBLISH_SUB_UNREGISTER,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_SUB_UNREGISTER_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Add topology relationships between devices. */
    ALIYUN_MQTT_PUBLISH_ADD_TOPO,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Delete the topology relationship between devices. */
    ALIYUN_MQTT_PUBLISH_DELETE_TOPO,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_DELETE_TOPO_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Get the topology relationship of the device. */ //TODO
    ALIYUN_MQTT_PUBLISH_GET_TOPO,                        /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_GET_TOPO_REPLY,                /**< Uplink（Alink JSON）response topic. */

    /**< Found a list of devices reported. */ //TODO
    ALIYUN_MQTT_PUBLISH_FOUND_LIST,           /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_FOUND_LIST_REPLY,   /**< Uplink（Alink JSON）response topic. */

    /**< Inform the gateway to add a topology relationship. */ //TODO
    ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_NOTIFY,                     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_ADD_TOPO_NOTIFY_REPLY,                 /**< Downlink（Alink JSON）response topic. */

    /**< Disable sub-device. */
    ALIYUN_MQTT_SUBSCRIBE_DISABLE,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_DISABLE_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Restore sub-device. */
    ALIYUN_MQTT_SUBSCRIBE_ENABLE,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_ENABLE_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Delete sub-device. */
    ALIYUN_MQTT_SUBSCRIBE_DELETE,     /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_PUBLISH_DELETE_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< Sub-device login. */
    ALIYUN_MQTT_PUBLISH_SESSION_COMBINE_LOGIN,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGIN_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Sub-device logout. */
    ALIYUN_MQTT_PUBLISH_SESSION_COMBINE_LOGOUT,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGOUT_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Sub-device offline and send message error. */
    ALIYUN_MQTT_SUBSCRIBE_EXT_ERROR, /**< Downlink（Alink JSON）notify topic. */

    /**< Up raw data */
    ALIYUN_MQTT_PUBLISH_MODEL_UP_RAW,         /**< Uplink（RAW）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY, /**< Uplink（RAW）response topic. */

    /**< Down raw data */
    ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW,     /**< Downlink（RAW）request topic. */
    ALIYUN_MQTT_PUBLISH_MODEL_DOWN_RAW_REPLY, /**< Downlink（RAW）response topic. */

    /**< The device reports attribute information. */
    ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_POST, /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_POST_PROPERTY_REPLY,         /**< Uplink（Alink JSON）response topic. Cloud response to device post property */

    /**< Cloud delivery device attributes. */
    ALIYUN_MQTT_PROPERTY_SET,                       /**< Downlink（Alink JSON）request topic. Cloud set device's property */
    ALIYUN_MQTT_PUBLISH_SET_SERVICE_PROPERTY_REPLY, /**< Downlink（Alink JSON）request topic. */

    /**< Equipment event report. */
    ALIYUN_MQTT_PUBLISH_EVENT_POST, /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_POST_EVENT_REPLY,   /**< Downlink（Alink JSON）response topic. Cloud reponse to device post event*/

    /**< Cloud service control. */
    ALIYUN_MQTT_SERVICE_INVOKE,           /**< Downlink（Alink JSON）request topic. Cloud invoke device's service */
    ALIYUN_MQTT_PUBLISH_RESPONSE_SERVICE, /**< Downlink（Alink JSON）response topic. */

    /**< The gateway reports device information in batches. */
    ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_PACK_POST,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_EVENT_PROPERTY_PACK_POST_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Device add attribute tag. */
    ALIYUN_MQTT_PUBLISH_UPDATE_DEVICEINFO, /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY,   /**< Uplink（Alink JSON）response topic. Cloud response to device update deviceinfo */

    /**< Device delete attribute tag. */
    ALIYUN_MQTT_PUBLISH_DELETE_DEVICEINFO, /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY,   /**< Uplink（Alink JSON）response topic. Cloud response to device delete deviceinfo */

    /**< The device actively requests configuration information. */
    ALIYUN_MQTT_PUBLISH_GET_CONFIG, /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_GET_CONFIG_REPLY,   /**< Uplink（Alink JSON）response topic. Cloud reponse to device get configuration */

    /**< The cloud actively pushes configuration information. */
    ALIYUN_MQTT_CONFIG_PUSH,               /**< Downlink（Alink JSON）request topic. Cloud push configuration to device */
    ALIYUN_MQTT_PUBLISH_CONFIG_PUSH_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< The cloud configuration is delivered. */
    ALIYUN_MQTT_SUBSCRIBE_MODEL_CONFIG_PUSH, /**< Downlink（Alink JSON）request topic. */

    /**< The device reports the firmware version. */
    ALIYUN_MQTT_PUBLISH_OTA_DEVICE_INFORM, /**< Uplink（Alink JSON）. */

    /**< Cloud push ota update information. */
    ALIYUN_MQTT_SUBSCRIBE_OTA_DEVICE_UPGRADE, /**< Downlink（Alink JSON）. */

    /**< The device reports ota progress information. */
    ALIYUN_MQTT_PUBLISH_OTA_DEVICE_PROGRESS, /**< Uplink（Alink JSON）. */

    /**< The device actively requests ota information. */
    ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST, /**< Uplink（Alink JSON）. */

    /**< Get the ntp timestamp. */
    ALIYUN_MQTT_PUBLISH_NTP_REQUEST,  /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_REQUEST_NTP_RESPONSE, /**< Uplink（Alink JSON）response topic. Used for receiving cloud response of ntp request*/

    /**< Get tsl template. */
    ALIYUN_MQTT_PUBLISH_GET_DSLTEMPLATE,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Acquisition model. */
    ALIYUN_MQTT_PUBLISH_GET_DYNAMICTSL,         /**< Downlink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_GET_DYNAMICTSL_REPLY, /**< Downlink（Alink JSON）response topic. */

    /**< The device requests the cloud to get the expected value. */
    ALIYUN_MQTT_PUBLISH_GET_DESIRED_PROPERTY,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_GET_DESIRED_PROPERTY_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< Device clears the cloud to get the expected value attribute. */
    ALIYUN_MQTT_PUBLISH_DELETE_DESIRED_PROPERTY,         /**< Uplink（Alink JSON）request topic. */
    ALIYUN_MQTT_SUBSCRIBE_DELETE_DESIRED_PROPERTY_REPLY, /**< Uplink（Alink JSON）response topic. */

    /**< RRPC system topic. */
    ALIYUN_MQTT_SUBSCRIBE_RRPC_REQUEST_WILDCARD, /**< Synchronize （Alink JSON） request topic. */
    ALIYUN_MQTT_PUBLISH_RRPC_REQUEST_WILDCARD,   /**< Synchronize （Alink JSON） request topic. */
    ALIYUN_MQTT_PUBLISH_RRPC_RESPONSE_WILDCARD,  /**< Synchronize （Alink JSON） response topic. */

    /**< Report log information. */
    ALIYUN_MQTT_PUBLISH_POST_LOG, /**< Uplink （Alink JSON） request topic. */

    ALIYUN_PARENT_OFFLINE, /**< parent is offline. */

    ALIYUN_MSG_TYPE_MAX, /**< aliyun msg type max */
} aliyun_msg_type_t;
#pragma pack()

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_DEFS_H__ */
