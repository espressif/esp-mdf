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

#include "aliyun_publish.h"
#include "aliyun_mqtt.h"
#include "aliyun_parse.h"
#include "aliyun_topic.h"
#include "cJSON.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_publish";

mdf_err_t aliyun_publish_sub_topo_add(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_TOPO_ADD, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);

    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_sub_topo_delete(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_TOPO_DELETE, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_sub_combine_login(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_EXT_SESSION_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_COMBINE_LOGIN, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_sub_combine_logout(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_EXT_SESSION_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_COMBINE_LOGOUT, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_awss_enrollee_match(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_AWSS_ENROLLEE_MATCH, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_reset(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_RESET, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_cipher_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_CIPHER_GET, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_event_property_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_EVENT_PROPERTY_POST, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_service_property_set_reply(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_SERVICE_PROPERTY_SET_REPLY, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_event_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    char *identifier = NULL;
    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    aliyun_parse_thing_event_post_identifier((char *)buffer->payload, buffer->payload_len, &identifier);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);

    char *str = strchr(identifier, '.');
    assert(str++ != NULL);
    str = strchr(str, '.');
    assert(str++ != NULL);
    char *over = strchr(str, '.');
    assert(over != NULL);
    *over = '\0';
    snprintf(buffer->topic + len, CONFIG_ALIYUN_TOPIC_SIZE - len, ALIYUN_TOPIC_THING_EVENT_POST, str);

    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);
    MDF_FREE(identifier);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_service_response(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    cJSON *json = cJSON_Parse((const char *)buffer->payload);
    cJSON *identifier = cJSON_GetObjectItem(json, "identifier");
    assert(identifier != NULL && cJSON_IsString(identifier));
    snprintf(buffer->topic + len, CONFIG_ALIYUN_PAYLOAD_SIZE - len - 1, ALIYUN_TOPIC_THING_SERVICE_RESPONSE, identifier->valuestring);
    cJSON_Delete(json);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_deviceinfo_update(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_DEVICEINFO_UPDATE, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_deviceinfo_delete(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_DEVICEINFO_DELETE, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_config_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_CONFIG_GET, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_config_push_reply(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_CONFIG_PUSH_REPLY, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_ota_device_inform(const char *product_key, const char *device_name,
        aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_OTA_DEVICE_INFORM, product_key, device_name);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_ota_device_request(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(buffer);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_OTA_DEVICE_REQUEST, product_key, device_name);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_ota_device_progress(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_OTA_DEVICE_PROGRESS, product_key, device_name);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_ntp_request(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_EXT_NTP_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_NTP_REQUEST, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_dsltemplate_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_DSLTEMPLATE_GET, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("Topic:%s", buffer->topic);
    MDF_LOGD("buffer->payload:%s", (char *)buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_property_desired_get(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_GET, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_property_desired_delete(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_PROPERTY_DESIRED_DELETE, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}

mdf_err_t aliyun_publish_thing_log_post(const char *product_key, const char *device_name, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    bzero(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE);

    int len = snprintf(buffer->topic, CONFIG_ALIYUN_TOPIC_SIZE, ALIYUN_TOPIC_SYS_PREFIX, product_key, device_name);
    assert(len > 0);
    strncat(buffer->topic, ALIYUN_TOPIC_THING_LOG_POST, CONFIG_ALIYUN_TOPIC_SIZE - len - 1);
    MDF_LOGD("topic: %s", buffer->topic);
    MDF_LOGD("payload: %.*s", buffer->payload_len, buffer->payload);

    return aliyun_mqtt_publish(buffer->topic, buffer->payload, buffer->payload_len, ALIYUN_MQTT_QOS0);
}
