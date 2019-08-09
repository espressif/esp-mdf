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

#ifndef __MALIYUN_LINKKIT_LIST_H__
#define __MALIYUN_LINKKIT_LIST_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief maliyun list create
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_create(void);

/**
 * @brief maliyun list destroy
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_destroy(void);

/**
 * @brief maliyun list insert
 *
 * @param[in]  device_id.
 * @param[in]  *mac.
 * @param[out]  *product.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_insert(int device_id, const uint8_t *mac, const char *product);

/**
 * @brief maliyun id for finding mac
 *
 * @param[in]  device_id.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_get(int device_id);

/**
 * @brief maliyun linkkit list get mac
 *
 * @param[in]  device_id.
 * @param[out]  *mac.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_get_mac(int device_id, uint8_t *mac);

/**
 * @brief maliyun linkkit list get id
 *
 * @param[in]  *mac.
 * @param[out]  device_id.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_get_id(const uint8_t *mac, int *device_id);

/**
 * @brief maliyun linkkit list delete id
 *
 * @param[in]  device_id.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_delete_id(int device_id);

/**
 * @brief maliyun linkkit list get offline id
 *
 * @param[in]  mac_array.
 * @param[in]  array_num.
 * @param[out]  *device_id.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t maliyun_linkkit_list_get_offline_id(uint8_t *mac_array, size_t array_num, int *device_id);

/**
 * @brief maliyun linkkit get product num
 *
 * @param[in]  product.
 *
 * @return
 *    - product_cnt
 */
int maliyun_linkkit_list_get_product_num(const char *product);

/**
 * @brief maliyun linkkit list get mac array
 *
 * @param[in]  product.
 * @param[in]  mac_array.
 *
 * @return
 *    - product_cnt
 */
int maliyun_linkkit_list_get_mac_array(const char *product, uint8_t *mac_array);

#endif  /* __MALIYUN_LINKKIT_LIST_H__ */
