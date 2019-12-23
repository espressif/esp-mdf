// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __MDF_DEBUG_LOG_H__
#define __MDF_DEBUG_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#include "mdf_common.h"
#include "mespnow.h"
#include "esp_wifi.h"
#include "esp_console.h"
#include "mdebug_espnow.h"

/**
 * @brief Log sending configuration
 */
typedef struct {
    bool log_uart_enable;   /**< Serial port enable */
    bool log_flash_enable;  /**< Write the log to flash enable */
    bool log_espnow_enable; /**< enable log transferred via ESP-NOW */
    uint8_t dest_addr[6];   /**< Turn off log sending if all is zero */
} mdebug_log_config_t;

/**
 * @brief Set the send type
 *
 */
typedef enum {
    MDEBUG_LOG_TYPE_ESPNOW = 1 << 1,
    MDEBUG_LOG_TYPE_FLASH  = 1 << 2,
} mdebug_log_type_t;

/**
 * @brief Type of log storage queue
 */
typedef struct {
    uint16_t size;          /**< The length of the log data */
    mdebug_log_type_t type; /**< Ways to send logs */
    char data[0];           /**< Log data */
} mdebug_log_queue_t;

/**
 * @brief  Get the configuration of the log during wireless debugging
 *
 * @param  config The configuration of the log
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_log_get_config(mdebug_log_config_t *config);

/**
 * @brief  Set the configuration of the log during wireless debugging
 *
 * @param  config The configuration of the log
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_log_set_config(const mdebug_log_config_t *config);

/**
 * @brief Init log mdebug
 *        - Set log mdebug configuration
 *        - Create log mdebug task
 *
 * @return
 *     - MDF_OK
 */
mdf_err_t mdebug_log_init(void);

/**
 * @brief De-initialize log mdebug
 *      Call this once when done using log mdebug functions
 *
 * @return
 *     - MDF_OK
 */
mdf_err_t mdebug_log_deinit(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_DEBUG_LOG_H__ */
