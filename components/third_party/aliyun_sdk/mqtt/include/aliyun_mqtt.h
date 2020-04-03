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

#ifndef __ALIYUN_MQTT_H__
#define __ALIYUN_MQTT_H__

#include "aliyun_defs.h"
#include "aliyun_sign.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum {
    ALIYUN_MQTT_QOS0,
    ALIYUN_MQTT_QOS1,
    ALIYUN_MQTT_QOS2,
} aliyun_mqtt_qos_t;

mdf_err_t aliyun_mqtt_get_connet_status(void);

mdf_err_t aliyun_mqtt_connect(aliyun_mqtt_config_t *mqtt_config);

mdf_err_t aliyun_mqtt_publish(const char *topic, const void *data, size_t len, aliyun_mqtt_qos_t qos);

int aliyun_mqtt_unsubscribe(const char *topic);

int aliyun_mqtt_subscribe(const char *topic, aliyun_mqtt_qos_t qos);

mdf_err_t aliyun_mqtt_disconnet(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SIGN_H__ */
