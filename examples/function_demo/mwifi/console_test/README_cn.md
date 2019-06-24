[[EN]](./README.md)

# ESP-MESH 控制台调试示例

## 概述

本示例将介绍如何使用 `Mdebug` 模块，在 mesh 产品中进行 ESP-MESH 基本调试。有以下功能：

 - [ESP-MESH 吞吐量测试](#esp-mesh-吞吐量测试介绍): 设置工作的模式（Client/Server），ping 功能
 - [ESP-MESH 网络配置](#mesh-config-命令): 配置 ESP-MESH 信息（路由器 SSID 、密码和 BSSID，工作信道，MESH ID 和密码，设备类型，最大连接数量，最大层数），打印/保存 ESP-MESH 配置信息，
 - [ESP-MESH 状态查询](#mesh_status-命令): 开始/停止 ESP-MESH，打印 ESP-MESH 设备状态
 - [Wi-Fi 扫描](#mesh_scan-命令): 扫描环境中的 AP 或 ESP-MESH 设备，设置过滤条件：RSSI、SSID、BSSID，设置在每个信道被动扫描的时间
 - [coredump 信息管理](#coredump-命令): 打印/擦除 coredump 信息，获取 coredump 数据长度，将 coredump 数据发送到指定设备，重传指定序号的 coredump 数据
 - [log 设置](#日志命令): 添加/移除监听设备，设置 log 传输级别，将 log 发送到指定设备
 - [一般命令](#一般命令): help（打印当前支持的所有命令）、version（获取 SDK 的版本）、heap（获取当前设备剩余内存）、restart（重启设备）、reset（重置设备并重启）

## 硬件准备

1. ESP32 开发板
2. 一台支持 2.4G 路由器

## 工作流程

1. 编译、烧录此工程到一个 ESP32 开发板
2. 打开串口终端，重启开发板，可在终端中看到以下信息

> 使用 `make monitor` 会出现意想不到的问题，建议使用 `minicom` 等串口终端。

<div align=center>
<img src="terminal_log.png"  width="800">
<p> TCP 服务器 </p>
</div>

3. 之后可以按照提示输入命令进行 ESP-MESH 调试

* 下面以操作流程为序，介绍每条命令的使用。

### 串口命令格式说明

* ESP-NOW debug 接收板支持的串口指令包括: help,sdcard,wifi_sniffer,wifi_config,wifi_scan,log,coredump,command。

* 串口命令的交互遵循以下规则：
    1. 控制命令通过串口，从 PC 端发送给 ESP-NOW debug 接收板，串口通信波特率为 115200
    2. 控制命令定义中，字符均为小写字母(部分选项为大写字母), 字符串不需要带引号
    3. 命令描述中括号 {} 包含的元素整体, 表示一个参数, 需要根据实际情况进行替换
    4. 命令描述中方括号 [] 包含的部分，表示为缺省值, 可以填写或者可能显示
    5. 串口命令的模式如下所示，每个元素之间，以空格符分隔

        ```
        命令＋选项＋参数，例如： log -s aa:bb:cc:dd:ee:ff
        ```

    6. 换行符支持 '\n' 或者 '\r\n'。
    7. 串口以 115200 波特率返回执行结果

### ESP-MESH 吞吐量测试介绍

1. 硬件准备
    * 两块 ESP32 开发板
    * 烧录 console_test 工程到两块开发板上
2. 操作流程
    1. 在两块开发板上配置 ESP-MESH，将两块开发板配置到同一 ESP-MESH 网络下。
        例如： 在两块开发板终端中输入相同的命令： `mesh_config -i 14:12:12:12:12:12 -s espressif -p espressif`
    2. 等待网络构建完成，输入以下命令查看查看状态
        `mesh_status -o`
    3. 配置其中一块开发板为 iperf 服务器模式
        `mesh_status -s`
    4. 配置另外一块开发板为 iperf 客户端模式
        `mesh_iperf -c 30:ae:a4:80:16:3c`

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

    |||||
    |-|-|-|-|
    |命令定义|log [\<tag\>] [\<level\>] [-s <addr (xx:xx:xx:xx:xx:xx)>]||
    |指令|log -s|将日志发送到指定设备|
    |参数|addr|监视设备 MAC 地址|
    ||tag|使用 tag 过滤日志|
    ||level|使用 level 过滤日志|
    |示例|log * NONE|设置所有的日志不输出|
    ||log mwifi INFO|设置 TAG 为 mwifi 的日志输出等级为 INFO|
    ||log -s 30:ae:a4:80:16:3c|将日志发送到 30:ae:a4:80:16:3c 设备|

### coredump 命令

1. coredump 数据

    |||||
    |-|-|-|-|
    |命令定义|coredump [-loe] [-q <seq>] [-s <addr (xx:xx:xx:xx:xx:xx)>]||
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

    |||||
    |-|-|-|-|
    |命令定义|mesh_config  [-oS] [-s <ssid>] [-p <password>] [-b <bssid (xx:xx:xx:xx:xx:xx)>] [-c <channel (1 ~ 13)>] [-i <mesh_id (6 Bytes)>] [-t <mesh_type ('idle'or 'root' or 'node' or 'leaf')>] [-P <mesh_password>] [-n <max_connection (1 ~ 10)>] [-l <max_layer (1 ~ 32)>]||
    |指令|mesh_config -s <ssid> -p <password> -b <bssid>|配置设备连接的路由器信息|
    ||mesh_config -c|配置设备 ESP-MESH 工作信道|
    ||mesh_config -i <mesh_id> -t <mesh_type> -P <mesh_password> -n <max_connection> -l <max_layer>|配置 ESP-MESH(ID、密码、容量)|
    ||mesh_config -o|打印 ESP-MESH 配置信息|
    ||mesh_config -S|保存 ESP-MESH 配置信息|
    |示例|mesh_config -c 11|配置 ESP-MESH 工作信道为 11 信道|
    ||mesh_config -i 14:12:12:12:12:12 -s espressif -p espressif|配置 ESP-MESH ID 为 14:12:12:12:12:12，连接路由器信息： SSID 为 espressif， 密码为 espressif|
    ||mesh_config -o|打印 ESP-MESH 配置信息|
    ||mesh_config -S|保存 ESP-MESH 配置信息|

### mesh_status 命令

1. mesh status

    |||||
    |-|-|-|-|
    |命令定义|mesh_status [-spo]||
    |指令|mesh_status -s|开始 ESP-MESH|
    ||mesh_status -p|停止 ESP-MESH|
    ||mesh_status -o|打印 ESP-MESH 状态信息|
    |示例|mesh_status -o|打印 ESP-MESH 状态信息|
    ||mesh_status -s|开始 ESP-MESH|
    ||mesh_status -p|停止 ESP-MESH|

### mesh_scan 命令

1. mesh scan

    |||||
    |-|-|-|-|
    |命令定义|mesh_scan [-r <rssi (-120 ~ 0)>] [-s <ssid (xx:xx:xx:xx:xx:xx)>] [-b <bssid (xx:xx:xx:xx:xx:xx)>] [-t <type ('router' or 'mesh')>] [-p <time>]||
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

    |||||
    |-|-|-|-|
    |命令定义|mesh_iperf [-spa] [-c <host (xx:xx:xx:xx:xx:xx)>] [-i <interval (sec)>] [-l <len (Bytes)>] [-t <time (sec)>]||
    |指令|mesh_iperf -c|将该设备运行为 client 模式|
    ||mesh_iperf -s|将该设备运行为 server 模式|
    ||mesh_iperf -p|ping|
    ||mesh_iperf -i|设置带宽计算周期|
    ||mesh_iperf -l|设置 buffer 长度|
    ||mesh_iperf -t|设置 iperf 测试时间|
    ||mesh_iperf -a|停止 iperf 测试|
    |示例|mesh_iperf -s |将该设备运行为 server 模式|
    ||mesh_iperf -c 30:ae:a4:80:16:3c|将该设备运行为 client 模式，并尝试与 30:ae:a4:80:16:3c 服务器进行性能测试|
