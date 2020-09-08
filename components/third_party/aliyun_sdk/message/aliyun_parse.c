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

#include "cJSON.h"

#include "aliyun_answer.h"
#include "aliyun_list.h"
#include "aliyun_ota.h"
#include "aliyun_parse.h"
#include "aliyun_platform.h"
#include "aliyun_topic.h"
#include "esp_system.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_parse";

static mdf_err_t aliyun_list_select_device_name_addrs(const char *product_key, const char *device_name, uint8_t addrs[])
{
    aliyun_device_meta_t meta = { 0 };
    strcpy(meta.product_key, product_key);
    strcpy(meta.device_name, device_name);
    return aliyun_list_select_addrs(&meta, addrs);
}

mdf_err_t aliyun_parse_thing_topo_add_reply(const char *data, int data_len)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;
    cJSON *pinfo = NULL;

    mdf_err_t ret = MDF_FAIL;
    MDF_LOGD("data:%s", data);
    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data id Error");
    pinfo = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON data id Error");
    MDF_LOGD("code:%d, id: %d", pSub->valueint, atoi(pinfo->valuestring));

    if (pSub->valueint == ALIYUN_SUCC_CODE) {
        ret = aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_TOPO_ADD_SUCC);
        MDF_LOGD("ret:%d", ret);
    } else {
        ret = aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_TOPO_ADD_FAIL);
        MDF_LOGD("ret:%d", ret);
    }

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_topo_delete_reply(const char *data, int data_len)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;
    cJSON *pinfo = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data id Error");
    pinfo = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON data id Error");
    MDF_LOGI("code:%d, id: %d", pSub->valueint, atoi(pinfo->valuestring));

    if (pSub->valueint == ALIYUN_SUCC_CODE) {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_TOPO_DELETE_SUCC);
    } else {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_TOPO_DELETE_FAIL);
    }

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_session_login_reply(const char *data, int data_len)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;
    cJSON *pinfo = NULL;
    MDF_LOGD("data:%s", data);
    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data id Error");
    pinfo = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON data id Error");
    MDF_LOGD("code:%d, id: %d", pSub->valueint, atoi(pinfo->valuestring));

    if (pSub->valueint == ALIYUN_SUCC_CODE) {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_LOGIN_SUCC);
    } else {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_LOGIN_FAIL);
    }

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_session_logout_reply(const char *data, int data_len)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;
    cJSON *pinfo = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data id Error");
    pinfo = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON data id Error");
    MDF_LOGD("code:%d, id: %d", pSub->valueint, atoi(pinfo->valuestring));

    if (pSub->valueint == ALIYUN_SUCC_CODE) {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_LOGOUT_SUCC);
    } else {
        aliyun_list_update_msg_status(atoi(pinfo->valuestring), ALIYUN_LIST_LOGOUT_FAIL);
    }

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_reset_reply(const char *data, uint32_t *msg_id, uint32_t *code)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");
    *msg_id = atoi(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code Error");
    *code = pSub->valueint;

    pSub = cJSON_GetObjectItem(pJson, "message");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON message Error");

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_awss_enrollee_match_reply(const char *data, uint32_t *msg_id, uint32_t *code)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");
    *msg_id = atoi(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code Error");
    *code = pSub->valueint;

    pSub = cJSON_GetObjectItem(pJson, "message");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON message Error");

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_gateway_reset_notify(const char *data)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "cmd");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON cmd id Error");

    if (!strcmp(pSub->valuestring, "ota")) {
        esp_restart();
    }

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_utils_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");

    *msg_id = atol(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code Error");
    *code = pSub->valueint;

    pSub = cJSON_GetObjectItem(pJson, "data");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data Error");

    *params = cJSON_PrintUnformatted(pSub);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);

    if (*params != NULL) {
        MDF_FREE(*params);
    }

    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_event_property_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_proprrty_set(const char *data, size_t data_len, uint32_t *msg_id, char **params)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");

    *msg_id = atol(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "params");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON params Error");

    *params = cJSON_PrintUnformatted(pSub);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);

    if (*params != NULL) {
        MDF_FREE(*params);
    }

    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_model_up_raw_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_deviceinfo_update_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_deviceinfo_delete_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_config_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_config_push_reply(const char *data, size_t data_len, uint32_t *msg_id, char **params)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");

    *msg_id = atol(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "params");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data Error");

    *params = cJSON_PrintUnformatted(pSub);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);

    if (*params != NULL) {
        MDF_FREE(*params);
    }

    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_event_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_event_post_identifier(const char *data, size_t data_len, char **identifier)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "method");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON method Error");
    *identifier = cJSON_PrintUnformatted(pSub);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);

    if (*identifier != NULL) {
        MDF_FREE(*identifier);
    }

    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_service_request(const char *data, size_t data_len, uint32_t *msg_id, char **identifier, char **params)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "id");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");

    *msg_id = atol(pSub->valuestring);

    pSub = cJSON_GetObjectItem(pJson, "method");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON method Error");
    *identifier = cJSON_PrintUnformatted(pSub);

    pSub = cJSON_GetObjectItem(pJson, "params");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data Error");
    *params = cJSON_PrintUnformatted(pSub);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);

    if (*params != NULL) {
        MDF_FREE(*params);
    }

    if (*identifier != NULL) {
        MDF_FREE(*identifier);
    }

    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_event_pack_post_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_ntp_response(const char *data, size_t data_len)
{
    cJSON *pJson = NULL;
    cJSON *pSub = NULL;

    mdf_err_t ret = MDF_OK;

    int64_t deviceRecvTime = aliyun_platform_get_utc_ms();

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(!pJson, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "deviceSendTime");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON id Error");
    int64_t deviceSendTime = (int64_t)pSub->valuedouble;

    pSub = cJSON_GetObjectItem(pJson, "serverRecvTime");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code Error");
    int64_t serverRecvTime = (int64_t)pSub->valuedouble;

    pSub = cJSON_GetObjectItem(pJson, "serverSendTime");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code Error");
    int64_t serverSendTime = (int64_t)pSub->valuedouble;

    if (deviceRecvTime - deviceSendTime > 60 * 1000) {
        MDF_LOGW("time difference is too large, ignore it");
        goto JSON_EXIT;
    }

    int64_t accurateTime = (deviceRecvTime - deviceSendTime + serverSendTime + serverRecvTime) / 2;
    ret = aliyun_platform_set_utc_ms(accurateTime);
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_EXIT, "aliyun_platform_set_utc_ms Error");

    aliyun_answer_update_utc_ms(aliyun_platform_get_utc_ms() - deviceRecvTime);

    cJSON_Delete(pJson);
    return MDF_OK;

