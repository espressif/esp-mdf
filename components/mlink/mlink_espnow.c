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

#include "esp_wifi.h"
#include "esp_now.h"

#include "mespnow.h"
#include "mlink_espnow.h"

static const char *TAG                       = "mlink_espnow";
static mlink_espnow_config_t g_espnow_config = {0};

/**
 * @brief Espnow data communication format
 */
typedef struct {
    uint32_t type;       /**< Data type */
    size_t addrs_num;    /**< Number of addresses */
    size_t size;         /**< Length of data */
    char data[0];        /**< Pointer of data */
} mlink_espnow_t;

mdf_err_t __mlink_espnow_write(const uint8_t *addrs_list, size_t addrs_num, const void *data,
                               size_t size, uint32_t type, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(addrs_list);
    MDF_PARAM_CHECK(size > 0);

    mdf_err_t ret      = MDF_OK;
    size_t espnow_size = sizeof(mlink_espnow_t) + size + addrs_num * ESP_NOW_ETH_ALEN;
    mlink_espnow_t *espnow_data = MDF_MALLOC(espnow_size);
    MDF_ERROR_CHECK(!espnow_data, MDF_ERR_NO_MEM, "");

    espnow_data->size      = size;
    espnow_data->type      = type;
    espnow_data->addrs_num = addrs_num;
    memcpy(espnow_data->data, data, size);
    memcpy(espnow_data->data + size, addrs_list, addrs_num * ESP_NOW_ETH_ALEN);

    /**< write date package to espnow. */
    ret = mespnow_write(MESPNOW_TRANS_PIPE_CONTROL, g_espnow_config.parent_bssid,
                        espnow_data, espnow_size, wait_ticks);
    MDF_FREE(espnow_data);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mespnow_write");

    return ret;
}

mdf_err_t __mlink_espnow_read(uint8_t **addrs_list, size_t *addrs_num, uint8_t **data,
                              size_t *size, uint32_t *type, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(size);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(addrs_num);
    MDF_PARAM_CHECK(addrs_list);

    mdf_err_t ret      = MDF_OK;
    size_t espnow_size = ESP_NOW_MAX_DATA_LEN * 4;
    uint8_t src_addr[ESP_NOW_ETH_ALEN] = {0x0};
    *size = 0;
    *data = NULL;
    *addrs_num = 0;

    mlink_espnow_t *espnow_data = MDF_MALLOC(ESP_NOW_MAX_DATA_LEN * 4);
    MDF_ERROR_CHECK(!espnow_data, MDF_ERR_NO_MEM, "");

    /**< read data from espnow */
    ret = mespnow_read(MESPNOW_TRANS_PIPE_CONTROL, src_addr,
                       espnow_data, &espnow_size, wait_ticks);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "mespnow_read");

    *size = espnow_data->size;
    *data = MDF_MALLOC(espnow_data->size);
    *addrs_num  = espnow_data->addrs_num;
    *addrs_list = MDF_MALLOC(*addrs_num * ESP_NOW_ETH_ALEN);

    if (!(*data) || !(*addrs_list)) {
        ret = MDF_ERR_NO_MEM;
        MDF_FREE(*data);
        MDF_FREE(*addrs_list);
    }

    if (type) {
        *type = espnow_data->type;
    }

    memcpy(*data, espnow_data->data, espnow_data->size);
    memcpy(*addrs_list, espnow_data->data + *size, *addrs_num * ESP_NOW_ETH_ALEN);

EXIT:
    MDF_FREE(espnow_data);
    return ret;
}

mdf_err_t mlink_espnow_init(mlink_espnow_config_t *config)
{
    MDF_PARAM_CHECK(config);

    mdf_err_t ret = MDF_OK;
    wifi_second_chan_t second = 0;

    ret = mespnow_init();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mespnow_init");

    /**< espnow need to set the channel */
    esp_wifi_set_promiscuous(1);
    ESP_ERROR_CHECK(esp_wifi_set_channel(config->channel, second));
    ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_STA, config->parent_bssid, NULL));
    memcpy(&g_espnow_config, config, sizeof(mlink_espnow_config_t));

    return MDF_OK;
}
