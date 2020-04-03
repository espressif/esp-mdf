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

#ifndef __ALIYUN_KV_H__
#define __ALIYUN_KV_H__

#include "aliyun_defs.h"
#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  set product key
 *
 * @param  product_key  Pointer to the stored key value stored in the flash.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_set_product_key(char *product_key);

/**
 * @brief  set device name
 *
 * @param  device_name  Pointer to the stored key value stored in the flash.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_set_device_name(char *device_name);

/**
 * @brief  set device secret
 *
 * @param  device_secret  Pointer to the stored key value stored in the flash.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_set_device_secret(char *device_secret);

/**
 * @brief  set product secret
 *
 * @param  product_secret  Pointer to the stored key value stored in the flash.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_set_product_secret(char *product_secret);

/**
 * @brief  get product_key
 *
 * @param  product_key  Pointer to the key value read from flash.
 * @param  size  The length of the product_key.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_get_product_key(char *product_key, size_t size);

/**
 * @brief  get device_name
 *
 * @param  device_name  Pointer to the key value read from flash.
 * @param  size  The length of the device_name.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_get_device_name(char *device_name, size_t size);

/**
 * @brief  get device_secret
 *
 * @param  device_secret  Pointer to the key value read from flash.
 * @param  size  The length of the device_secret.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_get_device_secret(char *device_secret, size_t size);

/**
 * @brief  get product_secret
 *
 * @param  product_secret  Pointer to the key value read from flash.
 * @param  size  The length of the product_secret.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_get_product_secret(char *product_secret, size_t size);

/**
 * @brief  get devide meta
 *
 * @param  meta  Pointer to the key value read from flash.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t aliyun_get_meta(aliyun_device_meta_t *meta);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_SIGN_H__ */
