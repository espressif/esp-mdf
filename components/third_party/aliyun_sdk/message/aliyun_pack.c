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
#include <stdlib.h>

#include "aliyun_pack.h"
#include "aliyun_kv.h"
#include "aliyun_platform.h"
#include "aliyun_sign.h"
#include "cJSON.h"
#include "esp_system.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_pack";

#define ALIYUN_PACK_PARAM_CHECK(con)                       \
    do {                                                   \
        if (!(con)) {                                      \
            MDF_LOGE("<MDF_ERR_INVALID_ARG> !(%s)", #con); \
            return NULL;                                   \
        }                                                  \
    } while (0)

static char g_product_key[ALIYUN_PRODUCT_KEY_MAXLEN] = { 0 };
static char g_device_name[ALIYUN_DEVICE_NAME_MAXLEN] = { 0 };
static char g_mac_str[ALIYUN_MAC_ADDRS_MAXLEN_STR] = { 0 };
static char g_ip_str[ALIYUN_NETWORK_ADDR_MAXLEN_STR] = { 0 };
static char g_token_str[ALIYUN_TOKEN_ADDR_MAXLEN_STR] = { 0 };

char *aliyun_pack_coap_connectapp_notify(uint32_t msg_id)
{
    aliyun_get_product_key(g_product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    aliyun_get_device_name(g_device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    aliyun_platform_get_device_mac_str(g_mac_str);
    aliyun_platform_get_ip_str(g_ip_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *awssVer = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "awss.event.connectap.notify");

    cJSON_AddStringToObject(awssVer, "smartconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "zconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "router", "2.0");
    cJSON_AddStringToObject(awssVer, "ap", "2.0");
    cJSON_AddStringToObject(awssVer, "softap", "2.0");

    cJSON_AddItemToObject(params, "awssVer", awssVer);
    cJSON_AddStringToObject(params, "productKey", g_product_key);
    cJSON_AddStringToObject(params, "deviceName", g_device_name);
    cJSON_AddStringToObject(params, "mac", g_mac_str);
    cJSON_AddStringToObject(params, "ip", g_ip_str);
    cJSON_AddNumberToObject(params, "cipherType", 3);
    cJSON_AddNumberToObject(params, "type", 0);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_coap_device_info_notify(uint32_t msg_id)
{
    aliyun_get_product_key(g_product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    aliyun_get_device_name(g_device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    aliyun_platform_get_device_mac_str(g_mac_str);
    aliyun_platform_get_ip_str(g_ip_str);
    aliyun_platform_get_token_str(g_token_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *awssVer = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "device.info.notify");

    cJSON_AddStringToObject(awssVer, "smartconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "zconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "router", "2.0");
    cJSON_AddStringToObject(awssVer, "ap", "2.0");
    cJSON_AddStringToObject(awssVer, "softap", "2.0");

    cJSON_AddItemToObject(params, "awssVer", awssVer);
    cJSON_AddStringToObject(params, "productKey", g_product_key);
    cJSON_AddStringToObject(params, "deviceName", g_device_name);
    cJSON_AddStringToObject(params, "mac", g_mac_str);
    cJSON_AddStringToObject(params, "ip", g_ip_str);
    cJSON_AddNumberToObject(params, "cipherType", 3);
    cJSON_AddStringToObject(params, "token", g_token_str);
    cJSON_AddNumberToObject(params, "remainTime", aliyun_platform_get_utc_ms());
    cJSON_AddNumberToObject(params, "type", 0);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_coap_connectapp_info_get(uint32_t msg_id)
{
    aliyun_get_product_key(g_product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    aliyun_get_device_name(g_device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    aliyun_platform_get_device_mac_str(g_mac_str);
    aliyun_platform_get_ip_str(g_ip_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *awssVer = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddNumberToObject(pJson, "code", 200);

    cJSON_AddStringToObject(awssVer, "smartconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "zconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "router", "2.0");
    cJSON_AddStringToObject(awssVer, "ap", "2.0");
    cJSON_AddStringToObject(awssVer, "softap", "2.0");
    cJSON_AddItemToObject(params, "awssVer", awssVer);

    cJSON_AddStringToObject(params, "productKey", g_product_key);
    cJSON_AddStringToObject(params, "deviceName", g_device_name);
    cJSON_AddStringToObject(params, "mac", g_mac_str);
    cJSON_AddStringToObject(params, "ip", g_ip_str);
    cJSON_AddNumberToObject(params, "cipherType", 3);
    cJSON_AddNumberToObject(params, "type", 0);

    cJSON_AddItemToObject(pJson, "data", params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_coap_device_info_get(uint32_t msg_id)
{
    aliyun_get_product_key(g_product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    aliyun_get_device_name(g_device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    aliyun_platform_get_device_mac_str(g_mac_str);
    aliyun_platform_get_ip_str(g_ip_str);
    aliyun_platform_get_token_str(g_token_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *awssVer = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddNumberToObject(pJson, "code", 200);

    cJSON_AddStringToObject(awssVer, "smartconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "zconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "router", "2.0");
    cJSON_AddStringToObject(awssVer, "ap", "2.0");
    cJSON_AddStringToObject(awssVer, "softap", "2.0");

    cJSON_AddItemToObject(params, "awssVer", awssVer);

    cJSON_AddStringToObject(params, "productKey", g_product_key);
    cJSON_AddStringToObject(params, "deviceName", g_device_name);
    cJSON_AddStringToObject(params, "mac", g_mac_str);
    cJSON_AddStringToObject(params, "ip", g_ip_str);
    cJSON_AddNumberToObject(params, "cipherType", 3);
    cJSON_AddStringToObject(params, "token", g_token_str);
    cJSON_AddNumberToObject(params, "remainTime", aliyun_platform_get_utc_ms());
    cJSON_AddNumberToObject(params, "type", 0);

    cJSON_AddItemToObject(pJson, "data", params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_awss_enrollee_match(uint32_t msg_id)
{
    aliyun_platform_get_token_str(g_token_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.awss.enrollee.match");

    cJSON_AddStringToObject(params, "token", g_token_str);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_reset(uint32_t msg_id)
{
    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.reset");

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_cipher_get(uint32_t msg_id)
{
    aliyun_get_product_key(g_product_key, ALIYUN_PRODUCT_KEY_MAXLEN);
    aliyun_get_device_name(g_device_name, ALIYUN_DEVICE_NAME_MAXLEN);
    aliyun_platform_get_token_str(g_token_str);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *awssVer = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.cipher.get");

    cJSON_AddStringToObject(awssVer, "smartconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "zconfig", "2.0");
    cJSON_AddStringToObject(awssVer, "router", "2.0");
    cJSON_AddStringToObject(awssVer, "ap", "2.0");
    cJSON_AddStringToObject(awssVer, "softap", "2.0");

    cJSON_AddItemToObject(params, "awssVer", awssVer);

    cJSON_AddStringToObject(params, "productKey", g_product_key);
    cJSON_AddStringToObject(params, "deviceName", g_device_name);
    cJSON_AddNumberToObject(params, "cipherType", 3);
    cJSON_AddStringToObject(params, "random", g_token_str);

    cJSON_AddItemToObject(pJson, "params", params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_sub_topo_add(uint32_t msg_id, const aliyun_device_meta_t *sub_meta)
{
    ALIYUN_PACK_PARAM_CHECK(sub_meta);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();

    aliyun_sub_sign_info_t sub_sign = { 0 };

    mdf_err_t ret = aliyun_sign_config(sub_meta, &sub_sign);
    ALIYUN_PACK_PARAM_CHECK(!ret);

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.topo.add");

    cJSON_AddStringToObject(params, "productKey", sub_sign.product_key);
    cJSON_AddStringToObject(params, "deviceName", sub_sign.device_name);
    cJSON_AddStringToObject(params, "clientId", sub_sign.client_id);
    cJSON_AddStringToObject(params, "timestamp", sub_sign.timestamp);
    cJSON_AddStringToObject(params, "signMethod", sub_sign.sign_method);
    cJSON_AddStringToObject(params, "sign", sub_sign.sign);

    cJSON_AddItemToArray(array, params);
    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, array);

    char *json_buf = cJSON_PrintUnformatted(pJson);

    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_sub_topo_delete(uint32_t msg_id, const aliyun_device_meta_t *sub_meta)
{
    ALIYUN_PACK_PARAM_CHECK(sub_meta);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();

    aliyun_sub_sign_info_t sub_sign = { 0 };

    mdf_err_t ret = aliyun_sign_config(sub_meta, &sub_sign);
    ALIYUN_PACK_PARAM_CHECK(!ret);

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.topo.delete");

    cJSON_AddStringToObject(params, "productKey", sub_sign.product_key);
    cJSON_AddStringToObject(params, "deviceName", sub_sign.device_name);

    cJSON_AddItemToArray(array, params);
    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, array);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_sub_combine_login(uint32_t msg_id, const aliyun_device_meta_t *sub_meta)
{
    ALIYUN_PACK_PARAM_CHECK(sub_meta);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    aliyun_sub_sign_info_t sub_sign = { 0 };

    mdf_err_t ret = aliyun_sign_config(sub_meta, &sub_sign);
    ALIYUN_PACK_PARAM_CHECK(!ret);

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "combine.login");

    cJSON_AddStringToObject(params, "productKey", sub_sign.product_key);
    cJSON_AddStringToObject(params, "deviceName", sub_sign.device_name);
    cJSON_AddStringToObject(params, "clientId", sub_sign.client_id);
    cJSON_AddStringToObject(params, "timestamp", sub_sign.timestamp);
    cJSON_AddStringToObject(params, "signMethod", sub_sign.sign_method);
    cJSON_AddStringToObject(params, "sign", sub_sign.sign);
    cJSON_AddStringToObject(params, "cleanSession", "true");

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);

    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_sub_combine_logout(uint32_t msg_id, const aliyun_device_meta_t *sub_meta)
{
    ALIYUN_PACK_PARAM_CHECK(sub_meta);

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    aliyun_sub_sign_info_t sub_sign = { 0 };

    mdf_err_t ret = aliyun_sign_config(sub_meta, &sub_sign);
    ALIYUN_PACK_PARAM_CHECK(!ret);

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "combine.logout");

    cJSON_AddStringToObject(params, "productKey", sub_sign.product_key);
    cJSON_AddStringToObject(params, "deviceName", sub_sign.device_name);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);

    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_event_property_post(uint32_t msg_id, const char *params)
{
    ALIYUN_PACK_PARAM_CHECK(params);

    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.event.property.post");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_event_property_pack_post(uint32_t msg_id, const char *params)
{
    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.event.property.pack.post");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_event_identifier_post(uint32_t msg_id, const char *identifier, const char *value)
{
    ALIYUN_PACK_PARAM_CHECK(identifier);
    ALIYUN_PACK_PARAM_CHECK(value);

    const char ALIYUN_METHOD_THING_EVENT_POST[] = "thing.event.%s.post";

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char *method = MDF_CALLOC(1, strlen(identifier) + strlen(ALIYUN_METHOD_THING_EVENT_POST));

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    sprintf(method, ALIYUN_METHOD_THING_EVENT_POST, identifier);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, method);

    cJSON_AddRawToObject(params, "value", value);
    cJSON_AddNumberToObject(params, "time", aliyun_platform_get_utc_ms());

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);
    MDF_FREE(method);

    return json_buf;
}

char *aliyun_pack_thing_id_code_reply(uint32_t msg_id, int code, const char *identifier, const char *data)
{
    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddNumberToObject(pJson, ALIYUN_MSG_KEY_CODE, code);

    if (identifier != NULL) {
        cJSON_AddStringToObject(pJson, "identifier", identifier);
    }

    if (data != NULL) {
        cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_DATA, data);
    } else {
        cJSON *params = cJSON_CreateObject();
        cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_DATA, params);
    }

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);
    return json_buf;
}

char *aliyun_pack_thing_event_deviceinfo_update(uint32_t msg_id, const char *params)
{
    ALIYUN_PACK_PARAM_CHECK(params);

    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.deviceinfo.update");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_event_deviceinfo_delete(uint32_t msg_id, const char *params)
{
    ALIYUN_PACK_PARAM_CHECK(params);

    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.deviceinfo.delete");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_ota_device_request(uint32_t msg_id, const char *version)
{

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);
    cJSON_AddStringToObject(params, "version", version);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_ota_device_inform(uint32_t msg_id, const char *version)
{
    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);
    cJSON_AddStringToObject(params, "version", version);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_ota_device_progress(uint32_t msg_id, int step, const char *desc)
{
    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddNumberToObject(params, "step", step);
    cJSON_AddStringToObject(params, "desc", desc);

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_dsltemplate_get(uint32_t msg_id)
{

    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.dsltemplate.get");

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

/* params 属性数组*/
char *aliyun_pack_thing_event_property_desired_get(uint32_t msg_id, const char *params)
{
    ALIYUN_PACK_PARAM_CHECK(params);

    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.property.desired.get");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

/* params 属性 json*/
char *aliyun_pack_thing_event_property_desired_delete(uint32_t msg_id, const char *params)
{
    ALIYUN_PACK_PARAM_CHECK(params);

    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.property.desired.delete");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_config_get(uint32_t msg_id)
{
    cJSON *pJson = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.config.get");

    cJSON_AddStringToObject(params, "configScope", "product");
    cJSON_AddStringToObject(params, "getType", "file");

    cJSON_AddItemToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_ntp_request(int64_t timestamp)
{
    cJSON *pJson = cJSON_CreateObject();

    cJSON_AddNumberToObject(pJson, "deviceSendTime", timestamp);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_log_post(uint32_t msg_id, const char *params)
{
    cJSON *pJson = cJSON_CreateObject();

    char str[20];
    snprintf(str, sizeof(str), "%u", msg_id);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_ID, str);

    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_VERSION, ALIYUN_MSG_VERSION);
    cJSON_AddStringToObject(pJson, ALIYUN_MSG_KEY_METHOD, "thing.log.post");

    cJSON_AddRawToObject(pJson, ALIYUN_MSG_KEY_PARAMS, params);

    char *json_buf = cJSON_PrintUnformatted(pJson);
    cJSON_Delete(pJson);

    return json_buf;
}

char *aliyun_pack_thing_update_sdk_version(uint32_t msg_id, const char *sdk_version)
{
    cJSON *version = cJSON_CreateObject();
    cJSON *language = cJSON_CreateObject();

    cJSON *array = cJSON_CreateArray();

    cJSON_AddStringToObject(version, "attrKey", "SYS_LP_SDK_VERSION");
    cJSON_AddStringToObject(version, "attrValue", sdk_version);
    cJSON_AddStringToObject(version, "domain", "SYSTEM");

    cJSON_AddStringToObject(language, "attrKey", "SYS_SDK_LANGUAGE");
    cJSON_AddStringToObject(language, "attrValue", "C");
    cJSON_AddStringToObject(language, "domain", "SYSTEM");

    cJSON_AddItemToArray(array, version);
    cJSON_AddItemToArray(array, language);

    char *params = cJSON_PrintUnformatted(array);
    MDF_LOGI("params: %s", params);

    char *json_buf = aliyun_pack_thing_event_deviceinfo_update(msg_id, params);

    cJSON_Delete(array);
    MDF_FREE(params);

    return json_buf;
}
