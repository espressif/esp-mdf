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
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_mesh.h"
#include "mdf_mem.h"

#include "aliyun_answer.h"
#include "aliyun_coap.h"
#include "aliyun_cyclic.h"
#include "aliyun_pack.h"
#include "aliyun_parse.h"

#include "aliyun_mqtt.h"
#include "aliyun_platform.h"
#include "aliyun_subdevice.h"
#include "aliyun_kv.h"


#define HEART_BEAT_INTERVAL (60)

#define GATEWAY_ADD_SUBDEVICE_REPLY BIT0
#define GATEWAY_DEL_SUBDEVICE_REPLY BIT1
#define SUBDEVICE_ADD_GATEWAY_TIMEOUT (10*1000*1000) // us
#define SUBDEVICE_ADD_GATEWAY_ERROR_TIMEOUT (1*1000*1000) // us

static const char *TAG = "aliyun_subdevice";
static TaskHandle_t g_aliyun_subdevice_task_handle = NULL;
static aliyun_answer_status_t g_subdevice_to_gateway_status = ALIYUN_ANSWER_STATUS_FAIL;
static bool g_network_config_flag = false;
static bool g_subdevice_is_added = false;
static bool g_parent_is_conneced = false;
static bool g_link_is_completed = true;
static bool g_inform_link_loss = false;
static bool g_is_online = false;

static char *remove_identifier_prefix(char *identifier)
{
    if (identifier == NULL) {
        return NULL;
    }

    char *str = strrchr(identifier, '.');
    assert(str != NULL);
    str += 1;
    char *quotation = strrchr(str, '\"');
    assert(quotation != NULL);
    *quotation = '\0';
    return str;
}

static void subdevice_inform_link_loss()
{
    uint8_t dest_addr[6] = "\xff\xff\xff\xff\xff\xff";
    uint32_t msg_id = aliyun_platform_get_msg_id();
    aliyun_platform_gateway_write(dest_addr, ALIYUN_PARENT_OFFLINE, &msg_id, sizeof(msg_id));
}

bool aliyun_subdevice_get_network_config(void)
{
    return g_network_config_flag;
}

void aliyun_subdevice_set_network_config(bool flag)
{
    g_network_config_flag = flag;
}

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
void aliyun_subdevice_set_rootless(bool rootless)
{
    if (rootless) {
        g_subdevice_is_added = false;
    }
}

void aliyun_subdevice_update_tods(mesh_event_toDS_state_t state)
{
    if (state == MESH_TODS_UNREACHABLE) {
        if (g_is_online) {
            aliyun_platform_event_loop_send(ALIYUN_EVENT_OFFLINE, NULL);
            g_is_online = false;
        }
    }
}

void aliyun_subdevice_parent_connected()
{
    if (!g_parent_is_conneced) {
        g_parent_is_conneced = true;
    }
}

void aliyun_subdevice_parent_disconnected()
{
    if (g_parent_is_conneced) {
        g_parent_is_conneced = false;
        g_inform_link_loss = true;

        if (g_is_online) {
            aliyun_platform_event_loop_send(ALIYUN_EVENT_OFFLINE, NULL);
            g_is_online = false;
        }
    }
}
#endif

mdf_err_t aliyun_subdevice_get_login_status(void)
{
    bool status = aliyun_platform_get_cloud_connect_status();
    return status && g_subdevice_to_gateway_status == ALIYUN_ANSWER_STATUS_SUCC ? MDF_OK : MDF_FAIL;
}

bool aliyun_subdevice_is_online(void)
{
    bool status = aliyun_platform_get_cloud_connect_status();
#if CONFIG_ALIYUN_PLATFORM_MDF
    return status && g_subdevice_is_added && g_parent_is_conneced && g_link_is_completed;
#else
    return status && g_subdevice_is_added;
#endif
}

mdf_err_t aliyun_subdevice_add_to_gateway(const aliyun_device_meta_t *meta)
{
    if (!aliyun_platform_get_cloud_connect_status()) {
        return MDF_ERR_INVALID_STATE;
    }

    MDF_PARAM_CHECK(meta);

    mdf_err_t ret = MDF_FAIL;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    MDF_LOGI("Subdevice add itself to gateway, ProductKey=%s, DeviceName=%s, Mac=" MACSTR, meta->product_key, meta->device_name, MAC2STR(mac));
    ret = aliyun_platform_subdevice_write(ALIYUN_GATEWAY_ADD_SUBDEVICE, meta, sizeof(aliyun_device_meta_t));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_login error");
    return ret;
}

