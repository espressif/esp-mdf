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

#ifndef __MDF_HTTP_PARSER_H__
#define __MDF_HTTP_PARSER_H__

#include "mdf_http_handle.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  handle http requests
 *
 * @param  conn         point to the connection information
 * @param  http_requset http request data
 * @param  size         The size of the requested data for http
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_server_request_parser(server_http_connect_t *conn);

/**
 * @brief  the root node handles the ota upgrade
 *
 * @param  conn     point to the connection information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_upgrade_parser(server_http_connect_t *conn);

/**
 * @brief  converts the mdf format response data to http format
 *
 * @param  http_response used to store the response data for the conversion
 * @param  mdf_data     will be converted to http body
 * @param  mdf_opt_data will be converted to http header
 *
 * @return
 *     - http response packet length
 *     - ESP_FAIL
 */
ssize_t mdf_http_response_create(char *http_response, const wifi_mesh_data_type_t *type,
                                 const wifi_mesh_addr_t *addr, void *data, size_t size);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_HTTP_PARSER_H__ */
