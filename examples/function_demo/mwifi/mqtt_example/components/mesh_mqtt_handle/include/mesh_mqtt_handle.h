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

#define MDF_EVENT_CUSTOM_MQTT_CONNECT (MDF_EVENT_CUSTOM_BASE + 1)
#define MDF_EVENT_CUSTOM_MQTT_DISCONNECT (MDF_EVENT_CUSTOM_BASE + 2)
#define MESH_MQTT_TOPIC_MAX_LEN (32)

typedef enum {
    MESH_MQTT_DATA_BYTES = 0,
    MESH_MQTT_DATA_STRING,
    MESH_MQTT_DATA_JSON,
    MESH_MQTT_DATA_TYPE_MAX,
} mesh_mqtt_publish_data_type_t;

typedef struct {
    size_t addrs_num; /**< Number of address */
    uint8_t *addrs_list; /**< List of address */
    size_t size; /**< Length of data */
    char *data; /**< Pointer of data */
} mesh_mqtt_data_t;

/**
 * @brief  Check if mqtt is connected
 *
 * @return
 *     - true
 *     - false
 */
bool mesh_mqtt_is_connect();

/**
 * @brief  Update topo to cloud
 *
 * @return  MDF_OK if success
 */
mdf_err_t mesh_mqtt_update_topo();

/**
 * @brief  mqtt subscribe special topic according device MAC address.
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_subscribe();

/**
 * @brief  mqtt unsubscribe special topic according device MAC address.
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_unsubscribe();

/**
 * @brief  mqtt publish data to special topic
 *
 * @param  addr node address
 * @param  data pointer of data
 * @param  size length of data
 * @param  type type of the data
 *
 * @note if type is MESH_MQTT_DATA_BYTES, the data will be encode as base64 string
 * @note if type is MESH_MQTT_DATA_STRING, the data will be treated as string
 * @note if type is MESH_MQTT_DATA_JSON, the data will be treated as json object
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_write(uint8_t *addr, const char *data, size_t size, mesh_mqtt_publish_data_type_t type);

/**
 * @brief  receive data from special topic
 *
 * @param  request Request data
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mesh_mqtt_read(mesh_mqtt_data_t **request, TickType_t wait_ticks);

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
