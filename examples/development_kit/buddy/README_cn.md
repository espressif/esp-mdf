[[EN]](./README.md)

# ESP32-Buddy 用户指南

## 概述

ESP32-Buddy 是针对 [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) 相关测试所设计的，它将帮助您更便于测试 ESP-MESH 的相关特性，并对 ESP32-Buddy 的程序进⾏⼆次开发。在运行本示例之前，请先详细阅读 [ESP32-MeshKit 指南](../README_cn.md)。

> 注：本示例不限于 ESP32-Buddy，您可以直接使用 ESP32 模块外接相关模块使用。

## 硬件说明

[ESP32-Buddy](https://www.espressif.com/sites/default/files/documentation/esp32-Buddy_user_guide_cn.pdf) 外接有 0.97 寸 OLED 显示屏、WS2812B 灯和 HTS221 温湿度传感器，以及 HMC8073 射频衰减器。

1. ESP32-Buddy 模块框图
<div align=center>
<img src="../docs/_static/esp_buddy_module.png"  width="400">
<p> ESP32-Buddy 模块框图 </p>
</div>

2. ESP32-Buddy 实物图
<div align=center>
<img src="../docs/_static/esp_buddy_physical_map_front.png"  width="400">
<p> ESP32-Buddy 实物图正面 </p>
</div>
<div align=center>
<img src="../docs/_static/esp_buddy_physical_map_back.png"  width="400">
<p> ESP32-Buddy 实物图背面 </p>
</div>

3. IO 映射关系

| IO | 描述 | 
|:---:|:---|
| IO0 | 按键 |
| IO32 | 按键 |
| IO25 | WS2812B 指示灯 |
| IO18 | I2C SDA |
| IO23 | I2C SCL |
| IO19 | HMC8073 LE(Latch Enable) |
| IO21 | HMC8073 CLK |
| IO22 | HMC8073 SI(Data Input) |

## 指示灯状态

| 指示灯颜色 | 状态 |
| ----------- | ------ |
| 红色 | 当前处于空闲状态，等待组网 |
| 蓝色 | 该节点为根节点 |
| 绿色 | 组网成功 |

## 工程介绍

### 简介

ESP-MESH 是一种基于 Wi-Fi 协议构建的网络协议。ESP-MESH 允许在大的物理区域（室内和室外）上分布的多个设备（以下称为节点）在单个 WLAN（无线局域网）下互连。ESP-MESH 具有自组织和自我修复功能，意味着网络可以自主构建和维护。

通过 ESP-Buddy 工程，目前可方便的进行以下测试：

 - 吞吐量测试
 - 距离测试
 - 组网测试（根据应用场景，可测试不同节点做为根节点的组网时间）

### 吞吐量测试流程

1. 编译、烧录此工程到两个 ESP32-Buddy 开发板
2. 选择其中一个长按 IO32 按钮，该设备将做为根节点，LED 将由红色变为蓝色
3. 等待另一块板子连接上父节点，LED 将由红色变为绿色
4. 配置根节点进入吞吐量测试模式：通过 IO0 按钮，将 OLED 界面换到吞吐量测试界面，并按 IO32 按钮配置为 iPerf Server
5. 配置子节点进入吞吐量测试模式：通过 IO0 按钮，将 OLED 界面换到吞吐量测试界面，并按 IO32 按钮配置为 iPerf Client，开始测试
6. 吞吐量测试结果将显示在 OLED 屏上。

### 组网测试流程

1. 编译、烧录此工程到两个 ESP32-Buddy 开发板
2. 选择其中一个长按 IO32 按钮，该设备将做为根节点，LED 将由红色变为蓝色
3. 等待其余板子连接上父节点，LED 将由红色变为绿色
6. 组网测试结果将显示在 OLED 屏上，在首页中将显示组网耗时（上电开始计时）。

* 下面将介绍每条命令的使用。

## 控制台命令

### 串口命令格式说明

* ESP-NOW debug 接收板支持的串口指令包括：help、sdcard、wifi_sniffer、wifi_config、wifi_scan、log、coredump、attenuator 和 command。

* 串口命令的交互遵循以下规则：
    1. 控制命令通过串口，从 PC 端发送给 ESP-NOW debug 接收板，串口通信波特率为 115200
    2. 控制命令定义中，字符均为小写字母（部分选项为大写字母），字符串不需要带引号
    3. 命令描述中方括号 [] 包含的部分，表示为缺省值，可以填写或者可能显示
    4. 串口命令的模式如下所示，每个元素之间，以空格符分隔

        ```
        命令＋选项＋参数，例如： log -s aa:bb:cc:dd:ee:ff
        ```

    5. 换行符支持 '\n' 或者 '\r\n'。
    6. 串口以 115200 波特率返回执行结果

### 一般命令

 - help
 	打印已注册命令及其说明
 - version
 	获取芯片和 SDK 版本
 - heap
 	获取当前可用堆内存大小
 - restart
 	软重启芯片
 - reset
 	清除设备所有配置信息
 
### 日志命令

1. 日志配置

    |命令定义|log [\<tag\>] [\<level\>] [-s <addr (xx:xx:xx:xx:xx:xx)>]||
    |:---:|:---|:---|
    |指令|log -s|将日志发送到指定设备|
    |参数|addr|监视设备 MAC 地址|
    ||tag|使用 tag 过滤日志|
    ||level|使用 level 过滤日志|
    |示例|log * NONE|设置所有的日志不输出|
    ||log mwifi INFO|设置 TAG 为 mwifi 的日志输出等级为 INFO|
    ||log -s 30:ae:a4:80:16:3c|将日志发送到 30:ae:a4:80:16:3c 设备|

### coredump 命令

1. coredump 数据

    |命令定义|coredump [-loe] [-q <seq>] [-s <addr (xx:xx:xx:xx:xx:xx)>]||
    |:---:|:---|:---|
    |指令|coredump -l|获取该设备上的 coredump 数据长度|
    ||coredump -o|读取该设备上的 coredump 数据并打印到控制台|
    ||coredump -e|擦除该设备上的 coredump 数据|
    ||coredump -s|发送设备上的 coredump 数据到指定设备|
    |参数|addr|监视设备 MAC 地址|
    ||sequence|coredump 数据的序号|
    |示例|coredump -s 30:ae:a4:80:16:3c|将 coredump 数据发送到 30:ae:a4:80:16:3c 设备|
    ||coredump -q 110 -s 30:ae:a4:80:16:3c|将序号 110 开始的 coredump 数据发送到 30:ae:a4:80:16:3c 设备|
    ||coredump -l|获取该设备上的 coredump 数据长度|
    ||coredump -o|读取该设备上的 coredump 数据并打印到控制台|
    ||coredump -e|擦除该设备上的 coredump 数据|

### mesh config 命令

1. mesh config

    |命令定义|mesh_config  [-oS] [-s <ssid>] [-p <password>] [-b <bssid (xx:xx:xx:xx:xx:xx)>] [-c <channel (1 ~ 13)>] [-i <mesh_id (6 Bytes)>] [-t <mesh_type ('idle'or 'root' or 'node' or 'leaf')>] [-P <mesh_password>] [-n <max_connection (1 ~ 10)>] [-l <max_layer (1 ~ 32)>]||
    |:---:|:---|:---|
    |指令|mesh_config -s <ssid> -p <password> -b <bssid>|配置设备连接的路由器信息|
    ||mesh_config -c|配置设备 ESP-MESH 工作信道|
    ||mesh_config -i <mesh_id> -t <mesh_type> -P <mesh_password> -n <max_connection> -l <max_layer>|配置 ESP-MESH(ID、密码、容量)|
    ||mesh_config -o|打印 ESP-MESH 配置信息|
    ||mesh_config -S|保存 ESP-MESH 配置信息|
    |示例|mesh_config -c 11|配置 ESP-MESH 工作信道为 11 信道|
    ||mesh_config -i 14:12:12:12:12:12 -s espressif -p espressif|配置 ESP-MESH ID 为 14:12:12:12:12:12，连接路由器信息：SSID 为 espressif， 密码为 espressif|
    ||mesh_config -o|打印 ESP-MESH 配置信息|
    ||mesh_config -S|保存 ESP-MESH 配置信息|

### mesh_status 命令

1. mesh status

    |命令定义|mesh_status [-spo]||
    |:---:|:---|:---|
    |指令|mesh_status -s|开始 ESP-MESH|
    ||mesh_status -p|停止 ESP-MESH|
    ||mesh_status -o|打印 ESP-MESH 状态信息|
    |示例|mesh_status -o|打印 ESP-MESH 状态信息|
    ||mesh_status -s|开始 ESP-MESH|
    ||mesh_status -p|停止 ESP-MESH|

### mesh_scan 命令

1. mesh scan

    |命令定义|mesh_scan [-r <rssi (-120 ~ 0)>] [-s <ssid (xx:xx:xx:xx:xx:xx)>] [-b <bssid (xx:xx:xx:xx:xx:xx)>] [-t <type ('router' or 'mesh')>] [-p <time>]||
    |:---:|:---|:---|
    |指令|mesh_scan -r|设置 RSSI 过滤值|
    ||mesh_scan -s|设置 SSID 过滤字符串|
    ||mesh_scan -b|设置 BSSID 过滤|
    ||mesh_scan -t|设置过滤类型|
    ||mesh_scan -p|设置每个信道被动扫描时间|
    |示例|mesh_scan -t mesh|过滤 mesh 设备|
    ||wifi_scan -r -60 -t mesh|扫描 RSSI 信号值在 -60 以内的 mesh 设备|
    ||wifi_scan -s espressif -t mesh|扫描 SSID 为 espressif 下的 mesh 设备|
    ||wifi_scan -p 300|设置每个信道被动扫描时间为 300 ms|

### mesh_iperf 命令

1. mesh iperf

    |命令定义|mesh_iperf [-spa] [-c <host (xx:xx:xx:xx:xx:xx)>] [-i <interval (sec)>] [-l <len (Bytes)>] [-t <time (sec)>]||
    |:---:|:---|:---|
    |指令|mesh_iperf -c|将该设备运行为 client 模式|
    ||mesh_iperf -s|将该设备运行为 server 模式|
    ||mesh_iperf -p|ping|
    ||mesh_iperf -i|设置带宽计算周期|
    ||mesh_iperf -l|设置 buffer 长度|
    ||mesh_iperf -t|设置 iperf 测试时间|
    ||mesh_iperf -a|停止 iperf 测试|
    |示例|mesh_iperf -s |将该设备运行为 server 模式|
    ||mesh_iperf -c 30:ae:a4:80:16:3c|将该设备运行为 client 模式，并尝试与 30:ae:a4:80:16:3c 服务器进行性能测试|

### attenuator 命令

1. attenuator

    |命令定义|attenuator [-d]||
    |:---:|:---|:---|
    |指令|attenuator -d|设置衰减器值，范围<0,31>|
    |示例|mesh_iperf -d 30 |设置衰减器为 30 dB|
