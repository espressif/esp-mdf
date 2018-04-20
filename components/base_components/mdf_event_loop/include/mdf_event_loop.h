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

#ifndef __MDF_EVENT_LOOP_H__
#define __MDF_EVENT_LOOP_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_EVENT_QUEUE_NUM
#define CONFIG_EVENT_QUEUE_NUM               10
#endif  /**< CONFIG_EVENT_QUEUE_NUM */
#define EVENT_QUEUE_NUM                      CONFIG_EVENT_QUEUE_NUM

#ifndef CONFIG_MDF_EVENT_TASK_NAME
#define CONFIG_MDF_EVENT_TASK_NAME          ("mesh_event_task")
#endif  /**< CONFIG_MDF_EVENT_TASK_NAME */
#define MDF_EVENT_TASK_NAME                 CONFIG_MDF_EVENT_TASK_NAME

#ifndef CONFIG_MDF_EVENT_TASK_STACK
#define CONFIG_MDF_EVENT_TASK_STACK         (2048)
#endif  /**< CONFIG_MDF_EVENT_TASK_STACK */
#define MDF_EVENT_TASK_STACK                CONFIG_MDF_EVENT_TASK_STACK

#ifndef CONFIG_MDF_EVENT_TASK_PRIOTY
#define CONFIG_MDF_EVENT_TASK_PRIOTY        ((tskIDLE_PRIORITY) + 10)
#endif  /**< CONFIG_MDF_EVENT_TASK_PRIOTY */
#define MDF_EVENT_TASK_PRIOTY               CONFIG_MDF_EVENT_TASK_PRIOTY

typedef uint32_t mdf_event_loop_t;

enum {
    MDF_EVENT_NETWORK_START_CONFIG = 64, /**< the equipment enters the distribution mode */
    MDF_EVENT_CONNECT_ROUTER,            /**< verify the correctness of the router password */
    MDF_EVENT_NETWORK_ORGANIZE,          /**< verify the correctness of the router password */
    MDF_EVENT_NETWORK_SUCCESS,           /**< the equipment enters the distribution mode */
    MDF_EVENT_NETWORK_FAIL,              /**< the equipment enters the distribution mode */
    MDF_EVENT_NETWORK_ENABLE_AUTO,       /**< enter the auto network mode */

    MDF_EVENT_SYSTEM_RUNNING,            /**< running normally */
    MDF_EVENT_SYSTEM_RESET,              /**< restore factory settings */
    MDF_EVENT_SYSTEM_REBOOT,             /**< the system reboots */
    MDF_EVENT_SYSTEM_CRASH,              /**< the system crash */

    MDF_EVENT_UPGRADE_START,             /**< start the upgrade */
    MDF_EVENT_UPGRADE_DATA_FINISH,       /**< upgrade data reception completed */
    MDF_EVENT_UPGRADE_SUCCESS,           /**< upgrade succed */
    MDF_EVENT_UPGRADE_FAIL,              /**< upgrade failed */

    MDF_EVENT_SNIFFER_BUFFER_FULL,       /**< sniffer's buffer is full */
};

/**
 * @brief  application specified event callback function
 *
 * @param  event event type defined in this file
 * @param  ctx   reserved for user
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
typedef esp_err_t (*mdf_event_loop_cb_t)(mdf_event_loop_t event, void *ctx);

/**
 * @brief  initialize event loop, create the event handler and task
 *
 * @param  cb application specified event callback, it can be modified by call mdf_event_loop_set
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t mdf_event_loop_init(mdf_event_loop_cb_t cb);

/**
 * @brief  set event loop callback function
 *
 * @param  cb set application event callback
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t mdf_event_loop_set(mdf_event_loop_cb_t cb);

/**
 * @brief  send the event to the event handler
 *
 * @param  event generated events
 * @param  ctx   reserved for user
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t mdf_event_loop_send(mdf_event_loop_t event, void *ctx);

/**
 * @brief  delay send the event to the event handler
 *
 * @param  event    Generated events
 * @param  ctx      reserved for user
 * @param  delay_ms Delay time
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t mdf_event_loop_delay_send(mdf_event_loop_t event, void *ctx, uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MDF_EVENT_LOOP_H__ */
