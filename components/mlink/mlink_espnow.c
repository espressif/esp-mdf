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

mdf_err_t mlink_espnow_write(const uint8_t *addrs_list, size_t addrs_num, const void *data,
                             size_t size, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(addrs_list);
    MDF_PARAM_CHECK(size > 0);

    mdf_err_t ret      = MDF_OK;
    size_t espnow_size = sizeof(mlink_espnow_t) + size + addrs_num * ESP_NOW_ETH_ALEN;
    mlink_espnow_t *espnow_data = MDF_MALLOC(espnow_size);

    espnow_data->size      = size;
    espnow_data->addrs_num = addrs_num;
    memcpy(espnow_data->data, data, size);
    memcpy(espnow_data->data + size, addrs_list, addrs_num * ESP_NOW_ETH_ALEN);

    ret = mespnow_write(MESPNOW_TRANS_PIPE_CONTROL, g_espnow_config.parent_bssid,
                        espnow_data, espnow_size, wait_ticks);
    MDF_FREE(espnow_data);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mespnow_write");

    return ret;
}

mdf_err_t mlink_espnow_read(uint8_t **addrs_list, size_t *addrs_num, uint8_t **data,
                            size_t *size, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(size);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(addrs_num);
    MDF_PARAM_CHECK(addrs_list);

    mdf_err_t ret      = MDF_OK;
    size_t espnow_size = ESP_NOW_MAX_DATA_LEN;
    uint8_t src_addr[ESP_NOW_ETH_ALEN] = {0x0};
    *size = 0;
    *data = NULL;
    *addrs_num = 0;

    mlink_espnow_t *espnow_data = MDF_MALLOC(ESP_NOW_MAX_DATA_LEN);
    ret = mespnow_read(MESPNOW_TRANS_PIPE_CONTROL, src_addr,
                       espnow_data, &espnow_size, wait_ticks);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mespnow_read");

    *size = espnow_data->size;
    *data = MDF_MALLOC(espnow_data->size);
    *addrs_num  = espnow_data->addrs_num;
    *addrs_list = MDF_MALLOC(*addrs_num * ESP_NOW_ETH_ALEN);
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

    /* espnow need to set the channel */
    ESP_ERROR_CHECK(esp_wifi_set_channel(config->channel, second));
    ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_STA, config->parent_bssid, NULL));
    memcpy(&g_espnow_config, config, sizeof(mlink_espnow_config_t));

    return MDF_OK;
}
