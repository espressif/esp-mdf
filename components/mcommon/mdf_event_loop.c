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

        /**< Call callback function to dispatch event */
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
    MDF_ERROR_CHECK(!event_data, MDF_ERR_NO_MEM, "<MDF_ERR_NO_MEM> g_upgrade_config");

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

    /**< Set event loop callback function */
    g_event_handler_cb = cb;

    return old_cb;
}

mdf_err_t mdf_event_loop_init(mdf_event_loop_cb_t cb)
{
    MDF_ERROR_CHECK(g_event_queue_handle, MDF_ERR_NOT_SUPPORTED,
                    "The event loop has been initialized");
    MDF_PARAM_CHECK(cb);

    g_event_queue_handle = xQueueCreate(EVENT_QUEUE_NUM, sizeof(mdf_event_loop_data_t));

    /**
     * @brief Create a task to dispatch event.
     *      Because all the callbacks are dispatched from the same task,
     *      it is recommended to only do the minimal possible amount of work from the callback itself,
     *      posting an event to a lower priority task using a queue instead.
     */
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
