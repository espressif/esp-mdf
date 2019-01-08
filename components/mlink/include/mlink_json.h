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

#ifndef __MLINK_JOSN_H__
#define __MLINK_JOSN_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief The type of the parameter
 */
enum mlink_json_type {
    MLINK_JSON_TYPE_NONE    = 0,  /**< Invalid parameter type */
    MLINK_JSON_TYPE_INT8    = 1,  /**< The type of the parameter is int8_t / uint8_t */
    MLINK_JSON_TYPE_INT16   = 10,  /**< The type of the parameter is int16_t / uint16_t */
    MLINK_JSON_TYPE_INT32   = 100,  /**< The type of the parameter is int32_t / uint32_t */
    MLINK_JSON_TYPE_FLOAT   = 1000,  /**< The type of the parameter is float */
    MLINK_JSON_TYPE_DOUBLE  = 10000,  /**< The type of the parameter is double */
    MLINK_JSON_TYPE_STRING  = 100000,  /**< The type of the parameter is string */
    MLINK_JSON_TYPE_POINTER = 1000000, /**< The type of the parameter is point */
};

/**
 * @brief  esp_err_t mlink_json_parse( const char *json_str,  const char *key, void *value)
 *         Parse the json formatted string
 *
 * @param  json_str    The string pointer to be parsed
 * @param  key         Build value pairs
 * @param  value       You must ensure that the incoming type is consistent with the
 *                     post-resolution type
 * @param  value_type  Type of parameter
 *
 * @note   does not support the analysis of array types
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t __mlink_json_parse(const char *json_str,  const char *key, void *value, int value_type);
#define mlink_json_parse(json_str, key, value) \
    __mlink_json_parse(json_str, key, value, \
                       __builtin_types_compatible_p(typeof(value), int8_t *) * MLINK_JSON_TYPE_INT8 \
                       + __builtin_types_compatible_p(typeof(value), uint8_t *) * MLINK_JSON_TYPE_INT8 \
                       + __builtin_types_compatible_p(typeof(value), short *) * MLINK_JSON_TYPE_INT16 \
                       + __builtin_types_compatible_p(typeof(value), uint16_t *) * MLINK_JSON_TYPE_INT16 \
                       + __builtin_types_compatible_p(typeof(value), int *) * MLINK_JSON_TYPE_INT32 \
                       + __builtin_types_compatible_p(typeof(value), uint32_t *) * MLINK_JSON_TYPE_INT32 \
                       + __builtin_types_compatible_p(typeof(value), long *) * MLINK_JSON_TYPE_INT32 \
                       + __builtin_types_compatible_p(typeof(value), unsigned long *) * MLINK_JSON_TYPE_INT32 \
                       + __builtin_types_compatible_p(typeof(value), float *) * MLINK_JSON_TYPE_FLOAT \
                       + __builtin_types_compatible_p(typeof(value), double *) * MLINK_JSON_TYPE_DOUBLE \
                       + __builtin_types_compatible_p(typeof(value), char *) * MLINK_JSON_TYPE_STRING \
                       + __builtin_types_compatible_p(typeof(value), char []) * MLINK_JSON_TYPE_STRING \
                       + __builtin_types_compatible_p(typeof(value), char **) * MLINK_JSON_TYPE_POINTER)

/**
 * @brief  mlink_json_pack(char *json_str, const char *key, int/double/char value);
 *         Create a json string
 *
 * @param  json_str   Save the generated json string
 * @param  key        Build value pairs
 * @param  value      This is a generic, support long / int / float / char / char* / char []
* @param   value_type Type of parameter
 *
 * @note   if the value is double or float type only retains the integer part,
 *         requires complete data calling  mlink_json_pack_double()
 *
 * @return
 *     - lenght: generates the length of the json string
 *     - ESP_FAIL
 */
esp_err_t __mlink_json_pack(char **json_str,  const char *key, int value, int value_type);
#define mlink_json_pack(json_str, key, value) \
    __mlink_json_pack(json_str, key, (int)(value), \
                      __builtin_types_compatible_p(typeof(value), char) * MLINK_JSON_TYPE_INT8 \
                      + __builtin_types_compatible_p(typeof(value), int8_t) * MLINK_JSON_TYPE_INT8 \
                      + __builtin_types_compatible_p(typeof(value), uint8_t) * MLINK_JSON_TYPE_INT8 \
                      + __builtin_types_compatible_p(typeof(value), short) * MLINK_JSON_TYPE_INT16 \
                      + __builtin_types_compatible_p(typeof(value), uint16_t) * MLINK_JSON_TYPE_INT16 \
                      + __builtin_types_compatible_p(typeof(value), int) * MLINK_JSON_TYPE_INT32 \
                      + __builtin_types_compatible_p(typeof(value), uint32_t) * MLINK_JSON_TYPE_INT32 \
                      + __builtin_types_compatible_p(typeof(value), long) * MLINK_JSON_TYPE_INT32 \
                      + __builtin_types_compatible_p(typeof(value), unsigned long) * MLINK_JSON_TYPE_INT32 \
                      + __builtin_types_compatible_p(typeof(value), char *) * MLINK_JSON_TYPE_STRING  \
                      + __builtin_types_compatible_p(typeof(value), const char *) * MLINK_JSON_TYPE_STRING  \
                      + __builtin_types_compatible_p(typeof(value), char []) * MLINK_JSON_TYPE_STRING  \
                      + __builtin_types_compatible_p(typeof(value), unsigned char *) * MLINK_JSON_TYPE_STRING  \
                      + __builtin_types_compatible_p(typeof(value), const unsigned char *) * MLINK_JSON_TYPE_STRING \
                      + __builtin_types_compatible_p(typeof(json_str), char **) * MLINK_JSON_TYPE_POINTER \
                      + __builtin_types_compatible_p(typeof(json_str), uint8_t **) * MLINK_JSON_TYPE_POINTER)

/**
 * @brief  Create a double type json string, Make up for the lack of mdf_json_pack()
 *
 * @param  json_ptr Save the generated json string
 * @param  key      Build value pairs
 * @param  value    The value to be stored
 *
 * @return
 *     - lenght: generates the length of the json string
 *     - ESP_FAIL
 */
ssize_t mlink_json_pack_double(char **json_ptr, const char *key, double value);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_JOSN_H__ */
