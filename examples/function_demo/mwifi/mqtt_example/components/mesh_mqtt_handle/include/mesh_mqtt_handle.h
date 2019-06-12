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

#ifndef __MESH_MQTT_HANDLE_H__
#define __MESH_MQTT_HANDLE_H__

#include "mdf_common.h"
#include "mqtt_client.h"

#define MDF_EVENT_CUSTOM_MQTT_CONNECT       (MDF_EVENT_CUSTOM_BASE+1)
#define MDF_EVENT_CUSTOM_MQTT_DISCONNECT    (MDF_EVENT_CUSTOM_BASE+2)

/**
 * @brief  Check if mqtt is connected
 *
 * @return
 *     - true
 *     - false
 */
bool mesh_mqtt_is_connect();

/**
 * @brief  mqtt subscribe special topic according device MAC address.
 *
 * @param  subdev_list node address list
 * @param  subdev_num the number of node
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_subscribe(uint8_t *subdev_list, size_t subdev_num);

/**
 * @brief  mqtt unsubscribe special topic according device MAC address.
 *
 * @param  subdev_list node address list
 * @param  subdev_num number of node
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_unsubscribe(uint8_t *subdev_list, size_t subdev_num);

/**
 * @brief  mqtt publish data to special topic
 *
 * @param  addr node address
 * @param  data pointer of data
 * @param  size length of data
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_write(uint8_t *addr, void *data, size_t size);

/**
 * @brief  receive data from special topic
 *
 * @param  addr node address
 * @param  data secondary pointer of data
 * @param  size length of data
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_read(uint8_t *addr, void **data, size_t *size, TickType_t wait_ticks);

/**
 * @brief  start mqtt client
 *
 * @param  url mqtt connect url
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_start(char *url);

/**
 * @brief  stop mqtt client
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_stop();

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MESH_MQTT_HANDLE_H__ */