mdf_err_t aliyun_subdevice_delete_from_gateway(const aliyun_device_meta_t *meta)
{
    MDF_PARAM_CHECK(meta);

    mdf_err_t ret = MDF_FAIL;
    MDF_LOGI("Subdevice delete itself from gateway, ProductKey=%s, DeviceName=%s", meta->product_key, meta->device_name);
    ret = aliyun_platform_subdevice_write(ALIYUN_GATEWAY_DELETE_SUBDEVICE, meta, sizeof(aliyun_device_meta_t));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_logout error");
    return ret;
}

mdf_err_t aliyun_subdevice_heartbeat_to_gateway(void)
{
    if (aliyun_platform_get_cloud_connect_status() == false) {
        return MDF_FAIL;
    }

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    ret = aliyun_platform_subdevice_write(ALIYUN_SUBDEVICE_HEARTBEAT, &msg_id, sizeof(uint32_t));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_heartbeat error");
    return ret;
}

mdf_err_t aliyun_subdevice_match_enrollee(void)
{
    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *post_buf = aliyun_pack_thing_awss_enrollee_match(msg_id);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_awss_enrollee_match is NULL");

    ret = aliyun_answer_set_wait(ALIYUN_ANSWER_TYPE_UPDATE_TOKEN, msg_id);
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_answer_set_wait error");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_MATCH, post_buf, strlen(post_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(post_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_reset(void)
{
    if (!aliyun_platform_get_is_gateway()) {
        return MDF_OK;
    }

    mdf_err_t ret = MDF_FAIL;
    char *post_buf = NULL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    post_buf = aliyun_pack_thing_reset(msg_id);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_reset is NULL");

    ret = aliyun_answer_set_wait(ALIYUN_ANSWER_TYPE_UPDATE_RESET, msg_id);
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_answer_set_wait error");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_RESET, post_buf, strlen(post_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(post_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_get_cipher(void)
{
    mdf_err_t ret = MDF_FAIL;
    char *post_buf = NULL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    post_buf = aliyun_pack_thing_cipher_get(msg_id);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_cipher_get is NULL");

    ret = aliyun_answer_set_wait(ALIYUN_ANSWER_TYPE_CIPHER_GET, msg_id);
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_answer_set_wait error");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_CIPHER_GET, post_buf, strlen(post_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(post_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_post_property(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    char *post_buf = NULL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    post_buf = aliyun_pack_thing_event_property_post(msg_id, params);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_post is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_POST, post_buf, strlen(post_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(post_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_reply_property_set(uint32_t id, int code, const char *data)
{
    if (data != NULL) {
        MDF_PARAM_CHECK(strlen(data));
        MDF_ERROR_CHECK(aliyun_platform_check_json(data) != MDF_OK, MDF_FAIL, "data not json");
    }

    mdf_err_t ret = MDF_FAIL;

    char *reply_buf = aliyun_pack_thing_id_code_reply(id, code, NULL, data);
    MDF_ERROR_CHECK(reply_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_set_reply is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_SET_SERVICE_PROPERTY_REPLY, reply_buf, strlen(reply_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(reply_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_post_event(const char *identifier, const char *value)
{
    MDF_PARAM_CHECK(identifier && strlen(identifier));
    MDF_PARAM_CHECK(value && strlen(value));
    MDF_ERROR_CHECK(aliyun_platform_check_json(value) != MDF_OK, MDF_FAIL, "value not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *post_buf = aliyun_pack_thing_event_identifier_post(msg_id, identifier, value);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_identifier_post is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_EVENT_POST, post_buf, strlen(post_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(post_buf);

    return ret;
}

mdf_err_t aliyun_subdevice_reply_service_invoke(uint32_t id, int code, const char *identifier, const char *data)
{
    if (data != NULL) {
        MDF_PARAM_CHECK(strlen(data));
        MDF_ERROR_CHECK(aliyun_platform_check_json(data) != MDF_OK, MDF_FAIL, "data not json");
    }

    mdf_err_t ret = MDF_FAIL;

    char *reply_buf = aliyun_pack_thing_id_code_reply(id, code, identifier, data);
    MDF_ERROR_CHECK(reply_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_set_reply is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_RESPONSE_SERVICE, reply_buf, strlen(reply_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(reply_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_pack_post_property(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *post_buf = aliyun_pack_thing_event_property_pack_post(msg_id, params);
    MDF_ERROR_CHECK(post_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_pack_post is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_PACK_POST, post_buf, strlen(post_buf));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_thing_event_property_pack_poet error");

    return ret;
}

mdf_err_t aliyun_subdevice_update_deviceinfo(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *update_buf = aliyun_pack_thing_event_deviceinfo_update(msg_id, params);
    MDF_ERROR_CHECK(update_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_deviceinfo_update is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_UPDATE_DEVICEINFO, update_buf, strlen(update_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(update_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_delete_deviceinfo(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *dalete_buf = aliyun_pack_thing_event_deviceinfo_delete(msg_id, params);
    MDF_ERROR_CHECK(dalete_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_deviceinfo_delete is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_DELETE_DEVICEINFO, dalete_buf, strlen(dalete_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(dalete_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_get_config(void)
{
    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *get_buf = aliyun_pack_thing_config_get(msg_id);
    MDF_ERROR_CHECK(get_buf == NULL, MDF_FAIL, "aliyun_subdevice_get_config is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_GET_CONFIG, get_buf, strlen(get_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(get_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_reply_config_push(int id, int code, const char *data)
{
    if (data != NULL) {
        MDF_PARAM_CHECK(strlen(data));
        MDF_ERROR_CHECK(aliyun_platform_check_json(data) != MDF_OK, MDF_FAIL, "data not json");
    }

    mdf_err_t ret = MDF_FAIL;

    char *reply_buf = aliyun_pack_thing_id_code_reply(id, code, NULL, data);
    MDF_ERROR_CHECK(reply_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_set_reply is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_CONFIG_PUSH_REPLY, reply_buf, strlen(reply_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(reply_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_inform_version(const char *version)
{
    MDF_PARAM_CHECK(version && strlen(version));

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *ota_buf = aliyun_pack_ota_device_inform(msg_id, version);
    MDF_ERROR_CHECK(ota_buf == NULL, MDF_FAIL, "aliyun_pack_ota_device_inform is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_OTA_DEVICE_INFORM, ota_buf, strlen(ota_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(ota_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_post_ota_progress(int step, const char *desc)
{
    if (desc != NULL) {
        MDF_PARAM_CHECK(strlen(desc));
    }

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *ota_buf = aliyun_pack_ota_device_progress(msg_id, step, desc);
    MDF_ERROR_CHECK(ota_buf == NULL, MDF_FAIL, "aliyun_pack_ota_device_request is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_OTA_DEVICE_PROGRESS, ota_buf, strlen(ota_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(ota_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_request_ota_device(const char *version)
{
    MDF_PARAM_CHECK(version && strlen(version));

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *ota_buf = aliyun_pack_ota_device_request(msg_id, version);
    MDF_ERROR_CHECK(ota_buf == NULL, MDF_FAIL, "aliyun_pack_ota_device_request is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST, ota_buf, strlen(ota_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(ota_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_request_ntp(void)
{
    mdf_err_t ret = MDF_FAIL;
    int64_t timestamp = aliyun_platform_get_utc_ms();

    char *ntp_buf = aliyun_pack_ntp_request(timestamp);
    MDF_ERROR_CHECK(ntp_buf == NULL, MDF_FAIL, "aliyun_pack_ntp_request is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_NTP_REQUEST, ntp_buf, strlen(ntp_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_subdevice_request_ntp error");
JSON_FREE:
    MDF_FREE(ntp_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_get_dslttemplate(void)
{
    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *get_buf = aliyun_pack_thing_dsltemplate_get(msg_id);
    MDF_ERROR_CHECK(get_buf == NULL, MDF_FAIL, "aliyun_subdevice_get_dslttemplate is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_GET_DSLTEMPLATE, get_buf, strlen(get_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_subdevice_get_dslttemplate error");

JSON_FREE:
    MDF_FREE(get_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_get_desired_property(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *get_buf = aliyun_pack_thing_event_property_desired_get(msg_id, params);
    MDF_ERROR_CHECK(get_buf == NULL, MDF_FAIL, "aliyun_pack_thing_event_property_desired_get is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_GET_DESIRED_PROPERTY, get_buf, strlen(get_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(get_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_delete_desired_property(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));
    MDF_ERROR_CHECK(aliyun_platform_check_json(params) != MDF_OK, MDF_FAIL, "params not json");

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *delete_buf = aliyun_pack_thing_event_property_desired_get(msg_id, params);
    MDF_ERROR_CHECK(delete_buf == NULL, MDF_FAIL, "aliyun_subdevice_delete_desired_property is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_DELETE_DESIRED_PROPERTY, delete_buf, strlen(delete_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(delete_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_post_log(const char *params)
{
    MDF_PARAM_CHECK(params && strlen(params));

    mdf_err_t ret = MDF_FAIL;
    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *log_buf = aliyun_pack_thing_log_post(msg_id, params);
    MDF_ERROR_CHECK(log_buf == NULL, MDF_FAIL, "aliyun_pack_thing_log_post is NULL");

    ret = aliyun_platform_subdevice_write(ALIYUN_MQTT_PUBLISH_POST_LOG, log_buf, strlen(log_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_subdevice_post_log error");

JSON_FREE:
    MDF_FREE(log_buf);
    return ret;
}

static mdf_err_t _aliyun_subdevice_coap_bind_notify(void)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *notify_buf = aliyun_pack_coap_device_info_notify(msg_id);
    MDF_ERROR_CHECK(notify_buf == NULL, MDF_FAIL, "aliyun_pack_device_info_notify is NULL");

    MDF_LOGD("notify_buf:%s", notify_buf);

    ret = aliyun_platform_subdevice_write(ALIYUN_COAP_SYS_NOTIFY_DEVICE_INFO, notify_buf, strlen(notify_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(notify_buf);
    return ret;
}

static mdf_err_t _aliyun_subdevice_coap_connectapp_notify(void)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *notify_buf = aliyun_pack_coap_connectapp_notify(msg_id);
    MDF_ERROR_CHECK(notify_buf == NULL, MDF_FAIL, "aliyun_pack_device_info_notify is NULL");

    MDF_LOGD("notify_buf:%s", notify_buf);

    ret = aliyun_platform_subdevice_write(ALIYUN_COAP_SYS_EVENT_CONNECTAP_NOTIFY, notify_buf, strlen(notify_buf));
    MDF_ERROR_GOTO(ret != MDF_OK, JSON_FREE, "aliyun_platform_subdevice_write error");

JSON_FREE:
    MDF_FREE(notify_buf);
    return ret;
}

mdf_err_t aliyun_subdevice_coap_bind_notify(uint32_t cyclic_num)
{
    return aliyun_cyclic_config(ALIYUN_CYCLIC_TYPE_NOTIFY_BIND_TOKEN, cyclic_num, 1000);
}

mdf_err_t aliyun_subdevice_coap_connectapp_notify(uint32_t cyclic_num)
{
    return aliyun_cyclic_config(ALIYUN_CYCLIC_TYPE_NOTIFY_CONNETED_AP, cyclic_num, 1000);
}

#define ALIYUN_MQTT_SUBSCRIBE_TYPE_MAX 24

typedef struct {
    aliyun_msg_type_t type;
    void *func;
} aliyun_subdevice_msg_fun_t;

static aliyun_subdevice_msg_fun_t g_aliyun_subdevice_cb_list[ALIYUN_MQTT_SUBSCRIBE_TYPE_MAX] = {
    //TODO
    /*
        {ALIYUN_MQTT_SUBSCRIBE_GATEWAY_PERMIT,                    NULL},
        {ALIYUN_MQTT_SUBSCRIBE_SUB_REGISTER_REPLY,                NULL},
        {ALIYUN_MQTT_SUBSCRIBE_SUB_UNREGISTER_REPLY,              NULL},

        {ALIYUN_MQTT_SUBSCRIBE_DELETE_TOPO_REPLY,                 NULL},
        {ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_REPLY,                    NULL},
        {ALIYUN_MQTT_SUBSCRIBE_GET_TOPO_REPLY,                    NULL},

        {ALIYUN_MQTT_SUBSCRIBE_FOUND_LIST_REPLY,                  NULL},
        {ALIYUN_MQTT_SUBSCRIBE_ADD_TOPO_NOTIFY,                   NULL},

        {ALIYUN_MQTT_SUBSCRIBE_DISABLE,                           NULL},
        {ALIYUN_MQTT_SUBSCRIBE_ENABLE,                            NULL},
        {ALIYUN_MQTT_SUBSCRIBE_DELETE,                            NULL},*/

    { ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY, NULL },
    { ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY, NULL },
    { ALIYUN_SUBDEVICE_HEARTBEAT_REPLY, NULL },
    { ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY, NULL },
    { ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW, NULL },

    { ALIYUN_MQTT_POST_PROPERTY_REPLY, NULL },
    { ALIYUN_MQTT_PROPERTY_SET, NULL },

    { ALIYUN_MQTT_POST_EVENT_REPLY, NULL },
    { ALIYUN_MQTT_SERVICE_INVOKE, NULL },
    { ALIYUN_MQTT_SUBSCRIBE_EVENT_PROPERTY_PACK_POST_REPLY, NULL },

    { ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY, NULL },
    { ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY, NULL },

    { ALIYUN_MQTT_GET_CONFIG_REPLY, NULL },
    { ALIYUN_MQTT_CONFIG_PUSH, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_MODEL_CONFIG_PUSH, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_OTA_DEVICE_UPGRADE, NULL },

    { ALIYUN_MQTT_REQUEST_NTP_RESPONSE, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY, NULL },
    { ALIYUN_MQTT_SUBSCRIBE_GET_DYNAMICTSL_REPLY, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_GET_DESIRED_PROPERTY_REPLY, NULL },
    { ALIYUN_MQTT_SUBSCRIBE_DELETE_DESIRED_PROPERTY_REPLY, NULL },

    { ALIYUN_MQTT_SUBSCRIBE_RRPC_REQUEST_WILDCARD, NULL },
};

mdf_err_t aliyun_subdevice_set_callback(aliyun_msg_type_t type, const void *func)
{
    MDF_PARAM_CHECK(func);

    for (int i = 0; i < ALIYUN_MQTT_SUBSCRIBE_TYPE_MAX; i++) {
        if (g_aliyun_subdevice_cb_list[i].type == type) {
            g_aliyun_subdevice_cb_list[i].func = (void *)func;
            return MDF_OK;
        }
    }

    MDF_LOGW("aliyun subdevice subscribe list is full !!");
    return MDF_FAIL;
}

void *aliyun_subdevice_get_callback(aliyun_msg_type_t type)
{
    for (int i = 0; i < ALIYUN_MQTT_SUBSCRIBE_TYPE_MAX; i++) {
        if (g_aliyun_subdevice_cb_list[i].type == type) {
            return g_aliyun_subdevice_cb_list[i].func;
        }
    }

    MDF_LOGW("aliyun subdevice subscribe list not get type !!");
    return NULL;
}

mdf_err_t aliyun_subdevice_subscribe_callback(aliyun_msg_type_t type, const uint8_t *data, size_t data_len)
{
    MDF_PARAM_CHECK(data);

    uint32_t msg_id, code;
    char *params = NULL;
    mdf_err_t ret = MDF_OK;

    switch (type) {
        case ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY: {
            MDF_LOGD("ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY");
            aliyun_device_reply_t *reply_info = (aliyun_device_reply_t *)data;
            MDF_LOGI("product_key: %s, device_name: %s, status: %d", reply_info->product_key, reply_info->device_name, reply_info->status);
            g_subdevice_is_added = reply_info->status == MDF_OK ? true : false;

            if (reply_info->status == MDF_OK) {
                g_link_is_completed = true;
                g_parent_is_conneced = true;
                g_is_online = true;
                aliyun_platform_event_loop_send(ALIYUN_EVENT_ONLINE, NULL);
                aliyun_subdevice_add_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

                if (callback_fun != NULL) {
                    ret = callback_fun(reply_info);
                }
            }
        }
        break;

        case ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY: {
            MDF_LOGD("ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY");
            aliyun_device_reply_t *reply_info = (aliyun_device_reply_t *)data;
            MDF_LOGI("product_key: %s, device_name: %s, status: %d", reply_info->product_key, reply_info->device_name, reply_info->status);

            if (reply_info->status == MDF_OK) {
                if (g_is_online) {
                    aliyun_platform_event_loop_send(ALIYUN_EVENT_OFFLINE, NULL);
                    g_is_online = false;
                }

                aliyun_subdevice_delete_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

                if (callback_fun != NULL) {
                    ret = callback_fun(reply_info);
                }
            }
        }
        break;

        case ALIYUN_SUBDEVICE_HEARTBEAT_REPLY: {
            MDF_LOGD("ALIYUN_SUBDEVICE_HEARTBEAT_REPLY");
            MDF_LOGD("heartbeat: %d, utc: %ld", *(uint32_t *)data, time(NULL));

            if (*(mdf_err_t *)data == MDF_FAIL) {
                g_subdevice_to_gateway_status = ALIYUN_ANSWER_STATUS_FAIL;
                g_subdevice_is_added = false;
            }
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_RESET_REPLY: {
            MDF_LOGI("ALIYUN_MQTT_SUBSCRIBE_RESET_REPLY");
            ret = aliyun_parse_thing_reset_reply((char *)data, &msg_id, &code);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_parse_thing_reset_reply error");
            ret = aliyun_answer_check_timeout(ALIYUN_ANSWER_TYPE_UPDATE_RESET, msg_id);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_check_timeout discover msg_id timeout");
            aliyun_answer_set_success(ALIYUN_ANSWER_TYPE_UPDATE_RESET, msg_id);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_FOUND_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_FOUND_REPLY");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_MATCH_REPLY: {
            MDF_LOGI("ALIYUN_MQTT_SUBSCRIBE_AWSS_ENROLLEE_MATCH_REPLY");
            ret = aliyun_parse_thing_awss_enrollee_match_reply((char *)data, &msg_id, &code);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_parse_thing_reset_reply error");
            ret = aliyun_answer_check_timeout(ALIYUN_ANSWER_TYPE_UPDATE_TOKEN, msg_id);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_check_timeout discover msg_id timeout");
            aliyun_answer_set_success(ALIYUN_ANSWER_TYPE_UPDATE_TOKEN, msg_id);

            if (code == ALIYUN_SUCC_CODE) {
                MDF_LOGI("ALIYUN_ANSWER_STATUS_SUCC");

                if (aliyun_platform_get_is_gateway()) {
                    aliyun_coap_service_init();
                    MDF_LOGI("aliyun_coap_service_init ");
                }

                aliyun_subdevice_coap_bind_notify(10);
                MDF_LOGI("aliyun_subdevice_coap_bind_notify 10 ");
            } else {
                MDF_LOGI("ALIYUN_ANSWER_STATUS_FAIL");
            }
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_CIPHER_GET_REPLY: {
            MDF_LOGW("ALIYUN_MQTT_SUBSCRIBE_CIPHER_GET_REPLY");
        }
        break;

        case ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET: {
            MDF_LOGD("ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET");
            MDF_LOGI("data:%.*s", data_len, (char *)data);
            ret = aliyun_parse_gateway_reset_notify((char *)data);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_gateway_reset_notify error");
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY");

            ret = aliyun_parse_thing_model_up_raw_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_model_up_raw_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_SUBSCRIBE_MODEL_UP_RAW_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW");
            aliyun_subdevice_model_down_raw_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(data, data_len);
            } else {
                MDF_LOGW("ALIYUN_MQTT_SUBSCRIBE_MODEL_DOWN_RAW callback_fun is NULL");
            }
        }
        break;

        case ALIYUN_MQTT_POST_PROPERTY_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_POST_PROPERTY_REPLY");

            ret = aliyun_parse_thing_event_property_post_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_post_property_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_POST_PROPERTY_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_PROPERTY_SET: {
            MDF_LOGD("ALIYUN_MQTT_PROPERTY_SET");

            uint32_t msg_id;
            char *params = NULL;

            ret = aliyun_parse_thing_proprrty_set((char *)data, data_len, &msg_id, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_proprrty_set error");

            aliyun_subdevice_property_set_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, (const char *)params, strlen(params));
            } else {
                MDF_LOGW("ALIYUN_MQTT_PROPERTY_SET callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_POST_EVENT_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_POST_EVENT_REPLY");

            ret = aliyun_parse_thing_event_post_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_proprrty_set error");

            aliyun_subdevice_post_event_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_PROPERTY_SET callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SERVICE_INVOKE: {
            MDF_LOGD("ALIYUN_MQTT_SERVICE_INVOKE");

            char *identifier = NULL;

            ret = aliyun_parse_thing_service_request((char *)data, data_len, &msg_id, &identifier, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_proprrty_set error");

            aliyun_subdevice_service_invoke_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, remove_identifier_prefix(identifier), (const char *)params, strlen(params));
            } else {
                MDF_LOGW("ALIYUN_MQTT_SERVICE_INVOKE callback_fun is NULL");
            }

            MDF_FREE(identifier);
            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_EVENT_PROPERTY_PACK_POST_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_EVENT_PROPERTY_PACK_POST_REPLY");

            ret = aliyun_parse_thing_event_pack_post_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_proprrty_set error");

            aliyun_subdevice_pack_post_property_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_PROPERTY_SET callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY");

            ret = aliyun_parse_thing_deviceinfo_update_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_update_deviceinfo_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY");

            ret = aliyun_parse_thing_deviceinfo_delete_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_delete_deviceinfo_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_GET_CONFIG_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_GET_CONFIG_REPLY");

            ret = aliyun_parse_thing_config_get_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_get_config_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_CONFIG_PUSH: {
            MDF_LOGD("ALIYUN_MQTT_CONFIG_PUSH");

            ret = aliyun_parse_thing_config_push_reply((char *)data, data_len, &msg_id, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_config_push_reply error");

            aliyun_subdevice_config_push_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_MODEL_CONFIG_PUSH: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_MODEL_CONFIG_PUSH");
        }
        break;

        case ALIYUN_MQTT_REQUEST_NTP_RESPONSE: {
            MDF_LOGD("ALIYUN_MQTT_REQUEST_NTP_RESPONSE");
            ret = aliyun_parse_thing_ntp_response((char *)data, data_len);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_ntp_response error");

            aliyun_subdevice_ruquest_ntp_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(aliyun_platform_get_utc_ms());
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY");

            ret = aliyun_parse_thing_dsltemplate_get_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_get_dsltemplate_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_GET_DESIRED_PROPERTY_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_GET_DESIRED_PROPERTY_REPLY");

            ret = aliyun_parse_thing_property_desired_get_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_get_desired_property_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_DELETE_DESIRED_PROPERTY_REPLY: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_DELETE_DESIRED_PROPERTY_REPLY");

            ret = aliyun_parse_thing_property_desired_delete_reply((char *)data, data_len, &msg_id, &code, &params);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_event_property_post_reply error");

            aliyun_subdevice_get_desired_property_reply_cb_t callback_fun = aliyun_subdevice_get_callback(type);

            if (callback_fun != NULL) {
                ret = callback_fun(msg_id, code, (const char *)params);
            } else {
                MDF_LOGW("ALIYUN_MQTT_GET_CONFIG_REPLY callback_fun is NULL");
            }

            MDF_FREE(params);
        }
        break;

        case ALIYUN_MQTT_SUBSCRIBE_RRPC_REQUEST_WILDCARD: {
            MDF_LOGD("ALIYUN_MQTT_SUBSCRIBE_RRPC_REQUEST_WILDCARD");
        }
        break;

        case ALIYUN_PARENT_OFFLINE: {
            MDF_LOGD("ALIYUN_PARENT_OFFLINE");
            g_link_is_completed = false;

            if (g_is_online) {
                aliyun_platform_event_loop_send(ALIYUN_EVENT_OFFLINE, NULL);
                g_is_online = false;
            }
        }
        break;

        default: {
            MDF_LOGW("aliyun_linkkit_check_cb_t type is null, type: %u", type);
        }
        break;
    }

    return MDF_OK;
}

static mdf_err_t aliyun_subdevice_loop_process(void)
{
    mdf_err_t ret = MDF_FAIL;

    for (int i = 0; i < ALIYUN_ANSWER_TYPE_MAX; i++) {
        if (aliyun_answer_check_resends(i) != MDF_OK) {
            switch (i) {
                case ALIYUN_ANSWER_TYPE_UPDATE_TOKEN:
                    ret = aliyun_answer_set_resends_flag(ALIYUN_ANSWER_TYPE_UPDATE_TOKEN);
                    ret = aliyun_subdevice_match_enrollee();
                    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_match_enrollee error");
                    break;

                case ALIYUN_ANSWER_TYPE_UPDATE_RESET:
                    ret = aliyun_answer_set_resends_flag(ALIYUN_ANSWER_TYPE_UPDATE_RESET);
                    ret = aliyun_subdevice_reset();
                    MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_subdevice_reset error");
                    break;

                default:
                    MDF_LOGW("aliyun_answer_check_resends null");
                    break;
            }
        }
    }

    return MDF_OK;
}

static mdf_err_t aliyun_subdevice_cyclic_process(void)
{
    mdf_err_t ret = MDF_FAIL;

    for (int i = 0; i < ALIYUN_CYCLIC_TYPE_MAX; i++) {
        if (aliyun_cyclic_check_type(i) == MDF_OK) {
            switch (i) {
                case ALIYUN_CYCLIC_TYPE_NOTIFY_BIND_TOKEN:
                    ret = _aliyun_subdevice_coap_bind_notify();
                    MDF_ERROR_CHECK(ret != MDF_OK, ret, "_aliyun_subdevice_coap_bind_notify error");
                    break;

                case ALIYUN_CYCLIC_TYPE_NOTIFY_CONNETED_AP:
                    ret = _aliyun_subdevice_coap_connectapp_notify();
                    MDF_ERROR_CHECK(ret != MDF_OK, ret, "_aliyun_subdevice_coap_connectapp_notify error");
                    break;

                default:
                    break;
            }
        }
    }

    return MDF_OK;
}

static void aliyun_subdevice_task(void *arg)
{
    mdf_err_t ret = MDF_OK;

    uint8_t *payload = NULL;
    size_t length = 0;

    aliyun_msg_type_t type = 0;
    int64_t heartbeat_interval = (esp_random() % 60 + HEART_BEAT_INTERVAL) * 1000000 + esp_timer_get_time();
    int64_t subdevice_add_tick = 0;
    int64_t subdevice_add_timeout = SUBDEVICE_ADD_GATEWAY_TIMEOUT;
    bool random_delay_flag = true;

    aliyun_device_meta_t subdevice_meta = { 0 };
    aliyun_get_meta(&subdevice_meta);
    MDF_LOGI("product_key: %s", subdevice_meta.product_key);
    MDF_LOGI("device_name: %s", subdevice_meta.device_name);

    MDF_LOGI("aliyun_subdevice_task is running");

    while (g_aliyun_subdevice_task_handle != NULL) {
        if (!g_subdevice_is_added && esp_timer_get_time() - subdevice_add_tick > subdevice_add_timeout) {
            if (random_delay_flag) {
                subdevice_add_tick = esp_timer_get_time() - SUBDEVICE_ADD_GATEWAY_TIMEOUT + (esp_random() % 30) * 1000 * 1000;
                random_delay_flag = false;
            } else {
                if ((ret = aliyun_subdevice_add_to_gateway(&subdevice_meta)) == MDF_OK) {
                    subdevice_add_timeout = SUBDEVICE_ADD_GATEWAY_TIMEOUT;
                } else {
                    MDF_LOGI("subdevice add to gateway error, reason: %s", mdf_err_to_name(ret));
                    subdevice_add_timeout = SUBDEVICE_ADD_GATEWAY_ERROR_TIMEOUT;
                }

                subdevice_add_tick = esp_timer_get_time();
            }
        } else if (g_subdevice_is_added) {
            random_delay_flag = true;
        }

        if (g_inform_link_loss) {
            g_inform_link_loss = false;
            subdevice_inform_link_loss();
        }

        ret = aliyun_platform_subdevice_read(&type, (void **)&payload, &length, CONFIG_ALIYUN_READ_TIMROUT_MS / portTICK_RATE_MS);

        if (ret == MDF_OK) {
            aliyun_subdevice_subscribe_callback(type, payload, length);
        } else if (ret == ESP_ERR_MESH_TIMEOUT || ret == MDF_ERR_TIMEOUT) {
            aliyun_subdevice_loop_process();
            aliyun_subdevice_cyclic_process();

            if (esp_timer_get_time() > heartbeat_interval && aliyun_platform_get_cloud_connect_status()) {
                ret = aliyun_subdevice_heartbeat_to_gateway();

                if (ret != MDF_OK) {
                    MDF_LOGW(" Send heartbeat to gateway failed (%d)", ret);
                }

                heartbeat_interval = (HEART_BEAT_INTERVAL + esp_random() % 60) * 1000000 + esp_timer_get_time();
            }
        } else {
            MDF_LOGW("aliyun_subdevice_read error, ret: 0x%x", ret);
        }

        MDF_FREE(payload);
    }

    MDF_LOGW("aliyun_subdevice_task is exit");
    vTaskDelete(NULL);
}

mdf_err_t aliyun_subdevice_init(void)
{
    if (g_aliyun_subdevice_task_handle == NULL) {
        xTaskCreate(aliyun_subdevice_task, "aliyun_subdevice_task", 4 * 1024,
                    NULL, CONFIG_ALIYUN_TASK_DEFAULT_PRIOTY, &g_aliyun_subdevice_task_handle);
    }

    return MDF_OK;
}

mdf_err_t aliyun_subdevice_deinit(void)
{
    if (g_aliyun_subdevice_task_handle != NULL) {
        g_aliyun_subdevice_task_handle = NULL;
    }

    return MDF_OK;
}
