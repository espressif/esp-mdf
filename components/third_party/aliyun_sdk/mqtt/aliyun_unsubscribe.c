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

#include "aliyun_unsubscribe.h"
#include "aliyun_mqtt.h"
#include "aliyun_topic.h"
#include "esp_system.h"

static const char *TAG = "aliyun_unsubscribe";

static int aliyun_unsubscribe_thing_event_post_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_EVENT_POST_REPLY_WILDCARD, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_property_desired_delete_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}
static int aliyun_unsubscribe_thing_property_desired_get_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_service_propety_set(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_service_request_wildcard(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_SERVICE_REQUEST_WILDCARD, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_deviceinfo_update_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_deviceinfo_delete_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_DEVICEINFO_DELETE_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_dynamicTsl_get_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_DYNAMICTSL_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_rrpc_request_wildcard(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_RRPC_REQUEST_WILDCARD, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_ntp_response(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_EXT_NTP_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_NTP_RESPONSE, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_error(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    snprintf(topic_buf, topic_size, ALIYUN_TOPIC_EXT_ERROR_PREFIX, product_key, device_name);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_model_down_raw(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_MODEL_DOWN_RAW, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_model_up_raw_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_MODEL_UP_RAW_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_topo_add_notify(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_TOPO_ADD_NOTIFY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_gateway_permit(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_GATEWAY_PERMIT, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_sub_register_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_SUB_REGISTER_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_sub_unregister_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_SUB_UNREGISTER_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_topo_add_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_TOPO_ADD_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_topo_delete_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_TOPO_DELETE_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_topo_get_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_TOPO_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_list_found_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_LIST_FOUND_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_session_combine_login_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_EXT_SESSION_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_COMBINE_LOGIN_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_session_combine_logout_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_EXT_SESSION_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_COMBINE_LOGOUT_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_disable(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_DISABLE, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_enable(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_ENABLE, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_delete(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_DELETE, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_ota_device_request(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    snprintf(topic_buf, topic_size, ALIYUN_TOPIC_OTA_DEVICE_REQUEST, product_key, device_name);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_ota_device_upgrade(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    snprintf(topic_buf, topic_size, ALIYUN_TOPIC_OTA_DEVICE_UPGRADE, product_key, device_name);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_config_get_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_CONFIG_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_config_push(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_CONFIG_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_reset_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_RESET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_awss_enrollee_match_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_awss_enrollee_checkin(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_CHECKIN, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_awss_enrollee_found_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_FOUND_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_cipher_get_reply(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_CIPHER_GET_REPLY, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

static int aliyun_unsubscribe_thing_awss_device_switchap(char *product_key, char *device_name, char *topic_buf, size_t topic_size)
{
    bzero(topic_buf, topic_size);
    int len = snprintf(topic_buf, topic_size, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(topic_buf, ALIYUN_TOPIC_THING_AWSS_DEVICE_SWITCHAP, topic_size - len - 1);
    MDF_LOGD("aliyun_mqtt_unsubscribe topic: %s", topic_buf);
    return aliyun_mqtt_unsubscribe(topic_buf);
}

mdf_err_t aliyun_unsubscribe_subdevice_all_topic(aliyun_device_meta_t *meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_OK;

    ret = aliyun_unsubscribe_thing_event_post_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_property_desired_delete_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_property_desired_get_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_service_propety_set(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_service_request_wildcard(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_deviceinfo_update_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_deviceinfo_delete_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_dynamicTsl_get_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_rrpc_request_wildcard(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_ntp_response(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_error(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_model_down_raw(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_model_up_raw_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_ota_device_request(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_ota_device_upgrade(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_config_get_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_config_push(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_reset_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_awss_enrollee_match_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_enrollee_checkin(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_enrollee_found_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_cipher_get_reply(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_device_switchap(meta->product_key, meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    return MDF_OK;
}

mdf_err_t aliyun_unsubscribe_gateway_all_topic(aliyun_device_meta_t *gateway_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_OK;

    ret = aliyun_unsubscribe_thing_property_desired_delete_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_property_desired_get_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_service_propety_set(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_service_request_wildcard(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_deviceinfo_update_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_deviceinfo_delete_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_dynamicTsl_get_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_rrpc_request_wildcard(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_ntp_response(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_error(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_model_down_raw(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_model_up_raw_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_topo_add_notify(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_gateway_permit(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_sub_register_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_sub_unregister_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_topo_add_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_topo_delete_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_topo_get_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_list_found_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_session_combine_login_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_session_combine_logout_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_disable(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_enable(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_delete(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_ota_device_request(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_ota_device_upgrade(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_config_get_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_config_push(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_reset_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    ret = aliyun_unsubscribe_thing_awss_enrollee_match_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_enrollee_checkin(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_enrollee_found_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_cipher_get_reply(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);
    ret = aliyun_unsubscribe_thing_awss_device_switchap(gateway_meta->product_key, gateway_meta->device_name, buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);
    MDF_LOGD("ret: %d", ret);

    return MDF_OK;
}