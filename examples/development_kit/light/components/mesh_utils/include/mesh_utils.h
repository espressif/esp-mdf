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