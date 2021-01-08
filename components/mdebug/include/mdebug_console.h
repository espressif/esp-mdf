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

#ifndef __MDF_CONSOLE_DEBUG_H__
#define __MDF_CONSOLE_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief   Initialize console module
 *          - Initialize the console
 *          - Register help commands
 *          - Initialize filesystem
 *          - Create console handle task
 * 
 * @attention Baudrate should not greater than 2030400 if console is enable
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_console_init(void);

/**
 * @brief   De-initialize console module
 *          Call this once when done using console module functions
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_console_deinit(void);

/**
 * @brief   Register frequently used system commands
 *          - version: Get version of chip and SDK
 *          - heap: Get the current size of free heap memory
 *          - restart: Software reset of the chip
 *          - reset: Clear device configuration information
 *          - log: Set log level for given tag
 *          - coredump: Get core dump information
 */
void mdebug_cmd_register_common(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_CONSOLE_DEBUG_H__ */
