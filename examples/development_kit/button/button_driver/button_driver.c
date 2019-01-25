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

#include "esp_adc_cal.h"

#include "mdf_common.h"
#include "button_driver.h"
#include "iot_light.h"

#define BUTTON_KEY_NUM                     (4)
#define BUTTON_GPIO_KEY0                   GPIO_NUM_39
#define BUTTON_GPIO_KEY1                   GPIO_NUM_34
#define BUTTON_GPIO_KEY2                   GPIO_NUM_32
#define BUTTON_GPIO_KEY3                   GPIO_NUM_35

#define BUTTON_LED_NUM                     (3)
#define BUTTON_GPIO_LED_RED                GPIO_NUM_27
#define BUTTON_GPIO_LED_GREEN              GPIO_NUM_26
#define BUTTON_GPIO_LED_BLUE               GPIO_NUM_14

#define BUTTON_GPIO_POWER_SWITCH           GPIO_NUM_5
#define BUTTON_GPIO_USB_CHARGE_STATUS      GPIO_NUM_37
#define BUTTON_GPIO_USB_DETECT             GPIO_NUM_38

#define BUTTON_GPIO_RS_FLIPFLOP_SET        GPIO_NUM_33
#define BUTTON_GPIO_RS_FLIPFLOP_RST        GPIO_NUM_15

#define BUTTON_BATTERY_ADC_CHANNEL         ADC1_CHANNEL_0
#define BUTTON_BATTERY_ADC_V_REF           (1100)
#define BUTTON_BATTERY_ADC_DIV             (10)

#define MDF_EVENT_BUTTON_KEY_LONG_PRESS_MS (2000)
#define RTC_SLOW_MEM                       ((uint32_t*) (0x50000000))       /*!< RTC slow memory, 8k size */

#define BUTTON_LED_TIMER_BIT               (LEDC_TIMER_12_BIT)
#define BUTTON_LED_FULL_DUTY               ((1 << BUTTON_LED_TIMER_BIT) - 1)

/**
 * @brief Key status
 */
typedef struct {
    gpio_num_t gpio_num;
    button_key_status_t status;
    bool push;
    bool release;
    int backup_tickcount;
} button_key_t;

/**
 * @brief The id of the channel corresponding to the led light
 */
enum light_channel {
    CHANNEL_ID_RED   = 0,
    CHANNEL_ID_GREEN = 1,
    CHANNEL_ID_BLUE  = 2,
};

static const char *TAG                    = "button_driver";
static xQueueHandle g_event_queue         = NULL;
static light_handle_t g_button_led_handle = NULL;
static bool g_button_led_blink_flag       = false;
static button_key_t g_button_key[BUTTON_KEY_NUM]               = {0x0};
static esp_adc_cal_characteristics_t g_adc_cal_characteristics = {0x0};

bool button_usb_is_connected()
{
    return gpio_get_level(BUTTON_GPIO_USB_DETECT);
}

