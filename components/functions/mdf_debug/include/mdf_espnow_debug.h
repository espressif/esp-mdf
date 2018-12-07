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
#ifndef __MDF_ESPNOW_DEBUG_H__
#define __MDF_ESPNOW_DEBUG_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

enum mdf_espnow_debug_type {
    MDF_ESPNOW_DEBUG_LOG = 0,  /**< log data */
    MDF_ESPNOW_DEBUG_COREDUMP, /**< coredump data */
    MDF_ESPNOW_DEBUG_CONFIG,   /**< config data */
    MDF_ESPNOW_DEBUG_ACK,      /**< ack data*/
    MDF_ESPNOW_DEBUG_MANUAL,   /**< manual */
};

enum mdf_espnow_coredump_operator {
    MDF_ESPNOW_COREDUMP_DETECT = 0, /**< detect coredump partition to check coredump data */
    MDF_ESPNOW_COREDUMP_RSP,        /**< response to detector whether coredump data exist */
    MDF_ESPNOW_COREDUMP_RETRANS,    /**< retransfer coredump data */
    MDF_ESPNOW_COREDUMP_ERASE,      /**< erase coredump partition */
};

enum mdf_espnow_coredump_rsp_params {
    MDF_ESPNOW_COREDUMP_START = 0,    /**< start transfer coredump data */
    MDF_ESPNOW_COREDUMP_TRANSFERRING, /**< transferring coredump data */
    MDF_ESPNOW_COREDUMP_END,          /**< end transfer coredump data */
    MDF_ESPNOW_COREDUMP_NULL = 10,    /**< no coredump data found */
};

enum mdf_espnow_config_operator {
    MDF_ESPNOW_CONFIG_LOG = 0,  /**< config espnow debug log level */
    MDF_ESPNOW_CONFIG_DEST,     /**< config espnow debug destination address */
    MDF_ESPNOW_CONFIG_ALL,      /**< config espnow debug log and destination address */
};

enum mdf_espnow_config_log_param {
    MDF_ESPNOW_LOG_NONE,     /**< no output */
    MDF_ESPNOW_LOG_ERROR,    /**< error log */
    MDF_ESPNOW_LOG_WARN,     /**< warning log */
    MDF_ESPNOW_LOG_INFO,     /**< info log */
    MDF_ESPNOW_LOG_DEBUG,    /**< debug log */
    MDF_ESPNOW_LOG_VERBOSE,  /**< verbose log */
    MDF_ESPNOW_LOG_RESTORE,  /**< restore to original ESP_LOG* channel */
};

enum mdf_espnow_ack_params {
    MDF_ESPNOW_RECV_OK = 0, /**< espnow package received is correct */
    MDF_ESPNOW_RECV_FAIL,   /**< espnow package received is error */
};

enum mdf_espnow_manual {
    MDF_ESPNOW_MANUAL_REBOOT = 0, /**< reboot device */
    MDF_ESPNOW_MANUAL_RESET,      /**< reset device */
    MDF_ESPNOW_MANUAL_CONFIG,     /**< config network mode */
    MDF_ESPNOW_MANUAL_ERASE,      /**< erase nvs key */
};

typedef struct {
    uint8_t type;
    uint8_t oprt;
    uint8_t params;
    uint16_t seq;
    size_t size;
    uint8_t data[0];
} mdf_espnow_debug_pkt_t;

#define DEBUG_PKT_STRUCT_LEN  sizeof(mdf_espnow_debug_pkt_t)

/**
 * @brief  get coredump partition info
 *
 * @param  length coredump data len, if length is < 1024B means no coredump data
 *
 * @return const pointer to coredump partition
 */
const esp_partition_t *mdf_coredump_get_info(size_t *length);

/**
 * @brief  erase coredump partition
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_coredump_erase(void);

/**
 * @brief  set espnow debug module
 *
 * @param  espnow_config espnow config params
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_set_config(const mdf_espnow_debug_pkt_t *espnow_config);

/**
 * @brief  init espnow debug module
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_espnow_debug_init(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_DEBUG_H__ */
