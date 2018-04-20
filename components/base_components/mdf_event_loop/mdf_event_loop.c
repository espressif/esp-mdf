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

static const char *TAG = "mdf_event_loop";

typedef struct {
    mdf_event_loop_t event;
    void *ctx;
} mdf_event_loop_data_t;

static xQueueHandle xQueueEvent               = NULL;
static mdf_event_loop_cb_t g_event_handler_cb = NULL;

static void mdf_event_loop_task(void *pvParameters)
{
    esp_err_t ret = ESP_OK;

    mdf_event_loop_data_t event_data;

    for (;;) {
        if (xQueueReceive(xQueueEvent, &event_data, portMAX_DELAY) != pdPASS) {
            continue;
        }

        ret = (*g_event_handler_cb)(event_data.event, event_data.ctx);

        if (ret != ESP_OK) {
            MDF_LOGW("Event handling failed");
        }
    }

    vTaskDelete(NULL);
}

esp_err_t mdf_event_loop_send(mdf_event_loop_t event, void *ctx)
{
    MDF_ERROR_CHECK(!xQueueEvent, ESP_FAIL, "The event was not initialized");

    mdf_event_loop_data_t event_data = {
        .event = event,
        .ctx = ctx,
    };
    esp_err_t ret = xQueueSend(xQueueEvent, &event_data, 0);
    MDF_ERROR_CHECK(ret != pdTRUE, ESP_FAIL, "event xQueueSend fail");

    return ESP_OK;
}

static void event_send_timeout_cb(void *timer)
{
    MDF_LOGV("enter event_send_timeout_cb\n");

    mdf_event_loop_data_t *event_data = (mdf_event_loop_data_t *)pvTimerGetTimerID(timer);

    esp_err_t ret = xQueueSend(xQueueEvent, event_data, 0);

    if (ret < 0) {
        MDF_LOGW("event xQueueSend fail");
    }

    mdf_free(event_data);

    xTimerStop(timer, portMAX_DELAY);
    xTimerDelete(timer, portMAX_DELAY);
}

esp_err_t mdf_event_loop_delay_send(mdf_event_loop_t event, void *ctx, uint32_t delay_ms)
{
    MDF_ERROR_CHECK(!xQueueEvent, ESP_FAIL, "The event was not initialized");

    if (delay_ms == 0) {
        return mdf_event_loop_send(event, ctx);
    }

    mdf_event_loop_data_t *event_data = mdf_malloc(sizeof(mdf_event_loop_data_t));
    event_data->event = event;
    event_data->ctx = ctx;

    TimerHandle_t timer = xTimerCreate("event_timer",  delay_ms / portTICK_RATE_MS,
                                       false, event_data, event_send_timeout_cb);
    xTimerStart(timer, 0);

    return ESP_OK;
}

esp_err_t mdf_event_loop_init(mdf_event_loop_cb_t cb)
{
    MDF_PARAM_CHECK(cb);
    MDF_ERROR_CHECK(xQueueEvent, ESP_OK, "The event has been initialized");

    xQueueEvent = xQueueCreate(EVENT_QUEUE_NUM, sizeof(mdf_event_loop_data_t));
    xTaskCreate(mdf_event_loop_task, MDF_EVENT_TASK_NAME, MDF_EVENT_TASK_STACK,
                NULL, MDF_EVENT_TASK_PRIOTY, NULL);

    g_event_handler_cb = cb;

    MDF_LOGD("mdf_event_loop_init success");

    return ESP_OK;
}

esp_err_t mdf_event_loop_set(mdf_event_loop_cb_t cb)
{
    g_event_handler_cb = cb;

    return ESP_OK;
}
