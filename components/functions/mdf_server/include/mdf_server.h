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

#ifndef __MDF_H__
#define __MDF_H__

#include "mdf_common.h"
#include "mdf_socket.h"
#include "mdf_network_config.h"
#include "openssl/ssl.h"
#include "mdf_wifi_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  create local connection
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t mdf_server_init();

/**
 * @brief  determine whether the connection is disconnected locally
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t mdf_server_deinit();

/**
 * @brief  create a local connection
 *
 * @return
 *    - true
 *    - false
 */
bool mdf_server_conn_is_running();

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_H__ */
