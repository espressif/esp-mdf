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

#include <arpa/inet.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "esp_wifi.h"

#include "mbedtls/sha1.h"

#include "aliyun_platform.h"

static const char *TAG                          = "aliyun_platform";

static QueueHandle_t g_platform_mqtt_queue = NULL;

#define KEY_IOPAD_SIZE          (64)
#define SHA1_DIGEST_SIZE        (20)
#define MQTT_MSG_QUEUE (10)

mdf_err_t aliyun_platform_check_json(const char *json)
{
    cJSON *pJson = cJSON_Parse(json);

    if (pJson == NULL) {
        return MDF_FAIL;
    } else {
        cJSON_Delete(pJson);
        return MDF_OK;
    }
}

void aliyun_hexbuf_convert(uint8_t *digest, char *out, int in_len, bool uppercase)
{
    static char    *zEncode[] = {"0123456789abcdef", "0123456789ABCDEF"};
    int             j = 0;
    int             i = 0;
    int             idx = uppercase ? 1 : 0;

    for (i = 0; i < in_len; i ++) {
        uint32_t   a = digest[i];
        out[j++] = zEncode[idx][(a >> 4) & 0xf];
        out[j++] = zEncode[idx][a & 0xf];
    }
}

mdf_err_t aliyun_platform_hmacsha1(const char *msg, int msg_len, char *digest, const char *key, int key_len)
{
    MDF_PARAM_CHECK(msg);
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(digest);

    mbedtls_sha1_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[SHA1_DIGEST_SIZE];

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (int i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    mbedtls_sha1_init(&context);                                      /* init context for 1st pass */
    mbedtls_sha1_starts(&context);                                    /* setup context for 1st pass */
    mbedtls_sha1_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    mbedtls_sha1_update(&context, (unsigned char *) msg, msg_len);    /* then text of datagram */
    mbedtls_sha1_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer SHA */
    mbedtls_sha1_init(&context);                              /* init context for 2nd pass */
    mbedtls_sha1_starts(&context);                            /* setup context for 2nd pass */
    mbedtls_sha1_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    mbedtls_sha1_update(&context, out, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
    mbedtls_sha1_finish(&context, out);                       /* finish up 2nd pass */

    aliyun_hexbuf_convert(out, digest, SHA1_DIGEST_SIZE, true);
    return MDF_OK;
}

uint32_t aliyun_platform_get_msg_id(void)
{
    static uint32_t aliyun_msg_id = 1;
    return aliyun_msg_id ++;
}

int64_t aliyun_platform_get_utc_ms(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((int64_t)now.tv_sec * 1000 + (int64_t)now.tv_usec / 1000);
}

mdf_err_t aliyun_platform_set_utc_ms(int64_t utc_ms)
{
    struct timeval tv = {
        .tv_sec = utc_ms / 1000,
        .tv_usec = utc_ms % 1000,
    };
    /**< Set timezone to China Standard Time */
    setenv("TZ", "CST-8", 1);
    tzset();

    if (settimeofday(&tv, NULL) == ESP_OK) {
        return MDF_OK;
    } else {
        return MDF_FAIL;
    }
}

mdf_err_t aliyun_platform_get_ip_str(char ip_str[ALIYUN_NETWORK_ADDR_MAXLEN_STR])
{
    MDF_PARAM_CHECK(ip_str);

    mdf_err_t ret = MDF_OK;
    tcpip_adapter_ip_info_t info;

    ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "tcpip_adapter_get_ip_info error");

    return !strncpy(ip_str, inet_ntoa(info.ip.addr), ALIYUN_NETWORK_ADDR_MAXLEN_STR) ? MDF_OK : MDF_FAIL;
}

mdf_err_t aliyun_platform_get_device_mac(uint8_t mac[ALIYUN_MAC_ADDRS_MAXLEN])
{
    MDF_PARAM_CHECK(mac);

    mdf_err_t ret = MDF_OK;

    ret = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_wifi_get_mac error");

    return MDF_OK;
}

mdf_err_t aliyun_platform_get_device_mac_str(char mac_str[ALIYUN_MAC_ADDRS_MAXLEN_STR])
{
    MDF_PARAM_CHECK(mac_str);

    mdf_err_t ret = MDF_OK;
    uint8_t mac[ALIYUN_MAC_ADDRS_MAXLEN] = {0};

    ret = esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_wifi_get_mac error");

    return sprintf(mac_str, MACSTR, MAC2STR(mac)) > 0  ? MDF_OK : MDF_FAIL;
}

mdf_err_t aliyun_platform_get_token_str(char token[ALIYUN_TOKEN_ADDR_MAXLEN_STR])
{
    MDF_PARAM_CHECK(token);
    static char default_token[ALIYUN_TOKEN_ADDR_MAXLEN_STR] = {0};

    if (default_token[0] != 0 && default_token[1] != 0) {
        strncpy(token, default_token, ALIYUN_TOKEN_ADDR_MAXLEN_STR);
        return MDF_OK;
    }

    uint8_t random[16] = {0};

    if (aliyun_platform_get_is_gateway()) {
        random[0] = 0xFF;
    } else {
        do {
            random[0] = esp_random() % 0xff;
        } while (random[0] == 0xFF);
    }

    for (int i = 1; i < 16; i++) {
        random[i] = esp_random() % 0xff;
    }

    aliyun_hexbuf_convert(random, default_token, 16, true);
    strncpy(token, default_token, ALIYUN_TOKEN_ADDR_MAXLEN_STR);
    return MDF_OK;
}

mdf_err_t aliyun_platform_mqtt_init(void)
{

#ifndef CONFIG_ALIYUN_PLATFORM_MDF
    aliyun_idf_gateway_init();
#endif

    if (g_platform_mqtt_queue == NULL) {
        g_platform_mqtt_queue = xQueueCreate(MQTT_MSG_QUEUE, sizeof(aliyun_buffer_t *));
    }

    return MDF_OK;
}

mdf_err_t aliyun_platform_mqtt_deinit(void)
{
#ifndef CONFIG_ALIYUN_PLATFORM_MDF
    aliyun_idf_gateway_deinit();
#endif

    if (g_platform_mqtt_queue != NULL) {
        vQueueDelete(g_platform_mqtt_queue);
        g_platform_mqtt_queue = NULL;
    }

    return MDF_OK;
}

mdf_err_t aliyun_platform_mqtt_read(aliyun_buffer_t **mqtt_buf, uint32_t wait_ticks)
{
    return xQueueReceive(g_platform_mqtt_queue, mqtt_buf, wait_ticks) != pdTRUE ? MDF_FAIL : MDF_OK;
}

mdf_err_t aliyun_platform_mqtt_write(const aliyun_buffer_t *mqtt_buf, uint32_t wait_ticks)
{
    return xQueueSend(g_platform_mqtt_queue, &mqtt_buf, wait_ticks) != pdTRUE ? MDF_FAIL : MDF_OK;
}
