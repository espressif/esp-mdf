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


#include "esp_http_client.h"

#include "mdf_mem.h"
#include "aliyun_sign.h"
#include "aliyun_host.h"
#include "aliyun_platform.h"

static const char *TAG                      = "aliyun_sign";
extern const char aliyun_cert_pem_start[] asm("_binary_aliyun_cert_pem_start");
extern const char aliyun_cert_pem_end[]   asm("_binary_aliyun_cert_pem_end");


/* client_id key value pair define */
const char *client_id_kv[][2] = {
    {
        "timestamp", ALIYUN_TIMESTAMP_VALUE
    },
    {
        "_v", "sdk-c-"ALIYUN_SDK_VERSION
    },
    {
        "securemode", ALIYUN_SECURE_MODE
    },
    {
        "signmethod", "hmacsha1"
    },
    {
        "lan", "C"
    },
#if defined(DEVICE_MODEL_ENABLED) && !defined(DEVICE_MODEL_CLASSIC)
    {
        "v", ALIYUN_ALINK_VERSION
    },
#else
    {
        "gw", "0"
    },
    {
        "ext", "0"
    },
#endif
};

mdf_err_t aliyun_sign_get_client_id(char *client_id, const char *device_id)
{
    MDF_PARAM_CHECK(client_id);
    MDF_PARAM_CHECK(device_id);

    memcpy(client_id, device_id, strlen(device_id));
    memcpy(client_id + strlen(client_id), "|", 1);

    for (uint8_t i = 0; i < (sizeof(client_id_kv) / (sizeof(client_id_kv[0]))); i++) {
        if ((strlen(client_id) + strlen(client_id_kv[i][0]) + strlen(client_id_kv[i][1]) + 2) >=
                ALIYUN_SIGN_CLIENT_ID_MAXLEN) {
            return MDF_FAIL;
        }

        memcpy(client_id + strlen(client_id), client_id_kv[i][0], strlen(client_id_kv[i][0]));
        memcpy(client_id + strlen(client_id), "=", 1);
        memcpy(client_id + strlen(client_id), client_id_kv[i][1], strlen(client_id_kv[i][1]));
        memcpy(client_id + strlen(client_id), ",", 1);
    }

    memcpy(client_id + strlen(client_id) - 1, "|", 1);

    return MDF_OK;
}

mdf_err_t aliyun_generate_sign_string(const char *device_id, const char *device_name, const char *product_key, const char *device_secret, char *sign_string)
{
    const char sign_fmt[] = "clientId%sdeviceName%sproductKey%stimestamp%s";
    char signsource[ALIYUN_SIGN_SOURCE_MAXLEN] = {0};

    MDF_ERROR_CHECK(sizeof(sign_fmt) + strlen(device_id) + strlen(device_name) + strlen(product_key) + strlen(ALIYUN_TIMESTAMP_VALUE) > ALIYUN_SIGN_SOURCE_MAXLEN - 1, MDF_ERR_INVALID_SIZE, "aliyun sign signsource more than the size");

    memset(signsource, 0, ALIYUN_SIGN_SOURCE_MAXLEN);
    memcpy(signsource, "clientId", strlen("clientId"));
    memcpy(signsource + strlen(signsource), device_id, strlen(device_id));
    memcpy(signsource + strlen(signsource), "deviceName", strlen("deviceName"));
    memcpy(signsource + strlen(signsource), device_name, strlen(device_name));
    memcpy(signsource + strlen(signsource), "productKey", strlen("productKey"));
    memcpy(signsource + strlen(signsource), product_key, strlen(product_key));
    memcpy(signsource + strlen(signsource), "timestamp", strlen("timestamp"));
    memcpy(signsource + strlen(signsource), ALIYUN_TIMESTAMP_VALUE, strlen(ALIYUN_TIMESTAMP_VALUE));

    aliyun_platform_hmacsha1(signsource, strlen(signsource), sign_string, device_secret, strlen(device_secret));

    return MDF_OK;
}

