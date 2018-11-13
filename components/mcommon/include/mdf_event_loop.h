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

#ifndef CONFIG_MDF_EVENT_TASK_STACK
#define CONFIG_MDF_EVENT_TASK_STACK     (4096)
#endif  /**< CONFIG_MDF_EVENT_TASK_STACK */
#define MDF_EVENT_TASK_STACK CONFIG_MDF_EVENT_TASK_STACK

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
