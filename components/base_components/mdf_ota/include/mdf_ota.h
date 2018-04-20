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

#ifndef __MDF_OTA_H__
#define __MDF_OTA_H__

#include "esp_types.h"
#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MDF_OTA_OFFSET_NONE -1

/**
 * @brief initialize a firmware ota base on esp ota module.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_ota_start(void);

/**
 * @brief  save the firmware ota data to the specified location of the flash
 *
 * @param buf: Data buffer to write.
 * @param size: Size of data buffer in bytes.
 * @param offset: Destination address in Flash. Must be a multiple of 4 bytes.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t __mdf_ota_write(const char *buf, uint32_t size, ssize_t offset);
#define mdf_ota_write_3(buf, size, offset) __mdf_ota_write(buf, size, offset)
#define mdf_ota_write_2(buf, size) mdf_ota_write_3(buf, size, MDF_OTA_OFFSET_NONE)
#define mdf_ota_write(...) __PASTE(mdf_ota_write_, COUNT_PARMS(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief stop a firmware ota.
 *
 * @note After calling mdf_ota_stop(), the ota handle is no longer valid,
 *       and any memory associated with it is freed (regardless of result).
 *       so it is needed to call mdf_ota_start() to initialize a new ota.
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_ota_stop(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif/**< __MDF_OTA_H__ */
