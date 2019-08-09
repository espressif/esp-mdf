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

#ifndef __MCONFIG_CHAIN_H__
#define __MCONFIG_CHAIN_H__

#include "mdf_common.h"
#include "mconfig_queue.h"
#include "mdf_event_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDF_EVENT_MCONFIG_CHAIN_SLAVE_STARTED  (MDF_EVENT_MCONFIG_BASE + 0x101)
#define MDF_EVENT_MCONFIG_CHAIN_SLAVE_STOPED   (MDF_EVENT_MCONFIG_BASE + 0x102)
#define MDF_EVENT_MCONFIG_CHAIN_FINISH         (MDF_EVENT_MCONFIG_BASE + 0x103)
#define MDF_EVENT_MCONFIG_CHAIN_MASTER_STARTED (MDF_EVENT_MCONFIG_BASE + 0x104)
#define MDF_EVENT_MCONFIG_CHAIN_MASTER_STOPED  (MDF_EVENT_MCONFIG_BASE + 0x105)
#define MDF_EVENT_MCONFIG_CHAIN_FOUND_ROUTER   (MDF_EVENT_MCONFIG_BASE + 0x106)

/**
 * @brief  Chain configuration network slave initialization for obtaining network configuration information
 *
 * @attention The received network configuration information is sent to mconfig_queue
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mconfig_chain_slave_init(void);

/**
 * @brief  Disable slave to switch wifi channel
 *
 * @attention Chain configuration network slaves, will constantly switch channels
 *            to find the master, if you need to make a wifi connection, You must
 *            first disable the switch of the slave wifi channel.
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mconfig_chain_slave_channel_switch_disable(void);

/**
 * @brief  Enable slave to switch wifi channel
 *
 * @attention The slave will continuously switch the wifi channel for scanning,
 *            which is enabled by default.
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mconfig_chain_slave_channel_switch_enable(void);

/**
 * @brief  Free all resource allocated in mconfig_chain_slave_init and stop chain_slave task
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_NOT_SUPPORTED
 */
mdf_err_t mconfig_chain_slave_deinit(void);

/**
 * @brief  Chain configuration network host initialization,
 *         sending network configuration information to the slave
 *
 * @param  config This configuration information will be sent to the slave device.
 * @param  duration_ticks Stop chain_master after the set time
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_NOT_SUPPORTED
 */
mdf_err_t mconfig_chain_master(const mconfig_data_t *config, TickType_t duration_ticks);


/**
 * @brief  Chain Devices with weak rssi are not allowed to join the network.
 *
 * @note   If you use it, you must call after `mconfig_chain_master`
 *
 * @param  rssi When the device signal strength is less than this value, it will not join the network.
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mconfig_chain_filter_rssi(int8_t rssi);

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_CHAIN_H__ */

