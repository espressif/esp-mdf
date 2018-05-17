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

#ifndef __MDF_DEVICE_HANDLE_H__
#define __MDF_DEVICE_HANDLE_H__

#include "mdf_common.h"

#include "mdf_json.h"
#include "mdf_http.h"
#include "mdf_socket.h"
#include "mdf_event_loop.h"
#include "mdf_info_store.h"

#include "mdf_espnow.h"
#include "mdf_wifi_mesh.h"

#include "mdf_trigger.h"
#include "mdf_low_power.h"
#include "mdf_sniffer_handle.h"
#include "mdf_network_config.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MESH_PROTO_DATA_BIN             100
#define DEVICE_HANDLE_SIZE              50
#define DEVICE_CHARACTERISTICS_MAX_SIZE 50

/**
 * @brief protocol of transmitted application data
 */
typedef enum {
    MDF_PROTO_JSON,    /**< JSON format */
    MDF_PROTO_BIN,     /**< binary */
    MDF_PROTO_MQTT,    /**< MQTT protocol */
    MDF_PROTO_HTTP,    /**< HTTP protocol */
} data_proto_t;

/**
 * @brief transmitted application data
 */
typedef struct {
    char *request;               /**< request data */
    ssize_t request_size;        /**< The size of the request data */
    char *response;              /**< response data */
    ssize_t response_size;       /**< The size of the response data */
    data_proto_t response_proto; /**< The proto of the response data */
} device_data_t;

typedef esp_err_t (*device_func_t)(device_data_t *const device_data);

/**
 * @brief permissions for the characteristics
 */
typedef enum {
    PERMS_READ               = 1 << 0,                          /**< the characteristic of the device are readable */
    PERMS_WRITE              = 1 << 1,                          /**< the characteristic of the device are writable*/
    PERMS_TRIGGER            = 1 << 2,                          /**< the characteristic of the device are triggerable */
    PERMS_READ_WRITE         = PERMS_READ | PERMS_WRITE,        /**< the characteristic of the device are readable & writable */
    PERMS_READ_TRIGGER       = PERMS_READ | PERMS_TRIGGER,      /**< the characteristic of the device are readable & triggerable */
    PERMS_WRITE_TRIGGER      = PERMS_WRITE | PERMS_TRIGGER,     /**< the characteristic of the device are writable & triggerable */
    PERMS_READ_WRITE_TRIGGER = PERMS_READ_WRITE | PERMS_TRIGGER,/**< the characteristic of the device are readable & writable & triggerable */
} characteristics_perms_t;

/**
 * @brief the characteristics of the device
 */
typedef struct {
    uint16_t cid;                   /**< the identifier of the characteristics */
    char name[32];                  /**< the name of the characteristics */
    int min;                        /**< the minimum value of the attribute */
    int max;                        /**< the maximum value of the attribute */
    int step;                       /**< the step of the attribute */
    characteristics_perms_t perms;  /**< the permissions of the characteristics*/
} device_characteristics_t;

/**
 * @brief device configuration information
 */
typedef struct {
    uint32_t tid;                               /**< the identifier of the device type */
    char name[32];                              /**< the name of the device */
    char version[16];                           /**< the version of the device */
    uint8_t characteristics_num;                /**< the number of device attributes */
    device_characteristics_t *characteristics;  /**< the characteristics of the device */
} device_config_t;

typedef esp_err_t (*device_operate_t)(uint8_t cid, void *value);

/**
 * @brief  initialize the device configuration
 *
 * @param  device_config device configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_device_init_config(uint16_t tid, const char name[32], const char version[16]);

/**
 * @brief  get device name
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_device_set_name(const char name[32]);

/**
 * @brief  get device name
 *
 * @return name of the device
 */
const char *mdf_device_get_name();

/**
 * @brief  get device version
 *
 * @return version of the device
 */
const char *mdf_device_get_version();

/**
 * @brief  get device tid
 *
 * @return tid of the device
 */
uint16_t mdf_device_get_tid();

/**
 * @brief  initialize the device handle
 *
 * @param  event_cb event handling callback function,
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_device_add_characteristics(uint16_t cid, const char name[32],
        uint8_t perms, int min, int max, int step);

/**
 * @brief  initialize the device handle
 *
 * @param  event_cb      event handling callback function,
 *                       if passed null will be used with the default event handler
 * @param  get_value_cb  callback function to get the value of the device
 * @param  get_value_cb  callback function to set the value of the device
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_device_init_handle(mdf_event_loop_cb_t event_cb,
                                 const void *get_value_cb, const void *set_value_cb);

/**
 * @brief  add device handle
 *
 * @param  func_name instruction name
 * @param  func      handle the function pointer
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_device_add_handle(const char *func_name, void *func);

/**
 * @brief send new network configuration information to all devices in the mdf
 *
 * @param  network_config point to the configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_reconfig_network(network_config_t *network_config);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_DEVICE_HANDLE_H__ */
