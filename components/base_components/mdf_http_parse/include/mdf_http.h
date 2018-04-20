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

#ifndef __MDF_HTTP_H__
#define __MDF_HTTP_H__

#include "mdf_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum mdf_http_method {
    MDF_HTTP_DELETE = 0,
    MDF_HTTP_GET,
    MDF_HTTP_HEAD,
    MDF_HTTP_POST,
    MDF_HTTP_PUT,
};

/**
 * @brief  the request line data is parsed from the HTTP format data
 *
 * @param  http_buf Http format data
 * @param  method   Request method
 * @param  url      Uniform Resource Identifier
 *
 * @return
 *     - url_query
 *     - NULL
 */
char *__http_get_request_line(const char *http_buf, enum mdf_http_method *method, char *url);
#define mdf_http_get_request_line_3(http_buf, method, url) __http_get_request_line(http_buf, method, url)
#define mdf_http_get_request_line_2(http_buf, method) mdf_http_get_request_line_3(http_buf, method, NULL)
#define mdf_http_get_request_line_1(http_buf) mdf_http_get_request_line_2(http_buf, NULL)
#define mdf_http_get_request_line(...)\
    __PASTE(mdf_http_get_request_line_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief  the response state line data is parsed from the HTTP format data
 *
 * @param  http_buf    Http format data
 * @param  status_code Response status code
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_get_response_status(const char *http_buf, uint16_t *status_code);

/**
 * @brief  the message header data is parsed from the HTTP format data
 *         esp_err_t mdf_http_get_header(const char *http_buf, const char *field, char *value);
 *
 * @param  http_buf Http format data
 * @param  field    Header field
 * @param  value    Header value
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t __http_get_header(const char *http_buf, const char *field, void *value, int value_type);
#define mdf_http_get_header(http_buf, field, value) \
    __http_get_header(http_buf, field, value, \
                      __builtin_types_compatible_p(typeof(value), char *) * 1\
                      + __builtin_types_compatible_p(typeof(value), uint8_t *) * 1\
                      + __builtin_types_compatible_p(typeof(value), short *) * 2\
                      + __builtin_types_compatible_p(typeof(value), uint16_t *) * 2\
                      + __builtin_types_compatible_p(typeof(value), int *) * 3\
                      + __builtin_types_compatible_p(typeof(value), uint32_t *) * 3\
                      + __builtin_types_compatible_p(typeof(value), char **) * 4)

/**
 * @brief  the message body data is parsed from the HTTP format data
 *
 * @param  http_buf    Http format data
 * @param  body        http body
 * @param  size        http body size
 *
 * @return
 *     - body
 *     - NULL
 */
char *__http_get_body(const char *http_buf, char *body, size_t *size);
#define mdf_http_get_body_3(http_buf, body, size) __http_get_body(http_buf, body, size)
#define mdf_http_get_body_2(http_buf, size) mdf_http_get_body_3(http_buf, NULL, size)
#define mdf_http_get_body_1(http_buf) mdf_http_get_body_3(http_buf, NULL, NULL)
#define mdf_http_get_body(...)\
    __PASTE(mdf_http_get_body_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief  the path data is parsed from the Uniform Resource Identifier
 *
 * @param  http_url    Uniform Resource Identifier
 * @param  url_path    The path in the URL
 *
 * @return
 *     - ESP_OK
 *     - NULL
 */
char *__http_get_url_path(const char *http_url, char *url_path);
#define mdf_http_get_url_path_2(http_url, url_path) __http_get_url_path(http_url, url_path)
#define mdf_http_get_url_path_1(http_url) mdf_http_get_url_path_2(http_url, NULL)
#define mdf_http_get_url_path(...)\
    __PASTE(mdf_http_get_url_path_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief  the query data is parsed from the Uniform Resource Identifier
 *
 * @param  http_url    Uniform Resource Identifier
 * @param  url_query   The query in the URL
 *
 * @return
 *     - url_query
 *     - NULL
 */
char *__http_get_url_query(const char *http_url, char *url_query);
#define mdf_http_get_url_query_2(http_url, url_query) __http_get_url_query(http_url, url_query)
#define mdf_http_get_url_query_1(http_url) mdf_http_get_url_query_2(http_url, NULL)
#define mdf_http_get_url_query(...)\
    __PASTE(mdf_http_get_url_query_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief  add the request line data to the HTTP format data
 *
 * @param  http_buf Http format data
 * @param  method   Request method
 * @param  url      Uniform Resource Identifier
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_set_request_line(char *http_buf, enum mdf_http_method method, const char *url);

/**
 * @brief  add response state line data to the HTTP format data
 *
 * @param  http_buf    Http format data
 * @param  status_code Response status code
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_set_response_status(char *http_buf, uint16_t status_code);

/**
 * @brief  add message header data to the HTTP format data
 *
 * @param  http_buf Http format data
 * @param  field    Header field
 * @param  value    Field value
 *
 * @note   not support float or double type value
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t __http_set_header(char *http_buf, const char *field, const void *value, int value_type);
#define mdf_http_set_header(http_buf, field, value) \
    __http_set_header(http_buf, field, (const void *)value, \
                      __builtin_types_compatible_p(typeof(value), char) * 1\
                      + __builtin_types_compatible_p(typeof(value), int8_t) * 1\
                      + __builtin_types_compatible_p(typeof(value), uint8_t) * 1\
                      + __builtin_types_compatible_p(typeof(value), short) * 1\
                      + __builtin_types_compatible_p(typeof(value), uint16_t ) * 1\
                      + __builtin_types_compatible_p(typeof(value), int) * 1\
                      + __builtin_types_compatible_p(typeof(value), uint32_t ) * 1\
                      + __builtin_types_compatible_p(typeof(value), long ) * 1\
                      + __builtin_types_compatible_p(typeof(value), unsigned long ) * 1)

/**
 * @brief  add message body data to the HTTP format data
 *
 * @param  http_buf    Http format data
 * @param  body        http body
 * @param  size        http body size
 *
 * @return
 *     - http size
 *     - Other
 */
size_t mdf_http_set_body(char *http_buf, const char *body, size_t size);

#ifdef __cplusplus
}
#endif

#endif /**< __MDF_HTTP_H__ */
