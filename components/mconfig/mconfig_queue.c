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

#include "mconfig_queue.h"

static const char *TAG = "mconfig_queue";
static QueueHandle_t g_mconfig_queue = NULL;

mdf_err_t mconfig_queue_write(const mconfig_data_t *config_data, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(config_data);

    mconfig_data_t *q_data = MDF_MALLOC(sizeof(mconfig_data_t) + config_data->whitelist_size);
    MDF_ERROR_CHECK(!q_data, MDF_ERR_NO_MEM, "");

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

    if (!(*config_data = MDF_MALLOC(sizeof(mconfig_data_t) + q_data->whitelist_size))) {
        MDF_FREE(q_data);
        return MDF_ERR_NO_MEM;
    }

    memcpy(*config_data, q_data, sizeof(mconfig_data_t));
    memcpy((*config_data)->whitelist_data, q_data->whitelist_data, q_data->whitelist_size);

    MDF_FREE(q_data);

    return MDF_OK;
}
