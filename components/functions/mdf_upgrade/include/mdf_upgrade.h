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

#ifndef __MDF_UPGRADE_H__
#define __MDF_UPGRADE_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MDF_UPGRADE_GET_BITS(data, bits) ( ((data)[(bits) >> 0x3]) & ( 1 << ((bits) & 0x7)) )
#define MDF_UPGRADE_SET_BITS(data, bits) do { ((data)[(bits) >> 0x3]) |= ( 1 << ((bits) & 0x7)); } while(0);

/**
 * @brief  init mdf ota upgrade.
 *
 * @param  ota_bin_len     Len of ota bin
 * @param  ota_package_len Len of ota package
 * @param  ota_bin_verson  Version of ota bin
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_upgrade_init(ssize_t ota_bin_len, ssize_t ota_package_len,
                           const char *ota_bin_verson, const char *ota_bin_md5);

/**
 * @brief  init mdf ota upgrade.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_upgrade_deinit(void);

/**
 * @brief  query mdf ota status.
 *
 * @param  progress_array indicator array of whether the packet has been received
 * @param  packet_num     num of ota packet num
 * @param  packet_size    ota packet size
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_upgrade_status(uint8_t **progress_array, ssize_t *writted_num,
                             ssize_t *packet_num, ssize_t *packet_size);

/**
 * @brief  write ota data to next update partition.
 *
 * @param  ota_data      Data buffer to write.
 * @param  ota_data_size Size of data buffer in bytes.
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_upgrade_write(const void *ota_data, ssize_t ota_data_size);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_UPGRADE_H__ */
