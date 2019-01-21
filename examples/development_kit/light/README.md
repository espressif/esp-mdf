[[中文]](./README_cn.md)

# ESP32-MeshKit-Light

## Overview

ESP32-MeshKit-Light is a smart lighting solution based on [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html). It features network configuration, upgrade, local control, device association, etc., and will facilitate your understanding of ESP-MESH to implement further development. Before running this example, please refer to [ESP32-MeshKit Guide](../README.md).

> Note: This example is not limited to ESP32-MeshKit-Light, and can also be applied to an ESP32 module connected to an external LED.


## Hardware

[ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf) supports 5 types of PWM IO interfaces. Its color temperature (CW) and color hue (RGB) can be adjusted, with an output power of 9 W and 3.5 W respectively.

1. Inside View and Pin Layout

<div align=center>
<img src="../docs/_static/light_module.png"  width="400">
<p> Inside View </p>
</div>

2. Pin Definition

|No. | Name | Type | Description |
| :----: | :----: | :----: |:---- |
|1, 7| GND| P |   Ground   |
|2 | CHIP_PU| I | Chip enabling (High: On); module internal pull-up; alternative for external enabling |
|3 | GPIO32 | I/O | RTC 32K_XP (32.768 kHz crystal oscillator input); alternative for function expansion|
|4 | GPIO33 | I/O | RTC 32K_XN (32.768 kHz crystal oscillator output); alternative for function expansion|
|5 | GPIO0 | I/O| IC internal pull-up; alternative for function expansion |
|6 | VDD3.3V | P | Power supply, 3V3   |
|8 | GPIO4 | O | PWM_R output control |
|9 | GPIO16 | O | PWM_G output control; alternate UART interface (URXD) |
|10 | GPIO5 | O | PWM_B output control; alternate UART interface (UTXD) |
|11 | GPIO23 | O | PWM_BR output control |
|12 | GPIO19 | O | PWM_CT output control |
|13 | GPIO22 | I/O | Shared by PWM; alternative for function expansion |
|14 | GPIO15 | I | IC internal pull-up; alternative for function expansion |
|15 | GIPO2 | O | IC internal pull-down; alternative for function expansion |
|16 | UORXD | I/O | UART interface (RXD) for debugging and software downloading |
|17 | UOTXD | I/O | UART interface (TXD) for debugging and software downloading |
|19 | ANT | I/O | External antenna output |
|18, 20 | GND | P | RF ground |


## Light Color Change in Different Statuses

1. Wait be to networked: flashes yellow

2. Verify network configuration information: flashes orange

3. Networked successfully: flashes green

4. Start to upgrade: flashes light blue for 3 seconds

5. Upgrade successfully, ready to reboot: flashes blue

6. Abnormal reboot: flashes red

> Note: The device can be reset by powering it off/on for three consecutive times. 