mdf_err_t button_driver_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    mdf_err_t ret = MDF_OK;

    /**< NOTE: repair the pwd output unbalance */
    red = red / 2 * 180 / 255;
    green = green * 180 / 255;
    blue = blue * 180 / 255;

    if (g_button_led_blink_flag) {
        button_driver_led_blink_stop();
    }

    ret = iot_light_fade_with_time(g_button_led_handle, CHANNEL_ID_RED, red * BUTTON_LED_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_button_led_handle, CHANNEL_ID_GREEN, green * BUTTON_LED_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_button_led_handle, CHANNEL_ID_BLUE, blue * BUTTON_LED_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t button_driver_led_blink_start(uint8_t red, uint8_t green, uint8_t blue, uint32_t period_ms)
{
    mdf_err_t ret      = MDF_OK;

    /**< NOTE: repair the pwd output unbalance */
    red   = red / 2 * 128 / 255;
    green = green * 128 / 255;
    blue  = blue * 128 / 255;

    if (g_button_led_blink_flag) {
        button_driver_led_blink_stop();
    }

    ret = iot_light_breath_config(g_button_led_handle, CHANNEL_ID_RED,
                                  red * BUTTON_LED_FULL_DUTY / 255, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_button_led_handle, CHANNEL_ID_GREEN,
                                  green * BUTTON_LED_FULL_DUTY / 255, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_button_led_handle, CHANNEL_ID_BLUE,
                                  blue * BUTTON_LED_FULL_DUTY / 255, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);

    ret = iot_light_operate_start(g_button_led_handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_button_led_handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_button_led_handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);

    g_button_led_blink_flag = true;
    return MDF_OK;
}

mdf_err_t button_driver_led_blink_stop()
{
    mdf_err_t ret = MDF_OK;

    if (g_button_led_blink_flag == false) {
        return MDF_OK;
    }

    ret = iot_light_operate_stop(g_button_led_handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    ret = iot_light_operate_stop(g_button_led_handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    ret = iot_light_operate_stop(g_button_led_handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    return MDF_OK;
}

bool button_battery_get_status()
{
    if (gpio_get_level(BUTTON_GPIO_USB_CHARGE_STATUS)) {
        MDF_LOGV("Battery charging completed");
        return true;
    }

    return false;
}

uint8_t button_battery_get_electricity()
{
    int adc_data     = adc1_get_raw(BUTTON_BATTERY_ADC_CHANNEL);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_data, &g_adc_cal_characteristics) * BUTTON_BATTERY_ADC_DIV;
    uint8_t electricity = (voltage - 2700) * 100 / (4100 - 2700);
    MDF_LOGD("Usb charging, battery voltage: %dmV, electricity: %d%%", voltage, electricity);

    if (voltage < 2700) {
        MDF_LOGW("Low battery");
        return 0;
    } else if (electricity > 100 || button_battery_get_status()) {
        return 100;
    }

    return electricity;
}

static void button_is_charging_timercb(void *timer)
{
    if (!button_usb_is_connected()) {
        mdf_event_loop_send(MDF_EVENT_BUTTON_CHARGING_STOPED, NULL);
    } else if (gpio_get_level(BUTTON_GPIO_USB_CHARGE_STATUS)) {
        mdf_event_loop_send(MDF_EVENT_BUTTON_CHARGING_COMPLETED, NULL);
    }
}

mdf_err_t button_battery_init()
{
    mdf_err_t ret = MDF_OK;

    gpio_config_t battery_config = {
        .pin_bit_mask = BIT64(BUTTON_GPIO_USB_DETECT) | BIT64(BUTTON_GPIO_USB_CHARGE_STATUS),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    gpio_config_t ff_config = {
        .pin_bit_mask = BIT64(BUTTON_GPIO_RS_FLIPFLOP_SET) | BIT64(BUTTON_GPIO_RS_FLIPFLOP_RST),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    gpio_config(&battery_config);
    gpio_config(&ff_config);

    gpio_set_level(BUTTON_GPIO_RS_FLIPFLOP_SET, 0);
    gpio_set_level(BUTTON_GPIO_RS_FLIPFLOP_RST, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(BUTTON_GPIO_RS_FLIPFLOP_RST, 0);

    /**< Check TP is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        MDF_LOGD("eFuse Two Point: Supported");
    } else {
        MDF_LOGD("eFuse Two Point: NOT supported");
    }

    /**< Check Vref is burned into eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        MDF_LOGD("eFuse Vref: Supported");
    } else {
        MDF_LOGD("eFuse Vref: NOT supported");
    }

    ret = adc1_config_width(ADC_WIDTH_BIT_12);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Configure ADC1 capture width, meanwhile enable output invert for ADC1");

    ret = adc1_config_channel_atten(BUTTON_BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Set the attenuation of a particular channel on ADC1");

    ret = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
                                   BUTTON_BATTERY_ADC_V_REF, &g_adc_cal_characteristics);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Characterize an ADC at a particular attenuation");

    if (button_usb_is_connected()) {
        TimerHandle_t timer = xTimerCreate("button_is_charging", 500 / portTICK_RATE_MS,
                                           true, NULL, button_is_charging_timercb);
        xTimerStart(timer, 0);
    }

    return MDF_OK;
}

button_key_status_t button_key_get_status(uint8_t key_index)
{
    button_key_status_t status = g_button_key[key_index].status;
    return status;
}

void button_key_reset_status()
{
    for (int i = 0; i < 4; ++i) {
        if (g_button_key[i].status != BUTTON_KEY_LONG_PRESS_PUSH) {
            g_button_key[i].status = BUTTON_KEY_NONE;
        }
    }
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t key_index = (uint32_t) arg;
    xQueueSendFromISR(g_event_queue, &key_index, NULL);
}

static void button_get_value_task(void *arg)
{
    uint32_t key_index = 0;

    for (int i = 0; i < BUTTON_KEY_NUM; ++i) {
        MDF_LOGD("gpio_num: %d, level: %d", g_button_key[i].gpio_num, gpio_get_level(g_button_key[i].gpio_num));

        if (gpio_get_level(g_button_key[i].gpio_num)) {
            g_button_key[i].backup_tickcount -= 500 * portTICK_RATE_MS;
            g_button_key[i].push   = true;
            g_button_key[i].status = BUTTON_KEY_NONE;
            mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_PUSH, (void *)i);
            MDF_LOGD("push, gpio_num: %d", g_button_key[i].gpio_num);
        } else if (g_button_key[i].push) {
            g_button_key[i].status = BUTTON_KEY_SHORT_PRESS;
            g_button_key[i].push   = false;
            mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_SHORT_PRESS, (void *)i);
            MDF_LOGD("short_press, gpio_num: %d", g_button_key[i].gpio_num);
        }
    }

    for (;;) {
        key_index                = -1;
        button_key_t *button_key = NULL;

        if (!xQueueReceive(g_event_queue, &key_index, 1000 / portTICK_RATE_MS)) {
            for (int i = 0; i < 4; ++i) {
                button_key = g_button_key + i;
                uint8_t execution_time_ms = 10;
                uint32_t spent_timer_ms   = (xTaskGetTickCount() - button_key->backup_tickcount) * portTICK_RATE_MS;

                if (spent_timer_ms > MDF_EVENT_BUTTON_KEY_LONG_PRESS_MS - execution_time_ms
                        && button_key->push == true) {
                    button_key->push = false;

                    if (key_index == -1) {
                        key_index = i;
                        button_key->status = BUTTON_KEY_LONG_PRESS_PUSH;
                        MDF_LOGD("key_index: %d", key_index);
                    } else {
                        button_key->status = BUTTON_KEY_NONE;
                        mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_EXCEPTION, (void *)key_index);
                        MDF_LOGD("Exception, gpio_num: %d", button_key->gpio_num);
                        key_index = -1;
                        break;
                    }
                }
            }

            if (key_index != -1) {
                mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_LONG_PRESS_PUSH, (void *)key_index);
                MDF_LOGD("long_press_push, gpio_num: %d, status: %d", key_index, button_key->status);
            }

            continue;
        }

        button_key = g_button_key + key_index;
        int key_level = gpio_get_level(button_key->gpio_num);
        vTaskDelay(50 / portTICK_RATE_MS);

        if (key_level != gpio_get_level(button_key->gpio_num)) {
            continue;
        }

        MDF_LOGD("key_index: %d, gpio_num: %d, status: %d, key_level: %d",
                 key_index, g_button_key[key_index].gpio_num, button_key->status, key_level);

        if (key_level == 1) {
            button_key->push             = true;
            button_key->backup_tickcount = xTaskGetTickCount();
            mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_PUSH, (void *)key_index);
            g_button_key[key_index].status = BUTTON_KEY_NONE;
        } else if (button_key->push) {
            button_key->release = true;
        }

        if ((button_key->push && button_key->release)
                || (button_key->status == BUTTON_KEY_LONG_PRESS_PUSH)) {
            uint32_t spent_timer_ms = (xTaskGetTickCount() - button_key->backup_tickcount) * portTICK_RATE_MS;
            button_key->push    = false;
            button_key->release = false;

            if (spent_timer_ms < MDF_EVENT_BUTTON_KEY_LONG_PRESS_MS) {
                button_key->status = BUTTON_KEY_SHORT_PRESS;
                mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_SHORT_PRESS, (void *)key_index);
                MDF_LOGD("short_press, gpio_num: %d", button_key->gpio_num);

            } else {
                button_key->status = BUTTON_KEY_LONG_PRESS_RELEASE;
                mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_LONG_PRESS_RELEASE, (void *)key_index);
                MDF_LOGD("long_press, gpio_num: %d", button_key->gpio_num);
            }

            mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_RELEASE, (void *)key_index);
        }
    }

    vTaskDelete(NULL);
}

mdf_err_t button_driver_init()
{
    mdf_err_t ret = MDF_OK;

    /**
     * @brief must set BUTTON_GPIO_POWER_SWITCH to high electrical level first, because
     *        the gpio_config may output one low electrical level pulse to the BUTTON_GPIO_POWER_SWITCH
     */
    gpio_set_level(BUTTON_GPIO_POWER_SWITCH, true);

    gpio_config_t power_config = {
        .pin_bit_mask = BIT64(BUTTON_GPIO_POWER_SWITCH),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    gpio_config(&power_config);

    gpio_set_level(BUTTON_GPIO_POWER_SWITCH, true);

    /**
     * @brief led
     */
    for (int i = 0; i < BUTTON_LED_NUM; ++i) {
        g_button_led_handle = iot_light_create(LEDC_TIMER_0, LEDC_HIGH_SPEED_MODE, 2000, 3, BUTTON_LED_TIMER_BIT);
        iot_light_channel_regist(g_button_led_handle, CHANNEL_ID_RED, BUTTON_GPIO_LED_RED, LEDC_CHANNEL_0);
        iot_light_channel_regist(g_button_led_handle, CHANNEL_ID_GREEN, BUTTON_GPIO_LED_GREEN, LEDC_CHANNEL_1);
        iot_light_channel_regist(g_button_led_handle, CHANNEL_ID_BLUE, BUTTON_GPIO_LED_BLUE, LEDC_CHANNEL_2);
    }

    /**
     * @brief key
     */
    memcpy(g_button_key, RTC_SLOW_MEM, sizeof(button_key_t) * BUTTON_KEY_NUM);

    if (g_button_key[0].gpio_num != BUTTON_GPIO_KEY0) {
        memset(g_button_key, 0, sizeof(button_key_t) * BUTTON_KEY_NUM);
        g_button_key[0].gpio_num = BUTTON_GPIO_KEY0;
        g_button_key[1].gpio_num = BUTTON_GPIO_KEY1;
        g_button_key[2].gpio_num = BUTTON_GPIO_KEY2;
        g_button_key[3].gpio_num = BUTTON_GPIO_KEY3;
    }

    gpio_config_t key_config = {
        .intr_type    = GPIO_INTR_ANYEDGE,
        .pin_bit_mask = BIT64(g_button_key[0].gpio_num) | BIT64(g_button_key[1].gpio_num)
        | BIT64(g_button_key[2].gpio_num) | BIT64(g_button_key[3].gpio_num),
        .mode         = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };

    ret = gpio_config(&key_config);
    MDF_ERROR_CHECK(ret < 0, ret, "gpio_config");

    /**< Create a queue to handle gpio event from isr */
    g_event_queue = xQueueCreate(3, sizeof(uint32_t));

    /**< Install gpio isr service */
    MDF_ERROR_ASSERT(gpio_install_isr_service(false));

    /**< Hook isr handler for specific gpio pin */
    for (int i = 0; i < BUTTON_KEY_NUM; ++i) {
        MDF_ERROR_ASSERT(gpio_isr_handler_add(g_button_key[i].gpio_num, gpio_isr_handler, (void *)i));
    }

    xTaskCreate(button_get_value_task, "button_get_value", 4096, NULL, 15, NULL);

    button_battery_init();

    MDF_LOGI("Power hold");

    return MDF_OK;
}

mdf_err_t button_driver_deinit()
{
    MDF_LOGI("Power down");
    gpio_set_level(BUTTON_GPIO_POWER_SWITCH, false);

    return MDF_OK;
}


#if 0
/**
 * @brief
 *       esp-idf/components/bootloader/subproject/main/bootloader_start.c:call_start_cpu0()>Hardware initialization
 */
#include "soc/gpio_periph.h"

void button_bootloader_checkout()
{
#define RTC_SLOW_MEM ((uint32_t*) (0x50000000))       /*!< RTC slow memory, 8k size */
#define BUTTON_GPIO_LED_GREEN (26)

#define BUTTON_GPIO_KEY0      (39)
#define BUTTON_GPIO_KEY1      (34)
#define BUTTON_GPIO_KEY2      (32)
#define BUTTON_GPIO_KEY3      (35)
#define BUTTON_KEY_NUM        (4)

    typedef struct {
        uint32_t gpio_num;
        uint32_t status;
        bool push;
        bool release;
        int backup_tickcount;
    } button_key_t;

    bool key_press_flag = false;
    button_key_t button_key[BUTTON_KEY_NUM] = {
        {.gpio_num = BUTTON_GPIO_KEY0},
        {.gpio_num = BUTTON_GPIO_KEY1},
        {.gpio_num = BUTTON_GPIO_KEY2},
        {.gpio_num = BUTTON_GPIO_KEY3},
    };

    for (int i = 0; i < BUTTON_KEY_NUM; ++i) {
        gpio_pad_select_gpio(button_key[i].gpio_num);
        PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[button_key[i].gpio_num]);
    }

    uint32_t tm_start = esp_log_early_timestamp();

    do {
        for (int i = 0; !key_press_flag && i < BUTTON_KEY_NUM; ++i) {
            if (GPIO_INPUT_GET(button_key[i].gpio_num)) {
                ets_delay_us(10 * 1000);
            }

            button_key[i].push = GPIO_INPUT_GET(button_key[i].gpio_num);
            key_press_flag |= button_key[i].push;
            ESP_LOGI(TAG, "gpio_num: %d, level: %d", button_key[i].gpio_num, button_key[i].push);
        }
    } while (!key_press_flag && 30 > (esp_log_early_timestamp() - tm_start));

    if (key_press_flag) {
        gpio_pad_select_gpio(BUTTON_GPIO_LED_GREEN);
        GPIO_OUTPUT_SET(BUTTON_GPIO_LED_GREEN, 1);
        ESP_LOGI(TAG, "There is a button pressed");
    }

    memcpy(RTC_SLOW_MEM, button_key, sizeof(button_key_t) * BUTTON_KEY_NUM);
}
#endif