JSON_EXIT:
    cJSON_Delete(pJson);
    return MDF_FAIL;
}

mdf_err_t aliyun_parse_thing_dsltemplate_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_property_desired_get_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_thing_property_desired_delete_reply(const char *data, size_t data_len, uint32_t *msg_id, uint32_t *code, char **params)
{
    return aliyun_parse_thing_utils_reply(data, data_len, msg_id, code, params);
}

mdf_err_t aliyun_parse_mqtt_subdevice_callback(aliyun_msg_type_t type, char *product_key, char *device_name, char *data, int data_len)
{
    mdf_err_t ret = MDF_OK;
    uint8_t dest_addrs[ALIYUN_SUBDEVICE_ADDRS_MAXLEN] = { 0 };
    ret = aliyun_list_select_device_name_addrs(product_key, device_name, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_send_data not find subdevice mac, ret: %d", ret);
    MDF_LOGD("type:%d, product_key: %s, device_name: %s, dest_addrs: " MACSTR, type, product_key, device_name, MAC2STR(dest_addrs));
    return aliyun_platform_gateway_write(dest_addrs, type, data, data_len);
}

mdf_err_t aliyun_parse_mqtt_gateway_callback(aliyun_msg_type_t type, const void *data, size_t data_len)
{
    MDF_PARAM_CHECK(data);

    mdf_err_t ret = MDF_OK;

    switch (type) {

        case ALIYUN_MQTT_SUBSCRIBE_GATEWAY_PERMIT: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_GATEWAY_PERMIT");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_SUB_REGISTER_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_SUB_REGISTER_REPLY");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_SUB_UNREGISTER_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_SUB_UNREGISTER_REPLY");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_REPLY");
            ret = aliyun_parse_thing_topo_add_reply(data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_topo_add_reply error, ret: %d", ret);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_DELETE_TOPO_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_DELETE_TOPO_REPLY");
            ret = aliyun_parse_thing_topo_delete_reply(data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_topo_delete_reply error, ret: %d", ret);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_FOUND_LIST_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_FOUND_LIST_REPLY");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_NOTIFY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_NOTIFY");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_DISABLE: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_DISABLE");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_ENABLE: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_ENABLE");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_DELETE: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_DELETE");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGIN_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGIN_REPLY");
            ret = aliyun_parse_session_login_reply(data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_session_login_reply error, ret: %d", ret);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGOUT_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGOUT_REPLY");
            ret = aliyun_parse_session_logout_reply(data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_session_login_reply error, ret: %d", ret);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_EXT_ERROR: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_EXT_ERROR");
        }
        break;

        default:
            MDF_LOGW("aliyun_linkkit_check_cb_t type is null");
            break;
    }

    return MDF_OK;
}

