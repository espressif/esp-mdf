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

#include "iot_touchpad.h"
#include "touchpad.h"
#include "mdf_common.h"
#include "mdf_trigger.h"
#include "mdf_low_power.h"
#include "mdf_event_loop.h"
#include "mdf_device_handle.h"

#define COVER_THICK_LINEAR_SLIDER CONFIG_COVER_THICK_LINEAR_SLIDER
#if COVER_THICK_LINEAR_SLIDER == 0      /**< No Cover */
#define TOUCH_BUTTON_MAX_CHANGE_RATE_0   0.57      // (1158-658) / 1158 = 0.57
#define TOUCH_BUTTON_MAX_CHANGE_RATE_1   0.53      // (1053-553) / 1053 = 0.53
#define TOUCH_BUTTON_MAX_CHANGE_RATE_2   0.51      // (1023-523) / 1023 = 0.51
#define TOUCH_SLIDE_MAX_CHANGE_RATE_0    0.31      // (1158-358) / 1158 = 0.31
#define TOUCH_SLIDE_MAX_CHANGE_RATE_1    0.34      // (1210-410) / 1210 = 0.34
#define TOUCH_SLIDE_MAX_CHANGE_RATE_2    0.35      // (1222-422) / 1222 = 0.35
#define TOUCH_SLIDE_MAX_CHANGE_RATE_3    0.47      // (1122-522) / 1122 = 0.47
#define TOUCH_SLIDE_MAX_CHANGE_RATE_4    0.53      // (1065-565) / 1065 = 0.53
#define TOUCH_SLIDE_MAX_CHANGE_RATE_5    0.51      // (1030-530) / 1030 = 0.51
#define TOUCH_SLIDE_PAD_RANGE            255       // the position range of slide
#elif COVER_THICK_LINEAR_SLIDER <= 1    /*!< Plastic cover thickness is 0 ~ 1 mm */
#define TOUCH_BUTTON_MAX_CHANGE_RATE_0   0.0777    // (1158-1068) / 1158 = 0.0777
#define TOUCH_BUTTON_MAX_CHANGE_RATE_1   0.0854    // (1053- 963) / 1053 = 0.0854
#define TOUCH_BUTTON_MAX_CHANGE_RATE_2   0.0880    // (1023- 933) / 1023 = 0.0880
#define TOUCH_SLIDE_MAX_CHANGE_RATE_0    0.0820    // (1158-1063) / 1158 = 0.0820
#define TOUCH_SLIDE_MAX_CHANGE_RATE_1    0.0744    // (1210-1120) / 1210 = 0.0744
#define TOUCH_SLIDE_MAX_CHANGE_RATE_2    0.0736    // (1222-1132) / 1222 = 0.0736
#define TOUCH_SLIDE_MAX_CHANGE_RATE_3    0.0802    // (1122-1032) / 1122 = 0.0802
#define TOUCH_SLIDE_MAX_CHANGE_RATE_4    0.0798    // (1065- 980) / 1065 = 0.0798
#define TOUCH_SLIDE_MAX_CHANGE_RATE_5    0.0777    // (1030- 950) / 1030 = 0.0777
#define TOUCH_SLIDE_PAD_RANGE            150       // the position range of slide
#else
#error "Invalid setting of plastic cover thickness."
#endif

#define TOUCH_SLIDE_0            TOUCH_PAD_NUM9
#define TOUCH_SLIDE_1            TOUCH_PAD_NUM7
#define TOUCH_SLIDE_2            TOUCH_PAD_NUM5
#define TOUCH_SLIDE_3            TOUCH_PAD_NUM3
#define TOUCH_SLIDE_4            TOUCH_PAD_NUM2
#define TOUCH_SLIDE_5            TOUCH_PAD_NUM0

#define TOUCH_BUTTON_R           TOUCH_PAD_NUM8
#define TOUCH_BUTTON_G           TOUCH_PAD_NUM6
#define TOUCH_BUTTON_B           TOUCH_PAD_NUM4

typedef struct {
    uint8_t button[3];
    uint8_t slide;
} touch_status_t;