mdf_err_t aliyun_sub_generate_sign_string(const char *device_id, const char *device_name, const char *product_key, const char *device_secret, char *sign_string)
{
    const char sign_fmt[] = "clientId%sdeviceName%sproductKey%stimestamp%s";
    char signsource[ALIYUN_SIGN_SOURCE_MAXLEN] = {0};

    MDF_ERROR_CHECK(sizeof(sign_fmt) + strlen(device_id) + strlen(device_name) + strlen(product_key) + strlen(ALIYUN_TIMESTAMP_VALUE) > ALIYUN_SIGN_SOURCE_MAXLEN - 1, MDF_ERR_INVALID_SIZE, "aliyun sign signsource more than the size");

    memset(signsource, 0, ALIYUN_SIGN_SOURCE_MAXLEN);
    memcpy(signsource, "clientId", strlen("clientId"));
    memcpy(signsource + strlen(signsource), device_id, strlen(device_id));
    memcpy(signsource + strlen(signsource), "deviceName", strlen("deviceName"));
    memcpy(signsource + strlen(signsource), device_name, strlen(device_name));
    memcpy(signsource + strlen(signsource), "productKey", strlen("productKey"));
    memcpy(signsource + strlen(signsource), product_key, strlen(product_key));
    memcpy(signsource + strlen(signsource), "timestamp", strlen("timestamp"));
    memcpy(signsource + strlen(signsource), ALIYUN_TIMESTAMP_VALUE, strlen(ALIYUN_TIMESTAMP_VALUE));

    aliyun_platform_hmacsha1(signsource, strlen(signsource), sign_string, device_secret, strlen(device_secret));

    return MDF_OK;
}

mdf_err_t aliyun_sign_config(const aliyun_device_meta_t *meta, aliyun_sub_sign_info_t *sign_info)
{
    mdf_err_t ret = MDF_FAIL;

    bzero(sign_info->client_id, ALIYUN_SIGN_CLIENT_ID_MAXLEN);
    sprintf(sign_info->client_id, "%s.%s", meta->product_key, meta->device_name);

    bzero(sign_info->product_key, ALIYUN_PRODUCT_KEY_MAXLEN);
    strncpy(sign_info->product_key, meta->product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);

    bzero(sign_info->device_name, ALIYUN_DEVICE_NAME_MAXLEN);
    strncpy(sign_info->device_name, meta->device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);

    bzero(sign_info->timestamp, ALIYUN_TIMESTAMP_MAXLEN);
    strncpy(sign_info->timestamp, ALIYUN_TIMESTAMP_VALUE, ALIYUN_TIMESTAMP_MAXLEN - 1);

    bzero(sign_info->sign, ALIYUN_SIGN_PASSWORD_MAXLEN);
    ret = aliyun_sub_generate_sign_string(sign_info->client_id, meta->device_name, meta->product_key, meta->device_secret, sign_info->sign);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun sign client_id error");

    bzero(sign_info->sign_method, ALIYUN_SIGN_METHOD_MAXLEN);
    strcpy(sign_info->sign_method, "hmacSha1");

    return MDF_OK;
}

