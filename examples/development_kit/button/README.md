[[中文]](./README_cn.md)

# ESP32-MeshKit-Button

## Overview

ESP32-MeshKit-Button is a smart button solution based on [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html). The ESP-MeshKit solution features network configuration, upgrade, device association, etc.

ESP32-MeshKit-Button is designed to be used with [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf). Before configuring the device, please refer to [ESP32-MeshKit Guide](../README.md).

ESP32-MeshKit-Button will help you better understand how ESP-MESH can be used in systems with ultra-low power consumption. 

<div align=center>
<img src="docs/_static/en/module_diagram.png" width="800">
<p> Module Diagram </p>
</div>

## Hardware

ESP32-MeshKit-Button features all the necessary hardware components:

* **Microcontroller**: ESP32 SoC
* **Power supply interface**:
    * Mini USB
    * 2-pin connector for an external battery
* **Debugging interface**: For connecting ESP-Prog
* **Control component**: 4 buttons
* **Display component**: 4 RGB LEDs

<div align=center>
<img src="docs/_static/en/ESP32-MeshKit-Button_en.png" width="650">
</div>

To build an ESP-MESH network, you need the hardware given in the table below.

| Hardware | Quantity |
| -------- | -------- |
| [ESP32-MeshKit-Button](docs/ESP32-MeshKit-Button_Schematic.pdf) development board | 1 |
| [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf) | No less than 1 |
| [ESP-Prog](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP-Prog_guide_en.md) debugging tool (optional) | 1 |
| Power supply: 200-mAh lithium battery or Mini USB | 1 |

<div align=center>
<img src="docs/_static/zh_CN/interaction_diagram.png" width="600">
<p> Networking schematics </p>
</div>

## Network configuration

### 1. Preparation

As ESP32-MeshKit-Button cannot serve as a root node and can only work with ESP32-MeshKit-Light devices, it needs to be added to an already existing ESP-MESH network which includes at least one ESP32-MeshKit-Light device.

For more details on how to establish an ESP-MESH network, please refer to [ESP32-MeshKit Guide](../README.md).

<div align=center>
<img src="docs/_static/en/function_flow_chart.png"  width="850">
<p> Process </p>

</div>

### 2. Networking process

You can configure the network in either of the following two ways:

#### Proximity automatic configuration network

After the ESP32-MeshKit-Button is close to the ESP32-MeshKit-Light, press and hold any button for 5 seconds. The LED flashes green, indicating that the device has completed the distribution and setting the default associated event.

>Note: If you need to modify the default association event, please connect Mini USB and use APP to modify

#### APP Configuring the network

* Plug a Mini USB connector into your ESP32-MeshKit-Button. You can reset the device by long pressing any two buttons simultaneously for 5 seconds. LEDs will start slowly flashing yellow, signalling that the device has entered Network Configuration mode;

* App shows the prompt, tap `Add the device to the mesh network`;

* When the network configuration is completed, ESP32-MeshKit-Button appears on the list of added devices. Its LEDs turn to solid yellow, which means that the Button is waiting to be associated with ESP32-MeshKit-Light devices.

> Note:
> 1. ESP32-MeshKit-Button serves as a child node only, and must be used with ESP32-MeshKit;
> 2. ESP32-MeshKit-Button can only be added to an existing Mesh network.

<table>
    <tr>
        <td ><img src="docs/_static/en/discovery_device.png" width="300"><p align=center>Add to Mesh Network</p></td>
        <td ><img src="docs/_static/en/add_device.png" width="300"><p align=center>Network Successfully Configured</p></td>
    </tr>
</table>

### 3. Association

* After the device is networked, go to ESP-MESH App, open the list of added devices, and long press ESP32-MeshKit-Button.

* In the pop-up menu choose `Device association`, then choose the ESP32-MeshKit-Light and tap on `√` at the top right corner.

* Assign actions to the buttons by dragging action labels onto the available buttons. Then tap on `√` at the top right corner to complete the association. The LEDs turn blue.

<table>
    <tr>
        <td ><img src="docs/_static/en/device_association_en.png" width="300"><p align=center>Select Device Association</p></td>
        <td ><img src="docs/_static/en/associated_way.png" width="300"><p align=center>Select Association Way</p></td>
        <td ><img src="docs/_static/en/associated_device.png" width="300"><p align=center>Select Devices to be Associated</p></td>
        <td ><img src="docs/_static/en/association_add.png" width="300"><p align=center>Assign Functions to Buttons </p></td>
    </tr>
</table>

