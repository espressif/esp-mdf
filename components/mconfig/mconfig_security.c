/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"

#include "mdf_common.h"
#include "mconfig_security.h"

static const char *TAG                  = "mconfig_security";
static QueueHandle_t g_rsa_result_queue = NULL;
static char *g_pubkey_pem               = NULL;
static char *g_privkey_pem              = NULL;

mdf_err_t mconfig_random(void *rng_state, uint8_t *output, size_t len)
{
    MDF_PARAM_CHECK(output);
    MDF_PARAM_CHECK(len);

    while (len--) {
        *output++ = esp_random();
    }

    return MDF_OK;
}

mdf_err_t mconfig_dhm_gen_key(uint8_t *param, ssize_t param_size,
                              uint8_t *privkey, uint8_t *pubkey)
{
    MDF_PARAM_CHECK(param);
    MDF_PARAM_CHECK(param_size > 0);
    MDF_PARAM_CHECK(privkey);
    MDF_PARAM_CHECK(pubkey);

    mdf_err_t ret           = MDF_OK;
    mbedtls_dhm_context dhm = {0};
    size_t privkey_size     = 0;

    mbedtls_dhm_init(&dhm);

    ret = mbedtls_dhm_read_params(&dhm, &param, param + param_size);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_dhm_read_params, ret: 0x%x", -ret);

    ret = mbedtls_dhm_make_public(&dhm, (int) mbedtls_mpi_size(&dhm.P),
                                  pubkey, dhm.len, mconfig_random, NULL);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_dhm_make_public, ret: 0x%x", -ret);

    ret = mbedtls_dhm_calc_secret(&dhm, privkey, MCONFIG_DH_PRIVKEY_LEN * 8,
                                  &privkey_size, NULL, NULL);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_dhm_make_public, ret: 0x%x", -ret);

EXIT:
    mbedtls_dhm_free(&dhm);
    return ret;
}