mdf_err_t aliyun_parse_ota_upgrade(const char *product_key, const char *device_name, const char *data, int data_len)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(data);

    cJSON *pJson = NULL;
    cJSON *pSub = NULL;
    cJSON *pinfo = NULL;

    MDF_LOGI("product_key: %s", product_key);
    MDF_LOGI("device_name: %s", device_name);

    pJson = cJSON_Parse(data);
    MDF_ERROR_CHECK(pJson == NULL, MDF_FAIL, "cJSON_Parse");

    aliyun_ota_info_t *ota_info = MDF_CALLOC(1, sizeof(aliyun_ota_info_t));
    MDF_ERROR_CHECK(ota_info == NULL, MDF_FAIL, "cJSON_Parse");

    pSub = cJSON_GetObjectItem(pJson, "code");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON code id Error");

    pSub = cJSON_GetObjectItem(pJson, "data");
    MDF_ERROR_GOTO(pSub == NULL, JSON_EXIT, "cJSON data id Error");

    pinfo = cJSON_GetObjectItem(pSub, "size");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON size id Error");
    ota_info->size = pinfo->valueint;
    MDF_LOGI("size : %d", ota_info->size);

    pinfo = cJSON_GetObjectItem(pSub, "version");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON version id Error");
    strncpy(ota_info->version, pinfo->valuestring, ALIYUN_FIRMWARE_VERSION_MAXLEN - 1);
    MDF_LOGI("version : %s", ota_info->version);

    pinfo = cJSON_GetObjectItem(pSub, "url");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON url id Error");
    strncpy(ota_info->url, pinfo->valuestring, ALIYUN_OTA_URL_MAXLEN - 1);
    MDF_LOGD("url : %s", ota_info->url);

    pinfo = cJSON_GetObjectItem(pSub, "signMethod");
    MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON signMethod id Error");
    strncpy(ota_info->sign_method, pinfo->valuestring, ALIYUN_SIGN_METHOD_MAXLEN);
    MDF_LOGI("sign_method : %s", ota_info->sign_method);

    if (!strcmp(ota_info->sign_method, "Md5")) {
        pinfo = cJSON_GetObjectItem(pSub, "md5");
        MDF_ERROR_GOTO(pinfo == NULL, JSON_EXIT, "cJSON md5 id Error");
        strncpy(ota_info->sign, pinfo->valuestring, ALIYUN_SIGN_MAXLEN_STR - 1);
    }

    mdf_err_t err = aliyun_ota_init(product_key, device_name, ota_info);

    if (err != MDF_OK) {
        MDF_LOGE("Aliyun_ota_init failed(%d)", err);
    }

JSON_EXIT:
    MDF_FREE(ota_info);
    cJSON_Delete(pJson);
    return MDF_OK;
}

mdf_err_t aliyun_parse_mqtt_ota_callback(aliyun_msg_type_t type, char *product_key, char *device_name, char *data, int data_len)
{
    switch (type) {
        case ALIYUN_MQTT_SUBSCRIBE_OTA_DEVICE_UPGRADE: {
            MDF_LOGI("ALIYUN_MQTT_SUBSCRIBE_OTA_DEVICE_UPGRADE");
            aliyun_parse_ota_upgrade(product_key, device_name, data, data_len);
        }
        break;

        default:
            MDF_LOGW("aliyun_linkkit_check_cb_t type is null");
            break;
    }

    return MDF_OK;
}

