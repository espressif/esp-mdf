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
#include "mbedtls/base64.h"

#include "mdf_common.h"
#include "mdf_network_config.h"
#include "mdf_info_store.h"
#include "mdf_network_rsa.h"

#define MDF_RSA_PRIVKEY_ADDR         0x3a0000

static const char *TAG = "mdf_rsa";

static esp_err_t myrand(void *rng_state, unsigned char *output, size_t len)
{
    while (len--) {
        *output++ = esp_random();
    }

    return ESP_OK;
}

esp_err_t mdf_rsa_gen_key(char privkey_pem[MDF_RSA_PRIVKEY_PEM_SIZE],
                          char pubkey_pem[MDF_RSA_PUBKEY_PEM_SIZE])
{
    MDF_PARAM_CHECK(privkey_pem);
    MDF_PARAM_CHECK(pubkey_pem);

    esp_err_t ret                      = ESP_OK;
    mbedtls_rsa_context rsa            = {0};
    mbedtls_pk_context pk              = {0};
    esp_partition_iterator_t part_itra = NULL;
    uint32_t mdf_rsa_privkey_addr      = MDF_RSA_PRIVKEY_ADDR;

    /**< Compatible with older versions of partitions.csv*/
    part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA, 0x9a, "rsa");

    if (part_itra) {
        const esp_partition_t *rsa_part = esp_partition_get(part_itra);
        MDF_ERROR_CHECK(!rsa_part, ESP_FAIL, "esp_partition_get fail");

        mdf_rsa_privkey_addr = rsa_part->address;
    }

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    ret = mbedtls_rsa_gen_key(&rsa, myrand, NULL, MDF_RSA_KEY_BITS, MDF_RSA_EXPONENT);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_gen_key, ret: %x", -ret);

    pk.pk_ctx  = &rsa;
    pk.pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    MDF_ERROR_GOTO(!pk.pk_info, EXIT, "mbedtls_pk_info_from_type failed");

    ret = spi_flash_erase_sector(mdf_rsa_privkey_addr / SPI_FLASH_SEC_SIZE);
    MDF_ERROR_GOTO(ret < 0, EXIT, "spi_flash_erase_sector, key_str, ret: %d", ret);

    ret = mbedtls_pk_write_key_pem(&pk, (uint8_t *)privkey_pem, MDF_RSA_PRIVKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_write_key_pem, ret: %x", -ret);
    ret = spi_flash_write(mdf_rsa_privkey_addr, privkey_pem, MDF_RSA_PRIVKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret < 0, EXIT, "spi_flash_write, key_str, ret: %d", ret);
    MDF_LOGV("privkey_pem, size: %d, data:\n%s", strlen((char *)privkey_pem), privkey_pem);

    ret = mbedtls_pk_write_pubkey_pem(&pk, (uint8_t *)pubkey_pem, MDF_RSA_PUBKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_write_key_pem, ret: %x", -ret);
    ret = spi_flash_write(mdf_rsa_privkey_addr + MDF_RSA_PRIVKEY_PEM_SIZE,
                          pubkey_pem, MDF_RSA_PUBKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret < 0, EXIT, "spi_flash_write, key_str, ret: %d", ret);
    MDF_LOGV("pubkey_pem, size: %d, data:\n%s", strlen((char *)pubkey_pem), pubkey_pem);

EXIT:
    mbedtls_rsa_free(&rsa);
    return ret;
}

esp_err_t mdf_rsa_decrypt(const uint8_t ciphertext[MDF_RSA_CIPHERTEXT_SIZE],
                          const uint8_t privkey_pem[MDF_RSA_PRIVKEY_PEM_SIZE],
                          void *plaintext, size_t plaintext_size)
{
    MDF_PARAM_CHECK(plaintext);
    MDF_PARAM_CHECK(plaintext_size <= MDF_RSA_CIPHERTEXT_SIZE - 11);
    MDF_PARAM_CHECK(privkey_pem);
    MDF_PARAM_CHECK(ciphertext);

    int ret                  = ESP_OK;
    size_t olen              = 0;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;

    memset(&pk, 0, sizeof(mbedtls_pk_context));

    ret = mbedtls_pk_parse_key(&pk, privkey_pem, MDF_RSA_PRIVKEY_PEM_SIZE, NULL, 0);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mbedtls_pk_parse_key, ret: %x\n", -ret);

    rsa = mbedtls_pk_rsa(pk);
    MDF_ERROR_GOTO(!rsa, EXIT, "mbedtls_pk_rsa, rsa: %p", rsa);
    rsa->len = MDF_RSA_KEY_BITS / 8;

    ret = mbedtls_rsa_check_privkey(rsa);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_check_privkey, ret: %x", -ret);

    ret = mbedtls_rsa_pkcs1_decrypt(rsa, myrand, NULL, MBEDTLS_RSA_PRIVATE,
                                    &olen, ciphertext, (uint8_t *)plaintext, plaintext_size);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_pkcs1_decrypt, ret: %x", -ret);

EXIT:
    mbedtls_pk_free(&pk);
    return (ret != ESP_OK) ? ESP_FAIL : ESP_OK;
}

esp_err_t mdf_rsa_encrypt(const void *plaintext, size_t plaintext_size,
                          const uint8_t pubkey_pem[MDF_RSA_PUBKEY_PEM_SIZE],
                          uint8_t ciphertext[MDF_RSA_CIPHERTEXT_SIZE])
{
    MDF_PARAM_CHECK(plaintext);
    MDF_PARAM_CHECK(plaintext_size <= MDF_RSA_CIPHERTEXT_SIZE - 11);
    MDF_PARAM_CHECK(pubkey_pem);
    MDF_PARAM_CHECK(ciphertext);

    int ret                  = ESP_FAIL;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;

    memset(&pk, 0, sizeof(mbedtls_pk_context));
    ret = mbedtls_pk_parse_public_key(&pk, pubkey_pem, MDF_RSA_PUBKEY_PEM_SIZE);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_pk_parse_public_key, ret: %x", -ret);

    rsa = mbedtls_pk_rsa(pk);
    MDF_ERROR_GOTO(!rsa, EXIT, "mbedtls_pk_rsa, rsa: %p", rsa);
    rsa->len = MDF_RSA_KEY_BITS / 8;

    ret = mbedtls_rsa_check_pubkey(rsa);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_check_pubkey, ret: %x", -ret);

    ret = mbedtls_rsa_pkcs1_encrypt(rsa, myrand, NULL, MBEDTLS_RSA_PUBLIC, plaintext_size,
                                    (uint8_t *)plaintext, ciphertext);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_rsa_pkcs1_encrypt, ret: %x", -ret);

EXIT:
    mbedtls_pk_free(&pk);
    return (ret == ESP_OK) ? ESP_OK : ESP_FAIL;
}