static const char *TAG = "touchpad_oprt";
static CTouchPadSlide *g_tp_slide;
static touch_status_t g_touch_status;
static TimerHandle_t g_deepsleep_timer;
static bool g_button_cd[3] = { false, false, false };

void deepsleep_timer_pass(TimerHandle_t timer)
{
    g_deepsleep_timer = timer;
}

static void deepsleep_timer_reset()
{
    if (g_deepsleep_timer) {
        xTimerReset(g_deepsleep_timer, portMAX_DELAY);
    }
}

esp_err_t touchpad_set_value(touch_pad_t tp_num)
{
    int idx = 0;

    switch (tp_num) {
        case TOUCH_BUTTON_R:
        case TOUCH_BUTTON_G:
        case TOUCH_BUTTON_B:
            idx = tp_num == TOUCH_BUTTON_R ? 0 :
                  tp_num == TOUCH_BUTTON_G ? 1 :
                  tp_num == TOUCH_BUTTON_B ? 2 : 0;
            g_touch_status.button[idx] = 1;
            ch450_write_button(idx);
            break;

        case TOUCH_SLIDE_0:
        case TOUCH_SLIDE_1:
        case TOUCH_SLIDE_2:
        case TOUCH_SLIDE_3:
        case TOUCH_SLIDE_4:
        case TOUCH_SLIDE_5:
            g_touch_status.slide = (TOUCH_PAD_MAX - tp_num) * 10;
            ch450_write_pos(g_touch_status.slide);
            break;

        default:
            MDF_LOGW("unsupported tp_num: %d", tp_num);
            return ESP_FAIL;
    }

    mdf_trigger_isr();
    return ESP_OK;
}

esp_err_t touchpad_get_value(uint8_t cid, int *value)
{
    MDF_PARAM_CHECK(value);

    switch (cid) {
        case TOUCH_BUTTON_CID_R:
        case TOUCH_BUTTON_CID_G:
        case TOUCH_BUTTON_CID_B:
            *value = g_touch_status.button[cid];
            break;

        case TOUCH_SLIDE_CID:
            *value = g_touch_status.slide;
            break;

        default:
            MDF_LOGW("unsupported cid: %d", cid);
            return ESP_FAIL;
    }

    return ESP_OK;
}

static void touchpad_calibrate_number(touch_pad_t pad, float change_rate)
{
    uint16_t val = 0;
    int sum_val = 0;
    int avg_val = 0;
    const int calibrate_cnt = 64;
    const int min_value = 300;
    int threshold = 0;

    for (int i = 0; i < calibrate_cnt; ++i) {
        touch_pad_read(pad, &val);
        sum_val += val;
    }

    avg_val = sum_val / calibrate_cnt;

    if (avg_val < min_value) {
        MDF_LOGD("Touch pad #%d average reading is too low: %d (expecting at least: %d). "
                 "Not using for deep sleep wakeup", pad, avg_val, min_value);
        touch_pad_config(pad, 0);
    } else {
        threshold = avg_val - (avg_val * change_rate) * 0.8;
        MDF_LOGD("Touch pad #%d average: %d, wakeup threshold set to: %d", pad, avg_val, threshold);
        touch_pad_config(pad, threshold);
    }
}

