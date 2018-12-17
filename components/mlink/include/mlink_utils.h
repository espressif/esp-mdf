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

#ifndef __MLINK_UTILS_H__
#define __MLINK_UTILS_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  Convert mac from string format to hex
 *
 * @param  mac_str String format mac
 * @param  mac_hex Hex format mac
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
uint8_t *mlink_mac_str2hex(const char *mac_str, uint8_t *mac_hex);

/**
 * @brief  Convert mac from hex format to string
 *
 * @param  mac_hex Hex format mac
 * @param  mac_str String format mac
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
char *mlink_mac_hex2str(const uint8_t *mac_hex, char *mac_str);

/**
 *  Each ESP32 chip has MAC addresses for Station (STA), Access Point (AP), Bluetooth low energy (BT) and Local Area Network (LAN).
 *  Their address values are incremented by one, i.e. LAN Mac = BT Mac + 1 = AP Mac + 2 = STA Mac + 3.
 *
 *  For example:
 *      - MAC for STA: `xx:xx:xx:xx:xx:00`
 *      - MAC for AP : `xx:xx:xx:xx:xx:01`
 *      - MAC for BT : `xx:xx:xx:xx:xx:02`
 *      - MAC for LAN: `xx:xx:xx:xx:xx:03`
 *
 *    The device's STA address is used For ESP-MESH communication.
 **/

/**
 * @brief Convert mac address from ap to sta
 *
 * @param  ap_mac  Access Point address in hexadecimal format
 * @param  sta_mac Station address in hexadecimal format
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
uint8_t *mlink_mac_ap2sta(const uint8_t *ap_mac, uint8_t *sta_mac);

/**
 * @brief Convert mac address from bt to sta
 *
 * @param  bt_mac  Access Point address in hexadecimal format
 * @param  sta_mac Station address in hexadecimal format
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
uint8_t *mlink_mac_bt2sta(const uint8_t *bt_mac, uint8_t *sta_mac);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_UTILS_H__ */
