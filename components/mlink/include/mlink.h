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

#ifndef __MLINK_H__
#define __MLINK_H__

#include "mdf_common.h"
#include "mlink_json.h"
#include "mlink_utils.h"
#include "mlink_notice.h"
#include "mlink_httpd.h"
#include "mlink_handle.h"
#include "mlink_trigger.h"
#include "mlink_espnow.h"
#include "mlink_ble.h"
#include "mlink_sniffer.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MDF_EVENT_MLINK_SYSTEM_RESET   (MDF_EVENT_MLINK_BASE + 0x00)
#define MDF_EVENT_MLINK_SYSTEM_REBOOT  (MDF_EVENT_MLINK_BASE + 0x01)
#define MDF_EVENT_MLINK_SET_STATUS     (MDF_EVENT_MLINK_BASE + 0x02)
#define MDF_EVENT_MLINK_GET_STATUS     (MDF_EVENT_MLINK_BASE + 0x03)
#define MDF_EVENT_MLINK_SET_TRIGGER    (MDF_EVENT_MLINK_BASE + 0x04)
#define MDF_EVENT_MLINK_BUFFER_FULL    (MDF_EVENT_MLINK_BASE + 0x05)

/**
 * @brief Type of protocol
 */
enum mlink_protocol {
    MLINK_PROTO_HTTPD  = 0, /**< Http protocol communication */
    MLINK_PROTO_NOTICE = 1, /**< UDP protocol communication */
};

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_H__ */
