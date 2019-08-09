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
#include "mlink.h"

#include "light_driver.h"
#include "light_handle.h"

static const char *TAG = "light_handle";

mdf_err_t light_show_layer(mlink_handle_data_t *handle_data)
{
    switch (esp_mesh_get_layer()) {
        case 1:
            light_driver_set_rgb(255, 0, 0);   /**< red */
            break;

        case 2:
            light_driver_set_rgb(255, 128, 0); /**< orange */
            break;

        case 3:
            light_driver_set_rgb(255, 255, 0); /**< yellow */
            break;

        case 4:
            light_driver_set_rgb(0, 255, 0);   /**< green */
            break;

        case 5:
            light_driver_set_rgb(0, 255, 255); /**< cyan */
            break;

        case 6:
            light_driver_set_rgb(0, 0, 255);   /**< blue */
            break;

        case 7:
            light_driver_set_rgb(128, 0, 255); /**< purple */
            break;

        default:
            light_driver_set_rgb(255, 255, 255); /**< white */
            break;
    }

    return MDF_OK;
}

mdf_err_t light_get_tsf_time(mlink_handle_data_t *handle_data)
{
    char tsf_time_str[16] = {0x0};

    sprintf(tsf_time_str, "%lld", esp_mesh_get_tsf_time());
    mlink_json_pack(&handle_data->resp_data, "tsf_time", tsf_time_str);

    handle_data->resp_size = strlen(handle_data->resp_data);

    return MDF_OK;
}

mdf_err_t mlink_set_value(uint16_t cid, void *arg)
{
    int value = *((int *)arg);

    switch (cid) {
        case LIGHT_CID_STATUS:
            switch (value) {
                case LIGHT_STATUS_ON:
                case LIGHT_STATUS_OFF:
                    light_driver_set_switch(value);
                    break;

                case LIGHT_STATUS_SWITCH:
                    light_driver_set_switch(!light_driver_get_switch());
                    break;

                case LIGHT_STATUS_HUE: {
                    uint16_t hue = light_driver_get_hue();
                    hue = (hue + 60) % 360;

                    light_driver_set_saturation(100);
                    light_driver_set_hue(hue);
                    break;
                }

                case LIGHT_STATUS_BRIGHTNESS: {
                    if (light_driver_get_mode() == MODE_HSV) {
                        uint8_t value = (light_driver_get_value() + 20) % 100;
                        light_driver_set_value(value);
                    } else {
                        uint8_t brightness = (light_driver_get_brightness() + 20) % 100;
                        light_driver_set_brightness(brightness);
                    }

                    break;
                }

                case LIGHT_STATUS_COLOR_TEMPERATURE: {
                    uint8_t color_temperature = (light_driver_get_color_temperature() + 20) % 100;

                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_set_color_temperature(color_temperature);

                    break;
                }

                default:
                    break;
            }

            break;

        case LIGHT_CID_MODE:
            switch (value) {
                case MODE_BRIGHTNESS_INCREASE:
                    light_driver_fade_brightness(100);
                    break;

                case MODE_BRIGHTNESS_DECREASE:
                    light_driver_fade_brightness(0);
                    break;

                case MODE_HUE_INCREASE:
                    light_driver_set_saturation(100);
                    light_driver_fade_hue(360);
                    break;

                case MODE_HUE_DECREASE:
                    light_driver_set_saturation(100);
                    light_driver_fade_hue(0);
                    break;

                case MODE_WARM_INCREASE:
                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_fade_warm(100);
                    break;

                case MODE_WARM_DECREASE:
                    if (!light_driver_get_brightness()) {
                        light_driver_set_brightness(30);
                    }

                    light_driver_fade_warm(0);
                    break;

                case MODE_NONE:
                    light_driver_fade_stop();
                    break;

                default:
                    break;
            }

            break;

        case LIGHT_CID_HUE:
            light_driver_set_hue(value);
            break;

        case LIGHT_CID_SATURATION:
            light_driver_set_saturation(value);
            break;

        case LIGHT_CID_VALUE:
            light_driver_set_value(value);
            break;

        case LIGHT_CID_COLOR_TEMPERATURE:
            light_driver_set_color_temperature(value);
            break;

        case LIGHT_CID_BRIGHTNESS:
            light_driver_set_brightness(value);
            break;

        default:
            MDF_LOGE("No support cid: %d", cid);
            return MDF_FAIL;
    }

    MDF_LOGD("cid: %d, value: %d", cid, value);

    return MDF_OK;
}

mdf_err_t mlink_get_value(uint16_t cid, void *arg)
{
    int *value = (int *)arg;

    switch (cid) {
        case LIGHT_CID_STATUS:
            *value = light_driver_get_switch();
            break;

        case LIGHT_CID_HUE:
            *value = light_driver_get_hue();
            break;

        case LIGHT_CID_SATURATION:
            *value = light_driver_get_saturation();
            break;

        case LIGHT_CID_VALUE:
            *value = light_driver_get_value();
            break;

        case LIGHT_CID_COLOR_TEMPERATURE:
            *value = light_driver_get_color_temperature();
            break;

        case LIGHT_CID_BRIGHTNESS:
            *value = light_driver_get_brightness();
            break;

        case LIGHT_CID_MODE:
            *value = light_driver_get_mode();
            break;

        default:
            MDF_LOGE("No support cid: %d", cid);
            return MDF_FAIL;
    }

    MDF_LOGV("cid: %d, value: %d", cid, *value);

    return MDF_OK;
}