const char ALIYUN_PARSE_SYS_PREFIX_REGULAR[] = "/sys/%" ALIYUN_PRODUCT_KEY_MAXLEN_STR "[^/]/%" ALIYUN_DEVICE_NAME_MAXLEN_STR "[^/]";
const char ALIYUN_PARSE_EXT_NTP_PREFIX_REGULAR[] = "/ext/ntp/%" ALIYUN_PRODUCT_KEY_MAXLEN_STR "[^/]/%" ALIYUN_DEVICE_NAME_MAXLEN_STR "[^/]";
const char ALIYUN_PARSE_EXT_ERROR_PREFIX_REGULAR[] = "/ext/error/%" ALIYUN_PRODUCT_KEY_MAXLEN_STR "[^/]/%" ALIYUN_DEVICE_NAME_MAXLEN_STR "[^/]";
const char ALIYUN_PARSE_OTA_DEVICE_REQUEST_REGULAR[] = "/ota/device/request/%" ALIYUN_PRODUCT_KEY_MAXLEN_STR "[^/]/%" ALIYUN_DEVICE_NAME_MAXLEN_STR "[^/]";
const char ALIYUN_PARSE_OTA_DEVICE_UPGRADE_REGULAR[] = "/ota/device/upgrade/%" ALIYUN_PRODUCT_KEY_MAXLEN_STR "[^/]/%" ALIYUN_DEVICE_NAME_MAXLEN_STR "[^/]";

const char ALIYUN_PARSE_CMP_SYS_PREFIX[] = "/sys/";
const char ALIYUN_PARSE_CMP_EXT_SESSION_PREFIX[] = "/ext/session/";
const char ALIYUN_PARSE_CMP_EXT_NTP_PREFIX[] = "/ext/ntp/";
const char ALIYUN_PARSE_CMP_EXT_ERROR_PREFIX[] = "/ext/error/";
const char ALIYUN_PARSE_CMP_OTA_DEVICE_REQUEST[] = "/ota/device/request/";
const char ALIYUN_PARSE_CMP_OTA_DEVICE_UPGRADE[] = "/ota/device/upgrade/";

const char ALIYUN_PARSE_THING_SERVICE_REQUEST_WILDCARD[] = "thing/service/";

const char ALIYUN_PARSE_THING_EVENT_POST_REPLY_WILDCARD_PREFIX[] = "thing/event/";
const char ALIYUN_PARSE_THING_EVENT_POST_REPLY_WILDCARD_SUFFIX[] = "/post_reply";

