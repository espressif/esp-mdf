/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
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

#ifndef __SDCARD_H__
#define __SDCARD_H__

#include "mdf_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    FILE_TYPE_NONE,
    FILE_TYPE_HEX,
    FILE_TYPE_BIN,
    FILE_TYPE_STRING,
    FILE_TYPE_BASE64,
} file_format_t;

/**
 * @brief  Initialize sdcard, Using SDMMC peripheral
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t sdcard_init();

/**
 * @brief  Remove file stored in sdcard
 *
 * @param  file_name  file name
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_remove_file(const char *file_name);

/**
 * @brief  Rename file stored in sdcard
 *
 * @param  file_name_old  file old name
 * @param  file_name_new  file new name
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_rename_file(const char *file_name_old, const char *file_name_new);

/**
 * @brief  List all matching files
 *
 * @param  file_name  File match string,eg: *, *.log.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_list_file(const char *file_name);

/**
 * @brief  Print special file content
 *
 * @param  file_name  file name
 * @param  type  file type
 * @param  size  file size
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_print_file(const char *file_name, file_format_t type, ssize_t size);

/**
 * @brief  Write file content to sdcard
 *
 * @param  file_name  file name
 * @param  offset  write offset, if offset == UINT32_MAX, add data at end of file.
 * @param  data  writen data
 * @param  size  data length
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_write_file(const char *file_name, uint32_t offset, const void *data, size_t size);

/**
 * @brief  Read file content from sdcard
 *
 * @param  file_name  file name
 * @param  offset  write offset
 * @param  data  read data
 * @param  size  data length
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t sdcard_read_file(const char *file_name, uint32_t offset, uint8_t *data, size_t *size);

/**
 * @brief  Check if sdcard is mounted
 *
 * @return
 *     - true
 *     - false
 */
bool sdcard_is_mount();

#ifdef __cplusplus
}
#endif

#endif/*!< __SDCARD_H__ */
