[[EN]](./README.md)

# ESP32-MeshKit-Light

## 概述

ESP32-MeshKit-Light 是基于 [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html) 应⽤的智能电灯，包含配网、升级、本地控制和设备联动等功能，它将帮助您更好地了解 ESP-MESH 的相关特性，并对 ESP32-MeshKit-Light 的程序进⾏⼆次开发。在运行本示例之前，请先详细阅读 [ESP32-MeshKit 指南](../README_cn.md)。

> 注：本示例不限于 ESP32-MeshKit-Light，您可以直接使用 ESP32 模块外接 LED 灯使用。

## 硬件说明

[ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_cn.pdf) 支持 5 路 PWM IO 口，支持色温（CW）灯和彩色（RGB）灯调节，色温灯的输出功率为 9 W，彩色灯的输出功率为 3.5 W。

1. 内部模组视图和引脚排序

<div align=center>
<img src="../docs/_static/light_module.png"  width="400">
<p> 内部模组视图 </p>
</div>

2. 引脚定义

|模块脚号 | 符号 | 状态 | 描述 | 
|:---:|:---:|:---:|:---|
|  1, 7 | GND | P| 接地脚|
| 2| CHIP_PU| I| 芯片使能（高电平有效），模块内部有上拉，为外部使能备用|
| 3 | GIPO32| I/O|RTC 32K_XP（32.768 kHz 晶振输入），功能扩展备用脚 | 
| 4 | GPIO33| I/O|RTC 32K_XN（32.768 kHz 晶振输入），功能扩展备用脚 |
| 5|GIPO0| I/O|IC 内部上拉，功能扩展备用脚 |
| 6| VDD3.3V|P | 模块供电脚，3V3|
| 8 | GPIO4|O | PWM_R 输出控制脚|
| 9 | GPIO16|O|PWM_G 输出控制脚，备用 UART 接口 (URXD)|
| 10 | GPIO5|O|PWM_B 输出控制脚，备用 UART 接口 (UTXD)|
| 11 | GPIO23|O |PWM_BR 输出控制脚 |
| 12 | GPIO19|O |PWM_CT 输出控制脚 |
| 13 | GPIO22| I/O|PWM 共用脚，功能扩展备用脚 |
| 14 | GPIO15| I|IC 内部上拉，功能扩展备用脚 |
| 15 | GPIO2| O|IC 内部下拉，功能扩展备用脚 |
| 16 | UORXD| I/O|UART 调试和软件烧录接口接收脚 |
| 17 | UOTXD| I/O|UART 调试和软件烧录接口发送脚|
| 19 | ANT| I/O| 外置天线输出脚 |
| 18, 20| GND | p| RF 接地|


## 灯的状态

1. 等待配网：黄色闪烁
2. 验证配网信息：橙色闪烁
3. 配网成功：绿色闪烁
4. 开始升级：淡蓝色闪烁三秒
5. 升级成功等待重启：蓝色闪烁
6. 异常重启：红色闪烁

> 注：连续上电三次（关 > 开 > 关 > 开 > 关 > 开）重置设备
