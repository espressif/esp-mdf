/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
