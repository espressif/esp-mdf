[[EN]](./README.md)

# ESP-NOW debug 接收板示例

ESP-NOW debug 接收板需要和 ESP-MDF 设备处于同一个 Wi-Fi 信道上才可以接收 ESP-MDF 设备发送的调试数据。同时，如果 ESP-NOW debug 接收板和 ESP-MDF 设备处于同一个局域网（连接同一个路由器）下，接收板还可以通过 mDNS 设备发现服务获取 Mesh 网络的基本信息，包括：Mesh-ID、根节点 IP、根节点 MAC 地址等。

<div align=center>
<img src="espnow_debug.png" width="800">
</div>

> ESP-NOW debug 接收板可以不直接与路由器进行连接，只需与 ESP-MESH 网络在同一信道上即可。
> 若需要与接收其他 ESP-MESH 设备的 log，需要在监听的设备中添加以下代码：
> ```
>    MDF_ERROR_ASSERT(mdebug_console_init());
>    MDF_ERROR_ASSERT(mdebug_espnow_init());
>    mdebug_cmd_register_common();
> ```

ESP-NOW debug 接收板提供的主要功能包括：：

 - [sdcard 文件管理](#sd-卡文件操作命令): 例出当前 SD 卡当中所有的文件，删除指定文件，可选择以某种格式打印文件内容(hex, string, base64)
 - [sniffer 监听环境中的 IEEE 802.11 包](#sniffer-操作命令): 抓取数据包并以 pcap 格式保存在 SD 卡中，可指定文件名称，设置数据包过滤条件，并且可指定在某个信道上监听
 - [Wi-Fi 配置](#wi-fi-配置命令): 设置工作在 STA 模式下的 Wi-Fi 信息（路由器 SSID 、密码和 BSSID，工作信道），保存/擦除配置信息
 - [Wi-Fi 扫描](#扫描命令): 工作在 STA 模式下，扫描环境中的 AP 或 ESP-MESH 设备，设置过滤条件：RSSI、SSID、BSSID，设置在每个信道被动扫描的时间
 - [log 设置](#日志命令): 监听其他设备的 log 日志,统计各类日志（I、W、E）的数量以及重启次数和  Coredump 次数并显示在屏幕上，添加/移除监听设备，设置 log 传输级别
 - [coredump 信息管理](#coredump-命令): 向指定设备查询是否有 coredump 信息，接收/擦除 coredump 信息，请求接收指定序号的 coredump 信息
 - [command](#command-命令): 在指定设备上运行命令
 - [一般命令](#其他命令): help（打印当前支持的所有命令）

## ESP-NOW 介绍

### 概述

ESP-NOW debug 接收板通过 [ESP-NOW](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/api-reference/wifi/esp_now.html) 无线传输技术接收 ESP-MDF 设备的运行日志和 Coredump 数据。ESP-NOW 是由 Espressif 定义的一种无连接的 Wi-Fi 通信协议，广泛应用于智能照明，遥控，传感器等领域。在 ESP-NOW 中，数据被封装在 Wi-Fi Action 帧中进行数据的收发，在没有连接的情况下从一个 Wi-Fi 设备传输数据到另一个 Wi-Fi 设备。

### ESP-NOW 特性

1. 收发双方必须在 `同一个信道上`
2. 接收端在非加密通信的情况下可以不添加发送端的 MAC 地址（加密通信时需要添加），但是发送端必须要添加接收端的 MAC 地址
3. ESP-NOW 最多可添加 20 个配对设备，同时支持其中最多 6 个设备进行通信加密
4. 通过注册回调函数的方式接收数据包，以及检查发送情况（成功或失败）
5. 利用 CTR 和 CBC-MAC 协议（CCMP）保护数据的安全

> 更多关于 ESP-NOW 的原理介绍，请参考 Espressif 官方手册 [ESP-NOW 使用说明](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/api-reference/wifi/esp_now.html) 和 ESP-IDF 示例 [espnow](https://github.com/espressif/esp-idf/tree/master/examples/wifi/espnow)。

## ESP-NOW debug 示例使用说明

### 硬件说明

| 硬件 | 数量 | 备注 |
| :--- | :--- | :--- |
| [ESP-WROVER-KIT V2(带屏)](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/hw-reference/modules-and-boards-previous.html#esp-wrover-kit-v2) 开发板 | 1 | [使用说明](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/get-started/get-started-wrover-kit-v2.html) |
| TF 卡 | 1 | 保存 ESP-MDF 设备的运行日志和 coredump 数据，容量建议大于 1G |
| MiniUSB 数据线 | 1 | |

<div align=center>
<img src="esp-wrover-kit-v2.jpg" width="400">
<p> ESP-WROVER-KIT-V2 </p>
</div>

> 注: 
> 1. 如果使用非 ESP-WROVER-KIT 也能运行, 只是部分关于存储的功能无法使用，例如：保存抓包数据
> 2. 如是使用 ESP-WROVER-KIT V4.1， 并同时使用 SD-Card 和 LCD，请将电阻 R167、R168 取掉，避免屏幕无法正常使用

### 项目文件组织结构

```
wireless_debug/
├── build                        // All files generated through compiling
├── components
│   ├── i2c_bus
│   ├── lcd                     // lcd module for info display
│   ├── pcap
│   └── sdcard                  // sdcard module for data storage
├── main
│   ├── component.mk
│   ├── debug_cmd.c
│   ├── debug_recv.cpp
│   ├── include
│   │   ├── debug_cmd.h
│   │   ├── debug_recv.h
│   │   └── image.h
│   ├── Kconfig.projbuild       // Kconfig file of the example
│   ├── main.c                  // Main entry of project
│   └── wifi_sniffer_cmd.c
├── Makefile
├── partitions.csv              // partition table file
├── README_cn.md
├── sdkconfig
├── sdkconfig.defaults          // default configuration options
├── sdkconfig.old
└── terminal_log.png
```

### 工作流程

1. 编译、烧录此工程到一个 ESP32 开发板
2. 打开串口终端，重启开发板，可在终端中看到以下信息

> 使用 `make monitor` 会出现意想不到的问题，建议使用 `minicom` 等串口终端。

<div align=center>
<img src="terminal_log.png"  width="800">
<p> 串口终端 </p>
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
        命令＋选项＋参数，例如： log -a aa:bb:cc:dd:ee:ff
        ```

    6. 换行符支持 '\n' 或者 '\r\n'。
    7. 串口以 115200 波特率返回执行结果

### SD 卡文件操作命令

1. 列出文件

    |||||
    |-|-|-|-|
    |命令定义|sdcard -l <file_name>||
    |指令|sdcard -l|显示 SD 卡所有匹配的文件列表|
    |参数|file_name|匹配字符串|
    |||* // 显示所有文件<br>*.ab // 显示所有 ab 文件类型的文件 |
    |示例|sdcard -l *.pcap|显示 SD 卡中所有 pcap 文件|

2. 删除文件

    |||||
    |-|-|-|-|
    |命令定义|sdcard -r <file_name>||
    |指令|sdcard -r|删除指定文件或匹配的所有文件|
    |参数|file_name|文件名或匹配字符串|
    |||* // 删除所有文件<br>*.ab // 删除所有 ab 文件类型的文件<br> a.abc // 删除 a.abc 文件 |
    |示例|sdcard -r *.pcap|删除 SD 卡中所有 pcap 文件|

3. 打印文件内容

    |||||
    |-|-|-|-|
    |命令定义|sdcard -o <file_name> -t <type>||
    |指令|sdcard -o|打印指定文件内容|
    |参数|file_name|文件名|
    ||| a.abc // 打印 a.abc 文件 |
    ||type|文件打印类型|
    ||| hex // hex |
    ||| string // string |
    ||| base64 // base64 |
    |示例|sdcard -o a.abc|打印 a.abc 文件内容|

### sniffer 操作命令

1. 设置 sniffer 监听信道

    |||||
    |-|-|-|-|
    |命令定义|wifi_sniffer -c <channel (1 ~ 13)>||
    |指令|wifi_sniffer -c|设置 sniffer 监听信道|
    |参数|channel|信道号|
    ||| 11 // 监听 11 信道 |
    |示例|wifi_sniffer -c 11|sniffer 监听 11 信道|

2. 设置监听数据包存储文件名

    |||||
    |-|-|-|-|
    |命令定义|wifi_sniffer -f <file>||
    |指令|wifi_sniffer -f|设置数据包存储文件名|
    |参数|file|文件名|
    ||| sniffer.pcap // 数据保存在 sniffer.pcap 文件 |
    |示例|wifi_sniffer -f sniffer.pcap|sniffer 监听的数据存储在 sniffer.pcap 文件中|

3. 设置监听数据包过滤条件

    |||||
    |-|-|-|-|
    |命令定义|wifi_sniffer -F <mgmt\|data\|ctrl\|misc\|mpdu\|ampdu> ||
    |指令|wifi_sniffer -F|设置数据包过滤类型|
    |参数|mgmt\|data\|ctrl\|misc\|mpdu\|ampdu|过滤数据包类型|
    ||| ampdu // 过滤 ampdu 数据包 |
    |示例|wifi_sniffer -F ampdu|sniffer 监听过滤 ampdu 数据包|

4. 停止 sniffer 监听

    |||||
    |-|-|-|-|
    |命令定义|wifi_sniffer -s||
    |指令|wifi_sniffer -s|停止监听|
    |示例|wifi_sniffer -s|停止 sniffer 监听|

### Wi-Fi 配置命令

1. Wi-Fi 配置

    |||||
    |-|-|-|-|
    |命令定义|wifi_config -c <channel (1 ~ 13)> -s <ssid> -b <bssid (xx:xx:xx:xx:xx:xx)> -p <password>||
    |指令|wifi_config -c -s -b -p|Wi-Fi 配置|
    |参数|channel|Wi-Fi 工作信道|
    ||| 11 // 监听 11 信道 |
    ||ssid|AP 的 SSID|
    ||bssid|AP 的 BSSID|
    ||password|AP 的 密码|
    |示例|wifi_config -s "esp-liyin" -p "password"|Wi-Fi 配置，连接 SSID 为 esp-liyin 密码为 password 的 AP|

2. 保存/擦除 Wi-Fi 配置信息

    |||||
    |-|-|-|-|
    |命令定义|wifi_config -SE|保存/擦除 Wi-Fi 配置信息|
    |示例|wifi_config -S|保存 Wi-Fi 配置信息|

### 扫描命令

1. 扫描命令

    |||||
    |-|-|-|-|
    |命令定义|wifi_scan [-m] [-r <rssi (-120 ~ 0)>] [-s <ssid>] [-b <bssid (xx:xx:xx:xx:xx:xx)>] [-p <time (ms)>] [-i <mesh_id (xx:xx:xx:xx:xx:xx)>] [-P <mesh_password>]||
    |参数|rssi|通过 RSSI 过滤 设备|
    ||ssid|通过 RSSI 过滤 设备|
    ||bssid|通过 bssid 过滤 设备|
    ||passive|每个信道被动扫描时间|
    ||mesh|扫描 mesh 设备|
    ||mesh_id|mesh_id|
    ||mesh_password|mesh_password|
    |示例|wifi_scan -m|扫描 mesh 设备|
    ||wifi_scan -m -r -60|扫描 RSSI 信号值在 -60 以内的 mesh 设备|
    ||wifi_scan -m -s espressif|扫描 SSID 为 espressif 下的 mesh 设备|
    ||wifi_scan -p 300|设置每个信道被动扫描时间为 300 ms|

### 日志命令

1. 日志配置

    |||||
    |-|-|-|-|
    |命令定义|log  [-ari] <mac (xx:xx:xx:xx:xx:xx)> [-t <tag>] [-l <level>]||
    |指令|log -a|添加日志监视设备|
    ||log -r|移除日志监视设备|
    |参数|mac|监视设备 MAC 地址|
    ||tag|使用 tag 过滤日志|
    ||level|使用 level 过滤日志|
    |指令|log -i|打印监视设备日志统计数据|
    |参数|mac|监视设备 MAC 地址|
    |示例|log -i 30:ae:a4:80:16:3c|打印监视设备 30:ae:a4:80:16:3c 日志统计数据|
    ||log -a 30:ae:a4:80:16:3c|添加监视设备 30:ae:a4:80:16:3c|
    ||log -r 30:ae:a4:80:16:3c|移除监视设备 30:ae:a4:80:16:3c|
    ||log 30:ae:a4:80:16:3c -t * -l INFO|设置监视设备 30:ae:a4:80:16:3c 上的所有日志输出等级为 INFO|

### coredump 命令

1. coredump 命令

    |||||
    |-|-|-|-|
    |命令定义|coredump  [-fre] <mac (xx:xx:xx:xx:xx:xx)> [-q <seq>]||
    |指令|coredump -f|查询指定设备是否有 coredump 数据|
    ||coredump -r|接收指定设备的 coredump 数据|
    ||coredump -e|擦除指定设备的 coredump 数据|
    |参数|mac|监视设备 MAC 地址|
    ||sequence|coredump 数据的序号|
    |示例|coredump -f 30:ae:a4:80:16:3c|查询设备 30:ae:a4:80:16:3c 是否有 coredump 数据|
    ||coredump -r 30:ae:a4:80:16:3c|接收设备 30:ae:a4:80:16:3c 的 coredump 数据|
    ||coredump -r 30:ae:a4:80:16:3c -q 110|接收设备 30:ae:a4:80:16:3c 的 coredump 数据，从序号为 110 开始|
    ||coredump -e 30:ae:a4:80:16:3c|擦除设备 30:ae:a4:80:16:3c 的 coredump 数据|

> 有关 Core Dump 的信息，可查看 [ESP32 Core Dump](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/api-guides/core_dump.html) 文档

### command 命令

1. command

    |||||
    |-|-|-|-|
    |命令定义|command  <addr ((xx:xx:xx:xx:xx:xx))> <"command">||
    |参数|addr|设备 MAC 地址|
    ||command|需要在指定设备上执行的命令|
    |示例|command 30:ae:a4:80:16:3c help|在设备 30:ae:a4:80:16:3c 执行 help 命令|

#### 其他命令

* `help`: 打印当前支持的所有命令

## 性能影响说明

由于 ESP-NOW 和 ESP-MESH 一样，都是通过 Wi-Fi 接口进行数据包收发，因此，当 ESP-MDF 设备数据传输量较大时，会对其控制命令接收或数据传输产生一些延时。

经实际测试，在网络环境良好的情况下，以下配置参数导致的 ESP-MDF 设备延时是可以忽略的阈值：

* 50 个 ESP-MDF 设备（设备数量越多，网络环境越差）
* ESP-NOW 接收端添加 `10` 个 ESP-MDF 设备（接收端添加数量越多，网络环境越差）
* 传输日志级别为 `info` （日志级别越低，网络环境越差）