void touchpad_calibrate(void)
{
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

    touch_pad_config(TOUCH_SLIDE_0, 0);
    touch_pad_config(TOUCH_SLIDE_1, 0);
    touch_pad_config(TOUCH_SLIDE_2, 0);
    touch_pad_config(TOUCH_SLIDE_3, 0);
    touch_pad_config(TOUCH_SLIDE_4, 0);
    touch_pad_config(TOUCH_SLIDE_5, 0);
    touch_pad_config(TOUCH_BUTTON_R, 0);
    touch_pad_config(TOUCH_BUTTON_G, 0);
    touch_pad_config(TOUCH_BUTTON_B, 0);

    touchpad_calibrate_number(TOUCH_SLIDE_0, TOUCH_SLIDE_MAX_CHANGE_RATE_0);
    touchpad_calibrate_number(TOUCH_SLIDE_1, TOUCH_SLIDE_MAX_CHANGE_RATE_1);
    touchpad_calibrate_number(TOUCH_SLIDE_2, TOUCH_SLIDE_MAX_CHANGE_RATE_2);
    touchpad_calibrate_number(TOUCH_SLIDE_3, TOUCH_SLIDE_MAX_CHANGE_RATE_3);
    touchpad_calibrate_number(TOUCH_SLIDE_4, TOUCH_SLIDE_MAX_CHANGE_RATE_4);
    touchpad_calibrate_number(TOUCH_SLIDE_5, TOUCH_SLIDE_MAX_CHANGE_RATE_5);
    touchpad_calibrate_number(TOUCH_BUTTON_R, TOUCH_BUTTON_MAX_CHANGE_RATE_0);
    touchpad_calibrate_number(TOUCH_BUTTON_G, TOUCH_BUTTON_MAX_CHANGE_RATE_1);
    touchpad_calibrate_number(TOUCH_BUTTON_B, TOUCH_BUTTON_MAX_CHANGE_RATE_2);
}

