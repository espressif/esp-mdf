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

#ifndef __MCONFIG_SECURITY_H__
#define __MCONFIG_SECURITY_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCONFIG_RSA_PRIVKEY_PEM_SIZE     (1024)
#define MCONFIG_RSA_PUBKEY_PEM_SIZE      (273)
#define MCONFIG_RSA_KEY_BITS             (1024)
#define MCONFIG_RSA_CIPHERTEXT_SIZE      (MCONFIG_RSA_KEY_BITS / 8)
#define MCONFIG_RSA_PLAINTEXT_MAX_SIZE   (MCONFIG_RSA_CIPHERTEXT_SIZE - 11)
#define MCONFIG_RSA_EXPONENT             (65537)
#define MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE (220)

#define PEM_BEGIN_PUBLIC_KEY             "-----BEGIN PUBLIC KEY-----\n"
#define PEM_END_PUBLIC_KEY               "-----END PUBLIC KEY-----\n"
#define PEM_BEGIN_PRIVATE_KEY            "-----BEGIN RSA PRIVATE KEY-----\n"
#define PEM_END_PRIVATE_KEY              "-----END RSA PRIVATE KEY-----\n"

#define MCONFIG_DH_PRIVKEY_LEN           (128)
#define MCONFIG_DH_PUBKEY_LEN            (128)
#define MCONFIG_AES_KEY_LEN              (16)

/**
 * @brief  Generate an array of random numbers
 *
 * @param  rng_state The seed of the random number, not used temporarily, you can pass NULL
 * @param  output    Pointer to an array of random numbers
 * @param  len       The length of the array of random numbers
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mconfig_random(void *rng_state, uint8_t *output, size_t len);

/**
 * @brief  Generate a public and private key using the DHM algorithm
 *
 * @param  param      DHM configuration parameters
 * @param  param_size The length of the DHM configuration parameter
 * @param  privkey    DHM's public key
 * @param  pubkey     DHM's private key
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mconfig_dhm_gen_key(uint8_t *param, ssize_t param_size,
                              uint8_t *privkey, uint8_t *pubkey);

/**
 * @brief  Generate a public and private key using the RSA algorithm
 *
 * @param  privkey_pem RSA public key in pem format
 * @param  pubkey_pem  RSA private key in pem format
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mconfig_rsa_gen_key(char *privkey_pem, char *pubkey_pem);

/**
 * @brief  Use RSA's public key to encrypt the data
 *
 * @param  ciphertext     Encrypted data
 * @param  privkey_pem    RSA's private key in pem format
 * @param  plaintext      Unencrypted data
 * @param  plaintext_size The length of unencrypted data
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 *     - MDF_ERR_INVALID_ARG
 */
mdf_err_t mconfig_rsa_decrypt(const uint8_t *ciphertext, const char *privkey_pem,
                              void *plaintext, size_t plaintext_size);

/**
 * @brief  use RSA's public key to encrypt the data
 *
 * @param  plaintext      Unencrypted data
 * @param  plaintext_size The length of unencrypted data
 * @param  pubkey_pem     RSA's public key in pem format
 * @param  ciphertext     Encrypted data
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 *     - MDF_ERR_INVALID_ARG
 */
mdf_err_t mconfig_rsa_encrypt(const void *plaintext, size_t plaintext_size,
                              const char *pubkey_pem, uint8_t *ciphertext);

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_SECURITY_H__ */
