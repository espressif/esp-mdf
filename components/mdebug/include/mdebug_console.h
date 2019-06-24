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