static void touch_slide_handle(void *arg)
{
    int pos_prev = 0;
    uint8_t pos = 0;

    for (;;) {
        pos = g_tp_slide->get_position();

        if (pos_prev != pos && pos != 0xff) {
            MDF_LOGI("slide pos evt[%d]", pos);
            ch450_write_pos(pos);
            pos_prev = pos;
            g_touch_status.slide = pos * 100 / TOUCH_SLIDE_PAD_RANGE;

            mdf_trigger_isr();
            deepsleep_timer_reset();
        }

        vTaskDelay(20 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

static bool is_all_button_pushed()
{
    return g_button_cd[0] && g_button_cd[1] && g_button_cd[2];
}

static esp_err_t touchpad_switch_mode()
{
    esp_err_t ret = ESP_OK;

    mdf_running_mode_t mode = mdf_get_running_mode();
    MDF_LOGI("mdf_get_running_mode %d", mode);

    if (mode == (mdf_running_mode_t)(POWER_ACTIVE | TRANS_WIFI_MESH)) {
        mode = (mdf_running_mode_t)(POWER_DEEP_SLEEP | TRANS_ESPNOW);
    } else {
        mode = (mdf_running_mode_t)(POWER_ACTIVE | TRANS_WIFI_MESH);
    }

    ret = mdf_set_running_mode(mode);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_set_running_mode, ret: %d", ret);
    MDF_LOGD("mdf_set_running_mode: %d", mode);

    if (mode == (mdf_running_mode_t)(POWER_DEEP_SLEEP | TRANS_ESPNOW)) {
        MDF_LOGI("esp deep sleep start");
        touchpad_calibrate();
        ch450_clear();
        ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());
        esp_deep_sleep_start();
    } else {
        esp_restart();
    }

    return ESP_OK;
}

static void touch_button_handle(void *arg)
{
    esp_err_t ret = ESP_OK;

    for (;;) {
        if (is_all_button_pushed()) {
            if (mdf_get_running_mode() == (mdf_running_mode_t)(POWER_DEEP_SLEEP | TRANS_ESPNOW)) {
                rgb_led_set(COLOR_LED_B, 100);
            } else {
                rgb_led_set(COLOR_LED_G, 100);
            }

            vTaskDelay(1000 / portTICK_RATE_MS);

            if (is_all_button_pushed()) {
                vTaskDelay(2000 / portTICK_RATE_MS);
                if (is_all_button_pushed()) {
                    MDF_LOGI("is_all_button_pushed for more than 3s, enter config network mode");
                    ret = mdf_network_clear_config();
                    MDF_ERROR_CONTINUE(ret < 0, "mdf_network_clear_config, ret: %d", ret);
                    ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, 0);
                    MDF_ERROR_CONTINUE(ret < 0, "mdf_event_loop_delay_send, ret: %d", ret);
                }
            } else {
                /**< push 3 button simultaneously for less than 2s, switch mode */
                ret = touchpad_switch_mode();
                if (ret < 0) {
                    rgb_led_set(COLOR_LED_B, 100);
                }
            }
        }

        vTaskDelay(20 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

static void touch_button_push_cb(void *arg)
{
    CTouchPad *tp_dev  = (CTouchPad *) arg;
    touch_pad_t tp_num = tp_dev->tp_num();
    MDF_LOGI("push callback of touch pad num %d", tp_num);

    int idx = tp_num == TOUCH_BUTTON_R ? 0 :
              tp_num == TOUCH_BUTTON_G ? 1 :
              tp_num == TOUCH_BUTTON_B ? 2 : 0;
    g_touch_status.button[idx] = 1;
    g_button_cd[idx] = true;
    ch450_write_button(idx);

    mdf_trigger_isr();
    deepsleep_timer_reset();
}

static void touch_button_release_cb(void *arg)
{
    CTouchPad *tp_dev  = (CTouchPad *) arg;
    touch_pad_t tp_num = tp_dev->tp_num();
    MDF_LOGI("release callback of touch pad num %d", tp_num);

    int idx = tp_num == TOUCH_BUTTON_R ? 0 :
              tp_num == TOUCH_BUTTON_G ? 1 :
              tp_num == TOUCH_BUTTON_B ? 2 : 0;
    g_touch_status.button[idx] = 0;
    g_button_cd[idx] = false;

    deepsleep_timer_reset();
}

void touchpad_init(void)
{
    CTouchPad *tp_dev0 = new CTouchPad(TOUCH_BUTTON_R, TOUCH_BUTTON_MAX_CHANGE_RATE_0);
    CTouchPad *tp_dev1 = new CTouchPad(TOUCH_BUTTON_G, TOUCH_BUTTON_MAX_CHANGE_RATE_1);
    CTouchPad *tp_dev2 = new CTouchPad(TOUCH_BUTTON_B, TOUCH_BUTTON_MAX_CHANGE_RATE_2);
    tp_dev0->add_cb(TOUCHPAD_CB_PUSH, touch_button_push_cb, tp_dev0);
    tp_dev1->add_cb(TOUCHPAD_CB_PUSH, touch_button_push_cb, tp_dev1);
    tp_dev2->add_cb(TOUCHPAD_CB_PUSH, touch_button_push_cb, tp_dev2);
    tp_dev0->add_cb(TOUCHPAD_CB_RELEASE, touch_button_release_cb, tp_dev0);
    tp_dev1->add_cb(TOUCHPAD_CB_RELEASE, touch_button_release_cb, tp_dev1);
    tp_dev2->add_cb(TOUCHPAD_CB_RELEASE, touch_button_release_cb, tp_dev2);

    const touch_pad_t tps[] = {
        TOUCH_SLIDE_0, TOUCH_SLIDE_1, TOUCH_SLIDE_2, TOUCH_SLIDE_3, TOUCH_SLIDE_4, TOUCH_SLIDE_5
    };
    const float variation[] = { TOUCH_SLIDE_MAX_CHANGE_RATE_0,
                                TOUCH_SLIDE_MAX_CHANGE_RATE_1,
                                TOUCH_SLIDE_MAX_CHANGE_RATE_2,
                                TOUCH_SLIDE_MAX_CHANGE_RATE_3,
                                TOUCH_SLIDE_MAX_CHANGE_RATE_4,
                                TOUCH_SLIDE_MAX_CHANGE_RATE_5
                              };
    g_tp_slide = new CTouchPadSlide(6, tps, TOUCH_SLIDE_PAD_RANGE, variation);

    xTaskCreate(touch_slide_handle, "slide_handle", 1024 * 3, NULL, MDF_TASK_DEFAULT_PRIOTY - 3, NULL);
    xTaskCreate(touch_button_handle, "button_handle", 1024 * 3, NULL, MDF_TASK_DEFAULT_PRIOTY - 3, NULL);
    memset(&g_touch_status, 0, sizeof(touch_status_t));
}
