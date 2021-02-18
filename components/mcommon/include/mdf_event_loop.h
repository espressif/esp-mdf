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

#ifndef __MDF_EVENT_LOOP_H__
#define __MDF_EVENT_LOOP_H__

#include "mdf_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t mdf_event_loop_t;

#ifndef CONFIG_EVENT_QUEUE_NUM
#define CONFIG_EVENT_QUEUE_NUM          (20)
#endif  /**< CONFIG_EVENT_QUEUE_NUM */
#define EVENT_QUEUE_NUM CONFIG_EVENT_QUEUE_NUM

#ifndef CONFIG_MDF_EVENT_TASK_NAME
#define CONFIG_MDF_EVENT_TASK_NAME      "mdf_event_loop"
#endif  /**< CONFIG_MDF_EVENT_TASK_NAME */
#define MDF_EVENT_TASK_NAME CONFIG_MDF_EVENT_TASK_NAME

#ifndef CONFIG_MDF_EVENT_TASK_STACK_SIZE
#define CONFIG_MDF_EVENT_TASK_STACK_SIZE     (4096)
#endif  /**< CONFIG_MDF_EVENT_TASK_STACK */
#define MDF_EVENT_TASK_STACK CONFIG_MDF_EVENT_TASK_STACK_SIZE

#ifndef CONFIG_MDF_EVENT_TASK_PRIOTY
#define CONFIG_MDF_EVENT_TASK_PRIOTY    ((tskIDLE_PRIORITY) + 15)
#endif  /**< CONFIG_MDF_EVENT_TASK_PRIOTY */
#define MDF_EVENT_TASK_PRIOTY CONFIG_MDF_EVENT_TASK_PRIOTY

#define MDF_EVENT_MWIFI_BASE            0x0
#define MDF_EVENT_MESPNOW_BASE          0x1000
#define MDF_EVENT_MCONFIG_BASE          0x2000
#define MDF_EVENT_MUPGRADE_BASE         0x3000
#define MDF_EVENT_MDEBUG_BASE           0x4000
#define MDF_EVENT_MLINK_BASE            0x5000
#define MDF_EVENT_CUSTOM_BASE           0x6000

/**
 * @brief  Application specified event callback function
 *
 * @param  event Event type defined in this file
 * @param  ctx   Reserved for user
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
typedef mdf_err_t (*mdf_event_loop_cb_t)(mdf_event_loop_t event, void *ctx);

/**
 * @brief  Initialize event loop, create the event handler and task
 *
 * @attention   Because all the callbacks are dispatched from the same task,
 *              it is recommended to only do the minimal possible amount of work from the callback itself,
 *              posting an event to a lower priority task using a queue instead.
 *
 * @param  cb Application specified event callback, it can be modified by call mdf_event_loop_set
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdf_event_loop_init(mdf_event_loop_cb_t cb);

/**
 * @brief  Deinitialize event loop, delete the event handler and task
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdf_event_loop_deinit();

/**
 * @brief  Call event callback function directly
 *
 * @param  event Event type defined in this file
 * @param  ctx   Reserved for user
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdf_event_loop(mdf_event_loop_t event, void *ctx);

/**
 * @brief  Set event loop callback function
 *
 * @param  cb Set application event callback
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_event_loop_cb_t mdf_event_loop_set(mdf_event_loop_cb_t cb);

/**
 * @brief  Send the event to the event handler
 *
 * @param  event Generated events
 * @param  ctx   Reserved for user
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdf_event_loop_send(mdf_event_loop_t event, void *ctx);

/**
 * @brief  Delay send the event to the event handler
 *
 * @param  event       Generated events
 * @param  ctx         Reserved for user
 * @param  delay_ticks Delay time
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdf_event_loop_delay_send(mdf_event_loop_t event, void *ctx, TickType_t delay_ticks);

#ifdef __cplusplus
}
#endif

#endif /* __MDF_EVENT_LOOP_H__ */
