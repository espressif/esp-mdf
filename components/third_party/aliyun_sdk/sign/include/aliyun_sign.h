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

#ifndef __ALIYUN_SIGN_H__
#define __ALIYUN_SIGN_H__

#include "aliyun_defs.h"
#include "aliyun_host.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/* all secure mode define */
#define ALIYUN_MODE_TLS_GUIDER "-1"
#define ALIYUN_MODE_TLS_DIRECT "2"
#define ALIYUN_MODE_TCP_DIRECT_PLAIN "3"

#ifdef CONFIG_ALIYUN_MQTT_MODE_PRE_AUTH
#define ALIYUN_SECURE_MODE ALIYUN_MODE_TLS_GUIDER
#else
#ifdef CONFIG_ALIYUN_NETWORK_SUPPORT_TLS
#define ALIYUN_SECURE_MODE ALIYUN_MODE_TLS_DIRECT
#else
#define ALIYUN_SECURE_MODE ALIYUN_MODE_TCP_DIRECT_PLAIN
#endif
#endif

#define ALIYUN_SDK_VERSION "3.0.1"
#define ALIYUN_ALINK_VERSION "20"
/* use fixed timestamp */
#define ALIYUN_TIMESTAMP_VALUE "2524608000000"

typedef enum {
    ALIYUN_TRANSPORT_UNKNOWN = 0x0,
    ALIYUN_TRANSPORT_OVER_TCP, /*!< ALIYUN over TCP, using scheme: ``mqtt`` */
    ALIYUN_TRANSPORT_OVER_SSL, /*!< ALIYUN over SSL, using scheme: ``mqtts`` */
    ALIYUN_TRANSPORT_OVER_WS,  /*!< ALIYUN over Websocket, using scheme:: ``ws`` */
    ALIYUN_TRANSPORT_OVER_WSS  /*!< ALIYUN over Websocket Secure, using scheme: ``wss`` */
} aliyun_mqtt_transport_t;

typedef struct {
    char host[ALIYUN_SIGN_HOSTNAME_MAXLEN];
    uint16_t port;
    char client_id[ALIYUN_SIGN_CLIENT_ID_MAXLEN];
    char username[ALIYUN_SIGN_USERNAME_MAXLEN];
    char password[ALIYUN_SIGN_PASSWORD_MAXLEN];
    const char *server_cert;
    aliyun_mqtt_transport_t transport;
    aliyun_device_meta_t *gateway_meta;
} aliyun_mqtt_config_t;

mdf_err_t aliyun_sign_config(const aliyun_device_meta_t *meta, aliyun_sub_sign_info_t *sign_info);

mdf_err_t aliyun_sign_mqtt_config(aliyun_mqtt_region_types_t region, const aliyun_device_meta_t *meta, aliyun_mqtt_config_t *mqtt_config);

mdf_err_t aliyun_auth_register_device(aliyun_mqtt_region_types_t region, const char *product_key, const char *product_secret);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SIGN_H__ */