static void mconfig_rsa_gen_key_task(void *arg)
{
    mdf_err_t ret           = MDF_OK;
    mbedtls_rsa_context rsa = {0};
    mbedtls_pk_context pk   = {0};
    char *privkey_pem       = g_privkey_pem;
    char *pubkey_pem        = g_pubkey_pem;

    memset(privkey_pem, 0, MCONFIG_RSA_PRIVKEY_PEM_SIZE);
    memset(pubkey_pem, 0, MCONFIG_RSA_PUBKEY_PEM_SIZE);

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    ret = mbedtls_rsa_gen_key(&rsa, mconfig_random, NULL, MCONFIG_RSA_KEY_BITS, MCONFIG_RSA_EXPONENT);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_gen_key, ret: %x", -ret);

    pk.pk_ctx  = &rsa;
    pk.pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    MDF_ERROR_GOTO(!pk.pk_info, EXIT, "mbedtls_pk_info_from_type failed");

    ret = mbedtls_pk_write_key_pem(&pk, (uint8_t *)privkey_pem, MCONFIG_RSA_PRIVKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_write_key_pem, ret: 0x%x", -ret);

    ret = mbedtls_pk_write_pubkey_pem(&pk, (uint8_t *)pubkey_pem, MCONFIG_RSA_PUBKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_write_key_pem, ret: 0x%x", -ret);

EXIT:
    mbedtls_rsa_free(&rsa);
    xQueueSend(g_rsa_result_queue, &ret, portMAX_DELAY);

    vTaskDelete(NULL);
}

mdf_err_t mconfig_rsa_gen_key(char *privkey_pem, char *pubkey_pem)
{
    MDF_PARAM_CHECK(privkey_pem);
    MDF_PARAM_CHECK(pubkey_pem);

    mdf_err_t ret       = MDF_OK;
    static void *s_lock = NULL;

    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);

#ifdef CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE
    MDF_LOGD("mconfig partition subtype: 0x%x, label: %s",
             CONFIG_MCONFIG_PARTITION_SUBTYPE, CONFIG_MCONFIG_PARTITION_LABLE);

    esp_partition_iterator_t part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA,
                                         CONFIG_MCONFIG_PARTITION_SUBTYPE, CONFIG_MCONFIG_PARTITION_LABLE);
    MDF_ERROR_CHECK(!part_itra, MDF_ERR_NOT_SUPPORTED, "partition no find, subtype: 0x%x, label: %s",
                    CONFIG_MCONFIG_PARTITION_SUBTYPE, CONFIG_MCONFIG_PARTITION_LABLE);

    const esp_partition_t *rsa_part = esp_partition_get(part_itra);
    MDF_ERROR_CHECK(!rsa_part, MDF_ERR_NOT_SUPPORTED, "esp_partition_get");

    if (esp_partition_read(rsa_part, 0, privkey_pem, MCONFIG_RSA_PRIVKEY_PEM_SIZE) == ESP_OK
            && esp_partition_read(rsa_part, MCONFIG_RSA_PRIVKEY_PEM_SIZE,
                                  pubkey_pem, MCONFIG_RSA_PUBKEY_PEM_SIZE) == ESP_OK) {
        if (!strncmp(privkey_pem, PEM_BEGIN_PRIVATE_KEY, strlen(PEM_BEGIN_PRIVATE_KEY))
                && !strncmp(pubkey_pem, PEM_BEGIN_PUBLIC_KEY, strlen(PEM_BEGIN_PUBLIC_KEY))) {
            xSemaphoreGive(s_lock);
            return MDF_OK;
        }
    }

    MDF_LOGE("The key of mconfig RSA is not written correctly");
    xSemaphoreGive(s_lock);
    return ESP_FAIL;
#endif /**< CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE */

    g_pubkey_pem       = pubkey_pem;
    g_privkey_pem      = privkey_pem;
    g_rsa_result_queue = xQueueCreate(1, sizeof(mdf_err_t));

    /**< Generating rsa requires 8k of stack space, reducing memory overhead by creating new tasks */
    xTaskCreatePinnedToCore(mconfig_rsa_gen_key_task, "mconfig_rsa_gen_key", 8 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

    xQueueReceive(g_rsa_result_queue, &ret, portMAX_DELAY);
    vQueueDelete(g_rsa_result_queue);
    g_rsa_result_queue = NULL;

    xSemaphoreGive(s_lock);
    return ret;
}

mdf_err_t mconfig_rsa_decrypt(const uint8_t *ciphertext, const char *privkey_pem,
                              void *plaintext, size_t plaintext_size)
{
    MDF_PARAM_CHECK(plaintext);
    MDF_PARAM_CHECK(plaintext_size <= MCONFIG_RSA_CIPHERTEXT_SIZE - 11);
    MDF_PARAM_CHECK(privkey_pem);
    MDF_PARAM_CHECK(ciphertext);

    int ret                  = MDF_OK;
    size_t olen              = 0;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;

    memset(&pk, 0, sizeof(mbedtls_pk_context));

    ret = mbedtls_pk_parse_key(&pk, (uint8_t *)privkey_pem, MCONFIG_RSA_PRIVKEY_PEM_SIZE, NULL, 0);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mbedtls_pk_parse_key, privkey_pem: %s, ret: %x",
                   privkey_pem, -ret);

    rsa = mbedtls_pk_rsa(pk);
    MDF_ERROR_GOTO(!rsa, EXIT, "mbedtls_pk_rsa, rsa: %p", rsa);
    rsa->len = MCONFIG_RSA_KEY_BITS / 8;

    ret = mbedtls_rsa_check_privkey(rsa);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_check_privkey, ret: %x", -ret);

    ret = mbedtls_rsa_pkcs1_decrypt(rsa, mconfig_random, NULL, MBEDTLS_RSA_PRIVATE,
                                    &olen, ciphertext, (uint8_t *)plaintext, plaintext_size);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_pkcs1_decrypt, ret: %x", -ret);

EXIT:
    mbedtls_pk_free(&pk);
    return ret;
}

mdf_err_t mconfig_rsa_encrypt(const void *plaintext, size_t plaintext_size,
                              const char *pubkey_pem, uint8_t *ciphertext)
{
    MDF_PARAM_CHECK(plaintext);
    MDF_PARAM_CHECK(plaintext_size <= MCONFIG_RSA_PLAINTEXT_MAX_SIZE);
    MDF_PARAM_CHECK(pubkey_pem);
    MDF_PARAM_CHECK(ciphertext);

    int ret                  = ESP_FAIL;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;

    memset(&pk, 0, sizeof(mbedtls_pk_context));
    ret = mbedtls_pk_parse_public_key(&pk, (uint8_t *)pubkey_pem, MCONFIG_RSA_PUBKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_parse_public_key, ret: %x", -ret);

    rsa = mbedtls_pk_rsa(pk);
    MDF_ERROR_GOTO(!rsa, EXIT, "mbedtls_pk_rsa, rsa: %p", rsa);
    rsa->len = MCONFIG_RSA_KEY_BITS / 8;

    ret = mbedtls_rsa_check_pubkey(rsa);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_check_pubkey, ret: %x", -ret);

    ret = mbedtls_rsa_pkcs1_encrypt(rsa, mconfig_random, NULL, MBEDTLS_RSA_PUBLIC, plaintext_size,
                                    (uint8_t *)plaintext, ciphertext);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_pkcs1_encrypt, ret: %x", -ret);

EXIT:
    mbedtls_pk_free(&pk);
    return ret;
}
