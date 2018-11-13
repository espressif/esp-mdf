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

#include "mconfig_queue.h"

static const char *TAG = "mconfig_queue";
static QueueHandle_t g_mconfig_queue = NULL;

mdf_err_t mconfig_queue_write(const mconfig_data_t *config_data, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(config_data);

    mconfig_data_t *q_data = MDF_MALLOC(sizeof(mconfig_data_t) + config_data->whitelist_size);

    if (!g_mconfig_queue) {
        g_mconfig_queue = xQueueCreate(1, sizeof(mconfig_data_t *));
    }

    memcpy(q_data, config_data, sizeof(mconfig_data_t));
    memcpy(q_data->whitelist_data, config_data->whitelist_data, config_data->whitelist_size);

    if (!xQueueSend(g_mconfig_queue, &q_data, wait_ticks)) {
        MDF_LOGW("xQueueSend failed");
        MDF_FREE(q_data);
        return MDF_ERR_TIMEOUT;
    }

    return MDF_OK;
}

mdf_err_t mconfig_queue_read(mconfig_data_t **config_data, TickType_t wait_ticks)
{
    mconfig_data_t *q_data = NULL;

    if (!g_mconfig_queue) {
        g_mconfig_queue = xQueueCreate(1, sizeof(mconfig_data_t *));
    }

    if (!xQueueReceive(g_mconfig_queue, &q_data, wait_ticks)) {
        MDF_LOGW("xQueueSend failed");
        return MDF_ERR_TIMEOUT;
    }

    *config_data = MDF_MALLOC(sizeof(mconfig_data_t) + q_data->whitelist_size);

    memcpy(*config_data, q_data, sizeof(mconfig_data_t));
    memcpy((*config_data)->whitelist_data, q_data->whitelist_data, q_data->whitelist_size);

    MDF_FREE(q_data);

    return MDF_OK;
}
