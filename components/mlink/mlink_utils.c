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

#include "mlink_utils.h"

static const char *TAG = "mlink_utils";

uint8_t *mlink_mac_str2hex(const char *mac_str, uint8_t *mac_hex)
{
    MDF_ERROR_ASSERT(!mac_str);
    MDF_ERROR_ASSERT(!mac_hex);

    uint32_t mac_data[6] = {0};

    sscanf(mac_str, "%02x%02x%02x%02x%02x%02x",
           mac_data, mac_data + 1, mac_data + 2, mac_data + 3, mac_data + 4, mac_data + 5);

    for (int i = 0; i < 6; i++) {
        mac_hex[i] = mac_data[i];
    }

    return mac_hex;
}

char *mlink_mac_hex2str(const uint8_t *mac_hex, char *mac_str)
{
    MDF_ERROR_ASSERT(!mac_hex);
    MDF_ERROR_ASSERT(!mac_str);

    sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", MAC2STR(mac_hex));

    return mac_str;
}

uint8_t *mlink_mac_ap2sta(const uint8_t *ap_mac, uint8_t *sta_mac)
{
    MDF_ERROR_ASSERT(!ap_mac);
    MDF_ERROR_ASSERT(!sta_mac);

    memcpy(sta_mac, ap_mac, 2);
    *((int *)(sta_mac + 2)) = htonl(htonl(*((int *)(ap_mac + 2))) - 1);

    return sta_mac;
}

uint8_t *mlink_mac_bt2sta(const uint8_t *bt_mac, uint8_t *sta_mac)
{
    MDF_ERROR_ASSERT(!bt_mac);
    MDF_ERROR_ASSERT(!sta_mac);

    memcpy(sta_mac, bt_mac, 2);
    *((int *)(sta_mac + 2)) = htonl(htonl(*((int *)(bt_mac + 2))) - 2);

    return sta_mac;
}