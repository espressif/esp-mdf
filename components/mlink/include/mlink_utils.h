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
 *    The device's STA address is used For ESP-WIFI-MESH communication.
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
