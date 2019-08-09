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