[Here](http://demo.iot.espressif.cn:8887/cmp/demo/ESP32-MeshKit-Button_Demo.mp4), you can also view a demo video of ESP32-MeshKit-Button.

### 4. USB Connection 

* If you plug a Mini USB into ESP32-MeshKit-Button and press a button, the device enters **ESP-MESH mode**. You can associate, upgrade and control the device, as well as implement its network configuration.

* If ESP32-MeshKit-Button is not plugged-in and uses the battery for power supply, the device enters **ESP-NOW mode**. In this case, the ESP32 chip keeps the power supply cut off and restarts only when a button is pressed. Once the chip finishes sending the control command, it immediately cuts off the power again.

### 5. LED Indicator

LED indicators visually represent the current status of ESP-MeshKit-Button.

| LED Color | USB Port | Status |
| --------- | -------- | ------ |
| Yellow (slow flashing) | Plugged-in | Not networked, in Network Configuration mode |
| Yellow (rapid flashing) | Plugged-in | Networking, being configured |
| Yellow (solid) | Plugged-in | Networked but not associated |
| Blue (flashing) | Plugged-in | Networked and associated, charging |
| Blue (solid) | Plugged-in | Networked and associated, charged |
| Red (flashing) | Plugged-in | Networked and associated, low battery |
| Off | Unplugged | Power Saving mode, buttons function normally |
| Green (button being pressed) | Both | Button pressed, device configured |
| Yellow (one blink) | Plugged-in | Button pressed, device not configured |
| Green then yellow | Unplugged | Button pressed, configured but not associated |
| Green then flashing yellow | Plugged-in | Button pressed, not configured and not associated |

## Actions for Buttons

The available short press actions are described in the table below.

| Function | Description | 
| -------- | ----------- |
| On/Off switch | Turns on and off the associated ESP32-MeshKit-Lights. | 
| Brightness adjustment | With every short-press the brightness increases incrementally. After reaching the highest level, it drops to a minimal value and starts increasing again. |
| Color temperature adjustment | With every short-press the color temperature increases incrementally. After reaching the highest level, it drops to a minimal value and starts increasing again. |
| Hue adjustment | With every short-press the hue changes, following the pre-set steps. After reaching the last step, the hue adjustment starts over. | 
| Bright mode | Brings the lights into Bright mode. They turn white. H/S/V: 0/0/100. |
| Read mode | The lights switch to an eye-friendly color. H/S/V: 216/10/100. |
| Cozy mode | The lights switch to a cozy light-yellow color. H/S/V: 60/10.2/100. |
| Bedtime mode | Good for taking a break in your drawing room or for bedroom. The lights turn warm-yellow. H/S/V: 28/63/100. |

For the available long press actions, see another table below.

| Function | Description | 
| -------- | ----------- |
| Brightness | Assigned to a pair of buttons, which you can long press to gradually increase/decrease brightness. |
| Color temperature | Assigned to a pair of buttons, which you can long press to gradually increase/decrease color temperature. |
| Hue | Assigned to a pair of buttons, which you can long press for smooth transition between pre-set hues back and forth. |

## Power Consumption and Time Characteristics

ESP32-MeshKit-Button has ultra-low power consumption. It can complete a packet sending process from a power-off status within about 1200 ms.  

The table below lists the current consumption and running duration of ESP32-MeshKit-Button in different scenarios. Note that the current is measured with its specific power-supply part DCVCC-SUFVCC.

| Scenario | Current | Duration |
|---|---|---|
| Power supply cut by the ESP32 chip | 0.1 uA | Depends on how long the button stays un-triggered when the power is supplied by a battery.|
| Start-up process after waking up | 40 mA | 800 ms |
| Initialization and packet sending control via ESP-NOW | 60 mA | 400 ms |

According to the table above, a single button-triggering event consumes 56 mAs (40 mA x 800 ms + 60 mA x 400 ms = 56 mAs). It is safe to say that a 200-mAh battery can provide up to 10,000 button control events (200 mAh x 3600 / 56 mAs ≈ 12800 - power consumption in the power-off status).

> **Note**: Time-saving features can also be found in the following aspects.
> 
> 1. **Data transmission**: ESP32-MeshKit-Button can be networked as a child node from start-up within around 500 ms, because its network configuration related data is transmitted via the ESP-NOW protocol. This is a connectionless Wi-Fi communication protocol based on the data link layer, which helps to reduce the device connection time.
> 2. **Wake-up button test**: A function related to a wake-up button test is added to `bootloader`, so you can test the device before the chip starts up.
> 3. **`bootloader` starting process**: Clear the `bootloader` log to reduce its starting time.

## Wake-up Button Test

If ESP32-MeshKit-Button is powered by an external battery, the startup process of its ESP32 chip right after waking up takes only 800 ms. Therefore, it is very unlikely to conduct the wake-up button trigger test after the chip is started. For this reason, a test related function is added to `bootloader`.

You need to add the test code to the file `esp-idf/components/bootloader/subproject/main/bootloader_start.c`, and call `button_bootloader_trigger()` after the hardware initialization with the function `bootloader_init()`  in `call_start_cpu0()` .

```c
#include "soc/gpio_periph.h"

void button_bootloader_trigger()
{
#define RTC_SLOW_MEM ((uint32_t*) (0x50000000))       /*!< RTC slow memory, 8k size */
#define BUTTON_GPIO_LED_BLUE (14)
#define BUTTON_GPIO_KEY0     (39)
#define BUTTON_GPIO_KEY1     (34)
#define BUTTON_GPIO_KEY2     (32)
#define BUTTON_GPIO_KEY3     (35)
#define BUTTON_KEY_NUM       (4)

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
        gpio_pad_select_gpio(BUTTON_GPIO_LED_BLUE);
        GPIO_OUTPUT_SET(BUTTON_GPIO_LED_BLUE, 1);
        ESP_LOGI(TAG, "There is a button pressed");
    }

    memcpy(RTC_SLOW_MEM, button_key, sizeof(button_key_t) * BUTTON_KEY_NUM);
}
```