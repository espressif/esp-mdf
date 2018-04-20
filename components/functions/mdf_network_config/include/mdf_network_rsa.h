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

#ifndef __MDF_AUTO_NETWORK_CONFIG_H__
#define __MDF_AUTO_NETWORK_CONFIG_H__

#include "mdf_common.h"
#include "mdf_network_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDF_RSA_PRIVKEY_PEM_SIZE     1024
#define MDF_RSA_PUBKEY_PEM_SIZE      273
#define MDF_RSA_KEY_BITS             1024
#define MDF_RSA_CIPHERTEXT_SIZE      (MDF_RSA_KEY_BITS / 8)
#define MDF_RSA_EXPONENT             65537
#define MDF_RSA_PUBKEY_PEM_DATA_SIZE 220

/**
 * @brief  generate rsa key pair
 *
 * @param  privkey_pem rsa public key in pem format
 * @param  pubkey_pem  rsa private key in pem format
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_rsa_gen_key(char privkey_pem[MDF_RSA_PRIVKEY_PEM_SIZE],
                          char pubkey_pem[MDF_RSA_PUBKEY_PEM_SIZE]);

/**
 * @brief  use rsa's public key to encrypt the data
 *
 * @param  plaintext      unencrypted data
 * @param  plaintext_size the length of unencrypted data
 * @param  pubkey_pem     rsa private key in pem format
 * @param  ciphertext     encrypted data
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_rsa_encrypt(const void *plaintext, size_t plaintext_size,
                          const uint8_t pubkey_pem[MDF_RSA_PUBKEY_PEM_SIZE],
                          uint8_t ciphertext[MDF_RSA_CIPHERTEXT_SIZE]);

/**
 * @brief  use rsa's public key to encrypt the data
 *
 * @param  plaintext      unencrypted data
 * @param  plaintext_size the length of unencrypted data
 * @param  pubkey_pem     rsa private key in pem format
 * @param  ciphertext     encrypted data
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_rsa_decrypt(const uint8_t ciphertext[MDF_RSA_CIPHERTEXT_SIZE],
                          const uint8_t privkey_pem[MDF_RSA_PRIVKEY_PEM_SIZE],
                          void *plaintext, size_t plaintext_size);

#ifdef __cplusplus
}
#endif

#endif /**< __MDF_AUTO_NETWORK_CONFIG_H__ */
