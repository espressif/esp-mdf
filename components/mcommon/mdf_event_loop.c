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

#include "mdf_common.h"
#include "mdf_event_loop.h"

typedef struct {
    mdf_event_loop_t event;
    void *ctx;
} mdf_event_loop_data_t;

static xQueueHandle g_event_queue_handle      = NULL;
static TaskHandle_t g_event_task_handle       = NULL;
static mdf_event_loop_cb_t g_event_handler_cb = NULL;
static const char *TAG                        = "mdf_event_loop";

typedef mdf_err_t (*mdf_event_loop_cb_t)(mdf_event_loop_t event, void *ctx);

mdf_err_t mdf_event_loop(mdf_event_loop_t event, void *ctx)
{
    MDF_ERROR_CHECK(!g_event_queue_handle, MDF_ERR_NOT_INIT,
                    "The event loop isn't initialized");
    MDF_ERROR_CHECK(!g_event_handler_cb, MDF_ERR_NOT_INIT,
                    "Event handler is empty");

    return g_event_handler_cb(event, ctx);
}

static void mdf_event_loop_task(void *pvParameters)
{
    mdf_err_t ret                    = MDF_OK;
    mdf_event_loop_data_t event_data = {0x0};

    for (;;) {
        if (xQueueReceive(g_event_queue_handle, &event_data, portMAX_DELAY) != pdPASS) {
            continue;
        }

        if (!g_event_handler_cb) {
            MDF_LOGW("Event handler is empty");
            continue;
        }

        ret = (*g_event_handler_cb)(event_data.event, event_data.ctx);

        if (ret != MDF_OK) {
            MDF_LOGW("Event handling failed");
        }
    }

    vTaskDelete(NULL);
}

mdf_err_t mdf_event_loop_send(mdf_event_loop_t event, void *ctx)
{
    MDF_ERROR_CHECK(!g_event_queue_handle, MDF_ERR_NOT_INIT,
                    "The event loop isn't initialized");

    mdf_event_loop_data_t event_data = {
        .event = event,
        .ctx = ctx,
    };

    /**< If g_event_queue_handle is full, delete the front item */
    if (!uxQueueSpacesAvailable(g_event_queue_handle)) {
        mdf_event_loop_data_t queue_buf;
        xQueueReceive(g_event_queue_handle, &queue_buf, 0);
    }

    mdf_err_t ret = xQueueSend(g_event_queue_handle, &event_data, portMAX_DELAY);
    MDF_ERROR_CHECK(ret != pdTRUE, ESP_FAIL, "Send queue failed");

    return MDF_OK;
}

static void event_send_timer_cb(void *timer)
{
    mdf_event_loop_data_t *event_data = (mdf_event_loop_data_t *)pvTimerGetTimerID(timer);

    /**< If g_event_queue_handle is full, delete the front item */
    if (!uxQueueSpacesAvailable(g_event_queue_handle)) {
        mdf_event_loop_data_t queue_buf;
        xQueueReceive(g_event_queue_handle, &queue_buf, 0);
    }

    mdf_err_t ret = xQueueSend(g_event_queue_handle, event_data, 0);

    if (ret != pdTRUE) {
        MDF_LOGW("Send queue failed");
    }

    MDF_FREE(event_data);

    xTimerStop(timer, portMAX_DELAY);
    xTimerDelete(timer, portMAX_DELAY);
}

mdf_err_t mdf_event_loop_delay_send(mdf_event_loop_t event, void *ctx, TickType_t delay_ticks)
{
    MDF_ERROR_CHECK(!g_event_queue_handle, MDF_ERR_NOT_INIT,
                    "The event loop isn't initialized");

    if (delay_ticks == 0) {
        return mdf_event_loop_send(event, ctx);
    }

    mdf_event_loop_data_t *event_data = MDF_MALLOC(sizeof(mdf_event_loop_data_t));
    event_data->event = event;
    event_data->ctx   = ctx;

    TimerHandle_t timer = xTimerCreate("event_timer", delay_ticks,
                                       false, event_data, event_send_timer_cb);
    xTimerStart(timer, 0);

    return MDF_OK;
}

mdf_event_loop_cb_t mdf_event_loop_set(mdf_event_loop_cb_t cb)
{
    mdf_event_loop_cb_t old_cb = g_event_handler_cb;

    g_event_handler_cb = cb;

    return old_cb;
}

mdf_err_t mdf_event_loop_init(mdf_event_loop_cb_t cb)
{
    MDF_ERROR_CHECK(g_event_queue_handle, MDF_ERR_NOT_SUPPORTED,
                    "The event loop has been initialized");
    MDF_PARAM_CHECK(cb);

    g_event_queue_handle = xQueueCreate(EVENT_QUEUE_NUM, sizeof(mdf_event_loop_data_t));
    xTaskCreatePinnedToCore(mdf_event_loop_task, MDF_EVENT_TASK_NAME, MDF_EVENT_TASK_STACK,
                            NULL, MDF_EVENT_TASK_PRIOTY, &g_event_task_handle, CONFIG_MDF_TASK_PINNED_TO_CORE);

    g_event_handler_cb = cb;

    return MDF_OK;
}

mdf_err_t mdf_event_loop_deinit()
{
    MDF_ERROR_CHECK(!g_event_queue_handle, MDF_ERR_NOT_INIT,
                    "The event loop isn't initialized");

    vQueueDelete(g_event_queue_handle);
    g_event_queue_handle = NULL;

    vTaskDelete(g_event_task_handle);
    g_event_task_handle = NULL;

    g_event_handler_cb = NULL;

    return MDF_OK;
}
