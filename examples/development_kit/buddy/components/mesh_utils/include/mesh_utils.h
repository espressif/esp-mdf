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

#ifndef __MESH_UTILS_H__
#define __MESH_UTILS_H__

#include "mdf_common.h"
#include "mwifi.h"
#include "mlink.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief restart count erase timer callback function.
 *
 * @return
 *     - NULL
 */
void restart_count_erase_timercb(void *timer);

/**
 * @brief Get restart count.
 *
 * @return
 *     - count
 */
int restart_count_get();

/**
 * @brief Determine if the restart is caused by an exception.
 *
 * @return
 *     - true
 *     - false
 */
bool restart_is_exception();

/**
 * @brief Initialize wifi.
 *
 * @return
 *     - NULL
 */
mdf_err_t wifi_init();

/**
 * @brief Periodically print system information.
 *
 * @param timer pointer of timer
 *
 * @return
 *     - NULL
 */
void show_system_info_timercb(void *timer);

/**
 * @brief   1.Get Mwifi initialization configuration information and Mwifi AP configuration information
 *      through the blufi or network configuration chain.
 *
 * @param init_config Mwifi initialization configuration
 * @param ap_config Mwifi AP configuration
 * @param tid device type id
 * @param nameprefix device name prefix
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t get_network_config(mwifi_init_config_t *init_config, mwifi_config_t *ap_config, uint16_t tid, char *name_prefix);

#ifdef __cplusplus
}
#endif

#endif /**< __MESH_UTILS_H__ */