mdf_err_t aliyun_parse_mqtt_data(char *topic, int topic_len, char *data, int data_len)
{
#define ALIYUN_PARSE_SSCANF_NUM (2)
    mdf_err_t ret = MDF_OK;

    char product_key[ALIYUN_PRODUCT_KEY_MAXLEN] = { 0 };
    char device_name[ALIYUN_DEVICE_NAME_MAXLEN] = { 0 };

    if (strncmp(topic, ALIYUN_PARSE_CMP_SYS_PREFIX, strlen(ALIYUN_PARSE_CMP_SYS_PREFIX)) == MDF_OK) {

        ret = sscanf(topic, ALIYUN_PARSE_SYS_PREFIX_REGULAR, product_key, device_name);
        MDF_ERROR_CHECK(ret != ALIYUN_PARSE_SSCANF_NUM, ret, "sscanf product_key and device_name Error");

        if (strstr(topic, ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_PROPERTY_SET, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_property_set Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_POST_PROPERTY_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_property_get Error");

        } else if (strstr(topic, ALIYUN_PARSE_THING_EVENT_POST_REPLY_WILDCARD_PREFIX) != NULL && strstr(topic, ALIYUN_PARSE_THING_EVENT_POST_REPLY_WILDCARD_SUFFIX) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_POST_EVENT_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_request_wildcard Error");

        } else if (strstr(topic, ALIYUN_PARSE_THING_SERVICE_REQUEST_WILDCARD) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SERVICE_INVOKE, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_request_wildcard Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_deviceinfo_update_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_DEVICEINFO_DELETE_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_deviceinfo_delete_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_RRPC_REQUEST_WILDCARD) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_RRPC_REQUEST_WILDCARD, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_rrpc_request_wildcard Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_MODEL_DOWN_RAW) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_model_down_raw Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_MODEL_UP_RAW_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_model_up_raw_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_TOPO_ADD_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_gateway_callback(ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_REPLY, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_topo_add_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_TOPO_DELETE_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_gateway_callback(ALIYUN_MQTT_SUBSCRIBE_DELETE_TOPO_REPLY, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_topo_delete_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_DYNAMICTSL_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_GET_DYNAMICTSL_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_dynamictsl_get_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_RESET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_RESET_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_reset_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_FOUND_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_awss_enrollee_found_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_MATCH_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_enrollee_match_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_CIPHER_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_CIPHER_GET_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_cipher_get_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_AWSS_DEVICE_SWITCHAP, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_awss_device_switchap Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_CONFIG_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_GET_CONFIG_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_get_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_THING_CONFIG_PUSH) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_CONFIG_PUSH, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push Error");
        } else if (strstr(topic, ALIYUN_TOPIC_THING_DSLTEMPLATE_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push Error");
        } else if (strstr(topic, ALIYUN_TOPIC_THING_DYNAMICTSL_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_GET_DYNAMICTSL_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push Error");
        } else if (strstr(topic, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_GET_DESIRED_PROPERTY_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push Error");
        } else if (strstr(topic, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_SUBSCRIBE_DELETE_DESIRED_PROPERTY_REPLY, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push Error");
        } else {
            MDF_LOGW("aliyun_parse_mqtt_data /sys/ error, topic: %s", topic);
        }

    } else if (strncmp(topic, ALIYUN_PARSE_CMP_EXT_SESSION_PREFIX, strlen(ALIYUN_PARSE_CMP_EXT_SESSION_PREFIX)) == MDF_OK) {
        if (strstr(topic, ALIYUN_TOPIC_COMBINE_LOGIN_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_gateway_callback(ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGIN_REPLY, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_session_login_reply Error");

        } else if (strstr(topic, ALIYUN_TOPIC_COMBINE_LOGOUT_REPLY) != NULL) {
            ret = aliyun_parse_mqtt_gateway_callback(ALIYUN_MQTT_SUBSCRIBE_SESSION_COMBINE_LOGOUT_REPLY, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_session_logout_reply Error");

        } else {
            MDF_LOGW("aliyun_parse_mqtt_data /ext/session/ error, topic: %s", topic);
        }

    } else if (strncmp(topic, ALIYUN_PARSE_CMP_EXT_NTP_PREFIX, strlen(ALIYUN_PARSE_CMP_EXT_NTP_PREFIX)) == MDF_OK) {

        ret = sscanf(topic, ALIYUN_PARSE_EXT_NTP_PREFIX_REGULAR, product_key, device_name);
        MDF_ERROR_CHECK(ret != ALIYUN_PARSE_SSCANF_NUM, ret, "sscanf product_key and device_name Error");

        if (strstr(topic, ALIYUN_TOPIC_NTP_RESPONSE) != NULL) {
            ret = aliyun_parse_mqtt_subdevice_callback(ALIYUN_MQTT_REQUEST_NTP_RESPONSE, product_key, device_name, data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_ext_ntp_response Error");

        } else {
            MDF_LOGW("aliyun_parse_mqtt_data /ext/ntp/ error, topic: %s", topic);
        }

    } else if (strncmp(topic, ALIYUN_PARSE_CMP_EXT_ERROR_PREFIX, strlen(ALIYUN_PARSE_CMP_EXT_ERROR_PREFIX)) == MDF_OK) {

        ret = aliyun_parse_mqtt_gateway_callback(ALIYUN_MQTT_SUBSCRIBE_EXT_ERROR, data, data_len);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_ext_error Error");

    } else if (strncmp(topic, ALIYUN_PARSE_CMP_OTA_DEVICE_REQUEST, strlen(ALIYUN_PARSE_CMP_OTA_DEVICE_REQUEST)) == MDF_OK) {

        ret = sscanf(topic, ALIYUN_PARSE_OTA_DEVICE_REQUEST_REGULAR, product_key, device_name);
        MDF_ERROR_CHECK(ret != ALIYUN_PARSE_SSCANF_NUM, ret, "sscanf product_key and device_name Error");

        ret = aliyun_parse_mqtt_ota_callback(ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST, product_key, device_name, data, data_len);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_ota_device_request Error");

    } else if (strncmp(topic, ALIYUN_PARSE_CMP_OTA_DEVICE_UPGRADE, strlen(ALIYUN_PARSE_CMP_OTA_DEVICE_UPGRADE)) == MDF_OK) {

        ret = sscanf(topic, ALIYUN_PARSE_OTA_DEVICE_UPGRADE_REGULAR, product_key, device_name);
        MDF_ERROR_CHECK(ret != ALIYUN_PARSE_SSCANF_NUM, ret, "sscanf product_key and device_name Error");

        ret = aliyun_parse_mqtt_ota_callback(ALIYUN_MQTT_SUBSCRIBE_OTA_DEVICE_UPGRADE, product_key, device_name, data, data_len);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_ota_device_request Error");

    } else {
        MDF_LOGE("aliyun_parse_mqtt_data error, topic: %s", topic);
        return MDF_FAIL;
    }

    return MDF_OK;
}
