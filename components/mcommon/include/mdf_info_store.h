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

#ifndef __MDF_INFO_STORE_H__
#define __MDF_INFO_STORE_H__

#include "mdf_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MDF_SPACE_NAME "ESP-MDF"

/**
 * @brief Initialize the default NVS partition
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t mdf_info_init(void);

/**
 * @brief save the information with given key
 *
 * @param  key    Key name. Maximal length is 15 characters. Shouldn't be empty.
 * @param  value  The value to set.
 * @param  length length of binary value to set, in bytes; Maximum length is
 *                1984 bytes (508000 bytes or (97.6% of the partition size - 4000) bytes
 *                whichever is lower, in case multi-page blob support is enabled).
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t mdf_info_save(const char *key, const void *value, size_t length);

/**
 * @brief  Load the information,
 *         esp_err_t mdf_info_load(const char *key, void *value, size_t *length);
 *         esp_err_t mdf_info_load(const char *key, void *value, size_t length);
 *
 * @attention  The interface of this api supports size_t and size_t * types.
 *             When the length parameter of the pass is size_t, it is only the
 *             length of the value. When the length parameter of the pass is size_t *,
 *             the length of the saved information can be obtained.
 *
 * @param  key    The corresponding key of the information that want to load
 * @param  value  The corresponding value of key
 * @param  length The length of the value, Pointer type will return length
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */

#define LENGTH_TYPE_NUMBER  1
#define LENGTH_TYPE_POINTER 2
esp_err_t __mdf_info_load(const char *key, void *value, size_t len, uint32_t type);

#define mdf_info_load(key, value, len) \
    __mdf_info_load(key, value, (size_t)(len), \
                    + __builtin_types_compatible_p(typeof(len), int8_t) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), uint8_t) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), int16_t) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), uint16_t) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), int) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), uint32_t) * LENGTH_TYPE_NUMBER \
                    + __builtin_types_compatible_p(typeof(len), int *) * LENGTH_TYPE_POINTER \
                    + __builtin_types_compatible_p(typeof(len), uint32_t *) * LENGTH_TYPE_POINTER)

/*
 * @brief  Erase the information with given key
 *
 * @param  key The corresponding key of the information that want to erase
 *
 * @return
 *     - ESP_FAIL
 *     - ESP_OK
 */
esp_err_t mdf_info_erase(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* __MDF_INFO_STORE_H__ */