mdf_err_t aliyun_sign_mqtt_config(aliyun_mqtt_region_types_t region, const aliyun_device_meta_t *meta, aliyun_mqtt_config_t *mqtt_config)
{
    MDF_PARAM_CHECK(g_aliyun_mqtt_domain[region]);

    if (region >= ALIYUN_MQTT_DOMAIN_MAX || meta == NULL || mqtt_config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    mdf_err_t ret = MDF_FAIL;

    char device_id[ALIYUN_SIGN_DEVICEID_MAXLEN] = {0};
    bzero(device_id, ALIYUN_SIGN_DEVICEID_MAXLEN);
    strncpy(device_id, meta->product_key, ALIYUN_SIGN_DEVICEID_MAXLEN);
    strncpy(device_id + strlen(device_id), ".", ALIYUN_SIGN_DEVICEID_MAXLEN - strlen(device_id));
    strncpy(device_id + strlen(device_id), meta->device_name, ALIYUN_SIGN_DEVICEID_MAXLEN - strlen(device_id));

    bzero(mqtt_config->client_id, ALIYUN_SIGN_CLIENT_ID_MAXLEN);
    ret = aliyun_sign_get_client_id(mqtt_config->client_id, device_id);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun sign client_id error");

    bzero(mqtt_config->host, ALIYUN_SIGN_HOSTNAME_MAXLEN);

#ifdef CONFIG_ALIYUN_MQTT_MODE_PRE_AUTH
    strncpy(mqtt_config->host, g_aliyun_mqtt_domain[region], ALIYUN_SIGN_HOSTNAME_MAXLEN);
#else
    strncpy(mqtt_config->host, meta->product_key, ALIYUN_SIGN_HOSTNAME_MAXLEN - 1);
    strncpy(mqtt_config->host + strlen(mqtt_config->host), ".", ALIYUN_SIGN_HOSTNAME_MAXLEN - 1 - strlen(mqtt_config->host));
    strncpy(mqtt_config->host + strlen(mqtt_config->host), g_aliyun_mqtt_domain[region], ALIYUN_SIGN_HOSTNAME_MAXLEN - 1 - strlen(mqtt_config->host));

    MDF_ERROR_CHECK(strlen(meta->device_name) + strlen(meta->product_key) + 2 > ALIYUN_SIGN_USERNAME_MAXLEN, MDF_ERR_INVALID_SIZE, "aliyun sign username more than the size");

    bzero(mqtt_config->username, ALIYUN_SIGN_USERNAME_MAXLEN);
    strncpy(mqtt_config->username, meta->device_name, ALIYUN_SIGN_USERNAME_MAXLEN - 1);
    strncpy(mqtt_config->username + strlen(mqtt_config->username), "&", ALIYUN_SIGN_USERNAME_MAXLEN - 1 - strlen(mqtt_config->username));
    strncpy(mqtt_config->username + strlen(mqtt_config->username), meta->product_key, ALIYUN_SIGN_USERNAME_MAXLEN - 1 - strlen(mqtt_config->username));

    bzero(mqtt_config->password, ALIYUN_SIGN_PASSWORD_MAXLEN);
    ret = aliyun_generate_sign_string(device_id, meta->device_name, meta->product_key, meta->device_secret, mqtt_config->password);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun sign client_id error");
#endif

#ifdef CONFIG_ALIYUN_NETWORK_SUPPORT_WS
    mqtt_config->port = CONFIG_ALIYUN_MQTT_WS_WSS_PORT;
#ifdef CONFIG_ALIYUN_NETWORK_SUPPORT_TLS
    mqtt_config->transport = ALIYUN_TRANSPORT_OVER_WSS;
    mqtt_config->server_cert = aliyun_cert_pem_start;
#else
    mqtt_config->transport = ALIYUN_TRANSPORT_OVER_WS;
#endif
#else
    mqtt_config->port = CONFIG_ALIYUN_MQTT_TCP_TLS_PORT;
#ifdef CONFIG_ALIYUN_NETWORK_SUPPORT_TLS
    mqtt_config->transport = ALIYUN_TRANSPORT_OVER_SSL;
    mqtt_config->server_cert = aliyun_cert_pem_start;
#else
    mqtt_config->transport = ALIYUN_TRANSPORT_OVER_TCP;
#endif
#endif /* #ifdef ALIYUN_NETWORK_SUPPORT_TLS */

    mqtt_config->gateway_meta = (aliyun_device_meta_t *)meta;

    return MDF_OK;
}

mdf_err_t aliyun_sign_register_config(const char *product_key, const char *product_secret, char *register_buf, size_t size)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(product_secret);
    MDF_PARAM_CHECK(register_buf);

    char sign[ALIYUN_SIGN_PASSWORD_MAXLEN] = {0};
    char mac_str[ALIYUN_MAC_ADDRS_MAXLEN_STR] = {0};
    uint32_t random = esp_random();

    aliyun_platform_get_device_mac_str(mac_str);

    bzero(register_buf, size);
    sprintf(register_buf, "deviceName%sproductKey%srandom%u", mac_str, product_key, random);

    aliyun_platform_hmacsha1(register_buf, strlen(register_buf), sign, product_secret, strlen(product_secret));

    bzero(register_buf, size);
    sprintf(register_buf, "productKey=%s&deviceName=%s&random=%u&sign=%s&signMethod=%s", product_key, mac_str, random, sign, "hmacSha1");
    return MDF_OK;
}


mdf_err_t aliyun_auth_register_device(aliyun_mqtt_region_types_t region, const char *product_key, const char *product_secret)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(product_secret);

    mdf_err_t ret    = MDF_OK;
    char    *register_buf       = MDF_MALLOC(512);

    esp_http_client_config_t config = {
        .url = "https://iot-auth.cn-shanghai.aliyuncs.com/auth/register/device",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");

    ret = aliyun_sign_register_config(product_key, product_secret, register_buf, 512);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "aliyun_sign_register_config error");

    MDF_LOGI("register_buf:%s", register_buf);

    if (region != ALIYUN_MQTT_REGION_SHANGHAI) {
        char    *register_url       = MDF_MALLOC(128);
        bzero(register_url, 128);
        sprintf(register_url, "https://%s/auth/register/device", g_aliyun_http_domain[region]);
        esp_http_client_set_url(client, register_url);
        MDF_FREE(register_url);
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, register_buf, strlen(register_buf));

    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        MDF_LOGI("HTTPS Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));

        if (!esp_http_client_is_chunked_response(client)) {
            uint32_t recv_len = esp_http_client_read(client, register_buf, 512);
            MDF_LOGI("esp_http_client_read: %.*s", recv_len, register_buf);
        }
    } else {
        MDF_LOGE("Error perform http request %s", esp_err_to_name(ret));
    }

EXIT:
    MDF_FREE(register_buf);

    if (client != NULL) {
        esp_http_client_cleanup(client);
    }

    return ret;
}
