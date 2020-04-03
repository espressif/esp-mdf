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


#include "aliyun_host.h"

const char *g_aliyun_mqtt_domain[ALIYUN_MQTT_DOMAIN_NUMBER] = {
    "iot-as-mqtt.cn-shanghai.aliyuncs.com",       /* Shanghai */
    "iot-as-mqtt.ap-southeast-1.aliyuncs.com",    /* Singapore */
    "iot-as-mqtt.ap-northeast-1.aliyuncs.com",    /* Japan */
    "iot-as-mqtt.us-west-1.aliyuncs.com",         /* America */
    "iot-as-mqtt.eu-central-1.aliyuncs.com",      /* Germany */
    NULL,                                         /* Custom */
};

const char *g_aliyun_http_domain[ALIYUN_HTTP_DOMAIN_NUMBER] = {
    "iot-auth.cn-shanghai.aliyuncs.com",         /* Shanghai */
    "iot-auth.ap-southeast-1.aliyuncs.com",      /* Singapore */
    "iot-auth.ap-northeast-1.aliyuncs.com",      /* Japan */
    "iot-auth.us-west-1.aliyuncs.com",           /* America */
    "iot-auth.eu-central-1.aliyuncs.com",        /* Germany */
    NULL,                                        /* Custom */
};
