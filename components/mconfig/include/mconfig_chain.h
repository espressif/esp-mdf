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

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_CHAIN_H__ */

