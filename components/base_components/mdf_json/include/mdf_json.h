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

#ifndef __MDF_JOSN_H__
#define __MDF_JOSN_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  esp_err_t mdf_json_parse( const char *json_str,  const char *key, void *value)
 *         Parse the json formatted string
 *
 * @param  json_str The string pointer to be parsed
 * @param  key      Nuild value pairs
 * @param  value    You must ensure that the incoming type is consistent with the
 *                  post-resolution type
 *
 * @note   does not support the analysis of array types
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t __mdf_json_parse(const char *json_str,  const char *key, void *value, int value_type);
#define mdf_json_parse(json_str, key, value) \
    __mdf_json_parse(( const char *)(json_str), ( const char *)(key), value, \
                     __builtin_types_compatible_p(typeof(value), int8_t *) * 1\
                     + __builtin_types_compatible_p(typeof(value), uint8_t *) * 1\
                     + __builtin_types_compatible_p(typeof(value), short *) * 2\
                     + __builtin_types_compatible_p(typeof(value), uint16_t *) * 2\
                     + __builtin_types_compatible_p(typeof(value), int *) * 3\
                     + __builtin_types_compatible_p(typeof(value), uint32_t *) * 3\
                     + __builtin_types_compatible_p(typeof(value), long *) * 3\
                     + __builtin_types_compatible_p(typeof(value), unsigned long *) * 3\
                     + __builtin_types_compatible_p(typeof(value), float *) * 4\
                     + __builtin_types_compatible_p(typeof(value), double *) * 5\
                     + __builtin_types_compatible_p(typeof(value), char *) * 6\
                     + __builtin_types_compatible_p(typeof(value), char []) * 6\
                     + __builtin_types_compatible_p(typeof(value), char **) * 7)

/**
 * @brief  mdf_json_pack(char *json_str,  const char *key, int/double/char value);
 *         Create a json string
 *
 * @param  json_str Save the generated json string
 * @param  key      Build value pairs
 * @param  value    This is a generic, support long / int / float / char / char* / char []
 *
 * @note   if the value is double or float type only retains the integer part,
 *         requires complete data calling  mdf_json_pack_double()
 *
 * @return
 *     - lenght: generates the length of the json string
 *     - ESP_FAIL
 */
esp_err_t __mdf_json_pack(char *json_str,  const char *key, int value, int value_type);
#define mdf_json_pack(json_str, key, value) \
    __mdf_json_pack((char *)(json_str), ( const char *)(key), (int)(value), \
                    __builtin_types_compatible_p(typeof(value), char) * 1\
                    + __builtin_types_compatible_p(typeof(value), int8_t) * 1\
                    + __builtin_types_compatible_p(typeof(value), uint8_t) * 1\
                    + __builtin_types_compatible_p(typeof(value), short) * 1\
                    + __builtin_types_compatible_p(typeof(value), uint16_t) * 1\
                    + __builtin_types_compatible_p(typeof(value), int) * 1\
                    + __builtin_types_compatible_p(typeof(value), uint32_t) * 1\
                    + __builtin_types_compatible_p(typeof(value), long) * 1\
                    + __builtin_types_compatible_p(typeof(value), unsigned long) * 1\
                    + __builtin_types_compatible_p(typeof(value), float) * 2\
                    + __builtin_types_compatible_p(typeof(value), double) * 2\
                    + __builtin_types_compatible_p(typeof(value), char *) * 3\
                    + __builtin_types_compatible_p(typeof(value), char []) * 3\
                    + __builtin_types_compatible_p(typeof(value), const char *) * 3\
                    + __builtin_types_compatible_p(typeof(value), unsigned char *) * 3\
                    + __builtin_types_compatible_p(typeof(value), const unsigned char *) * 3)

/**
 * @brief  create a double type json string, Make up for the lack of mdf_json_pack()
 *
 * @param  json_str Save the generated json string
 * @param  key      Build value pairs
 * @param  value    The value to be stored
 *
 * @return
 *     - lenght: generates the length of the json string
 *     - ESP_FAIL
 */
esp_err_t mdf_json_pack_double(char *json_str,  const char *key, double value);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_JOSN_H__ */
