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

#ifndef __MESH_LCD_H__
#define __MESH_LCD_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**< type of saving coredump date to sdcard */
typedef enum {
    ESPNOW_SDCARD_DEL_TEM,
    ESPNOW_SDCARD_WRITE_TEM,
    ESPNOW_SDCARD_SAVE
} espnow_sdcard_oprt_t;

/**
 * @brief  write log date to sdcard
 *
 * @param  mac_addr mac address of the mdf-device that is sending log
 * @param  data     log data
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t espnow_sd_log_write(const uint8_t *mac_addr, const char *data);


/**
 * @brief  write coredump date to sdcard
 *
 * @param  mac_addr mac address of the mdf-device that is sending coredump
 * @param  data     coredump data
 * @param  len      coredump data len
 * @param  oprt     type of saving coredump date to sdcard
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t espnow_sd_coredump_write(const uint8_t *mac_addr, uint8_t *data, size_t len, uint8_t oprt);


/**
 * @brief  init sdcard of espnow terminal board
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t espnow_sdcard_init(void);


#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MESH_LCD_H__ */
