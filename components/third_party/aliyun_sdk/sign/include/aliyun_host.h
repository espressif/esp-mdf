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

#ifndef __ALIYUN_HOST_H__
#define __ALIYUN_HOST_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum {
    ALIYUN_MQTT_REGION_SHANGHAI,  /* Shanghai */
    ALIYUN_MQTT_REGION_SINGAPORE, /* Singapore */
    ALIYUN_MQTT_REGION_JAPAN,     /* Japan */
    ALIYUN_MQTT_REGION_USA_WEST,  /* America */
    ALIYUN_MQTT_REGION_GERMANY,   /* Germany */
    ALIYUN_MQTT_REGION_CUSTOM,    /* Custom setting */
    ALIYUN_MQTT_DOMAIN_MAX        /* Maximum number of domain */
} aliyun_mqtt_region_types_t;

typedef enum {
    ALIYUN_HTTP_REGION_SHANGHAI,  /* Shanghai */
    ALIYUN_HTTP_REGION_SINGAPORE, /* Singapore */
    ALIYUN_HTTP_REGION_JAPAN,     /* Japan */
    ALIYUN_HTTP_REGION_USA_WEST,  /* America */
    ALIYUN_HTTP_REGION_GERMANY,   /* Germany */
    ALIYUN_HTTP_REGION_CUSTOM,    /* Custom setting */
    ALIYUN_HTTP_DOMAIN_MAX        /* Maximum number of domain */
} aliyun_http_region_types_t;

#define ALIYUN_MQTT_DOMAIN_NUMBER (ALIYUN_MQTT_DOMAIN_MAX)
#define ALIYUN_HTTP_DOMAIN_NUMBER (ALIYUN_HTTP_DOMAIN_MAX)

extern const char *g_aliyun_mqtt_domain[ALIYUN_MQTT_DOMAIN_NUMBER];

extern const char *g_aliyun_http_domain[ALIYUN_HTTP_DOMAIN_NUMBER];

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_DEFS_H__ */
