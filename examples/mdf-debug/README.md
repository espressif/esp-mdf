# 0. MDF-DEBUG 示例说明

本示例是 ESP-MDF 工程的调试示例，通过 [ESP-NOW](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html) 无线传输技术接收 ESP-MDF 设备的设备运行日志和 Coredump 数据。主要功能包括：

1. 通过 mDNS 服务获取 ESP-MESH 网络的基本信息，包括：Root IP、MAC 地址、Mesh-ID 等；
2. 包含 console 模块，通过命令行实现对路由器设置、运行设备的添加、删除和打印参数设置；
3. 接收 ESP-MDF 设备的 log 与 coredump 信息并保存至 SD 卡；
4. 对接收的 log 数据进行统计，显示所添加的设备的运行状态，包括：ERR 和 WARN log 数量、Reboot 次数、Coredump 接收个数、设备运行时间等；

## 1. ESP-NOW 介绍

### 1.1. 概述

ESP-NOW 是由 Espressif 定义的一种无连接的 Wi-Fi 通信协议，广泛应用于智能照明，遥控，传感器等领域。在 ESP-NOW 中，数据被封装在 Wi-Fi Action 帧中进行数据的收发，在没有连接的情况下从一个 Wi-Fi 设备传输数据到另一个 Wi-Fi 设备。

### 1.2. ESP-NOW 特性

1. 数据包的最大长度限制 `250 Bytes`；
2. 收发双方必须在 `同一个信道上`；
3. 发送端口可以是 Station 或 SoftAP（对应的 MAC 地址不一样）；
4. 接收端可以不添加发送端 MAC 地址，但是发送端必须要添加接收端的 MAC 地址；
5. ESP-NOW 最多可添加 `20` 个配对设备。同时，支持其中最多 `6` 个设备进行通信加密。
6. 通过注册回调函数的方式接收数据包，以及检查发送情况（成功或失败）；
7. 利用 CTR 和 CBC-MAC 协议（CCMP）保护 Action 帧的安全性；

> 更多关于 ESP-NOW 的原理介绍，请参考 Espressif 官方手册 [ESP-NOW 使用说明](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html)。

## 2. 示例说明

### 2.1. 硬件说明

该项目使用的开发板为乐鑫开发的 [ESP-WROVER-KIT V2](https://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards-previous.html#esp-wrover-kit-v2) 开发板 [使用说明](https://esp-idf.readthedocs.io/en/latest/get-started/get-started-wrover-kit-v2.html) 如链接所示。在开发板中需要插入 SD 卡，以保存由 ESP-NOW 接收的运行日志。

<div align=center>
<img src="docs/_static/esp-wrover-kit-v2.jpg" width="400">
<p> ESP-WROVER-KIT-V2 </p>
</div>

### 2.2. 项目文件组织结构

```
mdf-debug
├── build                        // All files generated through compiling
├── components
│   ├── espnow_console           // console module for interaction with terminal device
│   ├── espnow_lcd               // lcd module for info display
│   └── espnow sdcard            // sdcard module for data storage
├── main
│   ├── component.mk
│   ├── espnow_recv_handle.c     // handle with espnow received data
│   └── espnow_terminal.c        // Main entry of project
├── gen_misc.sh                  // script for compiling and downloading
├── Makefile
├── partitions.csv               // partition table file
├── README.md
└── sdkconfig                    // configuration options
```

### 2.3. 命令格式说明

Debug 命令包括: `join`, `channel`, `add`, `del`, `list`, `log`, `llog`, `dumpreq` and `dumperase`.

#### 2.3.1. 获取信道

1. join router_ssid router_pwd
    - 说明：连接路由器。
2. channel primary_channel secondary_channel
    - 说明：在没有连接路由器的时候设置 channel 用于接收 espnow log。
        - primary channel 可以选择 1-13
        - secondary channel 可以选择 none, above, below。

`join` 命令与 `channel` 命令相比，除了可以获取 ESP-MDF 设备的网络信道外，还可以获取根节点提供的 mDNS 信息，包括根节点的 IP 地址、通信端口、Mac 地址等。

#### 2.3.2. 添加删除设备

1. add -m aa:bb:cc:dd:ee:ff -m 12:34:56:78:90:ab
    - 说明：`add` 为添加设备命令，`-m` 参数表示通过设备 Mac 地址进行添加。一次最多可添加 5 个设备。
2. del -m aa:bb:cc:dd:ee:ff -m 12:34:56:78:90:ab
    - 说明：`del` 为删除设备命令，`-m` 参数表示通过设备 Mac 地址进行删除。一次最多可删除 5 个设备。
    - 不指定 `-m` 参数表示删除所有的设备。
3. list
    - 说明：打印已添加的设备数量及 Mac 地址。

#### 2.3.3. 调整日志级别

1. log -l level
    - 说明：`log` 为设置设备 log 命令，`-l` 参数表示调整设备的 log 级别，用于调整 ESP-MDF 设备的发送日志界别。level 有如下几种：
```
    MDF_ESPNOW_LOG_NONE,     /**< no output */
    MDF_ESPNOW_LOG_ERROR,    /**< error log */
    MDF_ESPNOW_LOG_WARN,     /**< warning log */
    MDF_ESPNOW_LOG_INFO,     /**< info log */
    MDF_ESPNOW_LOG_DEBUG,    /**< debug log */
    MDF_ESPNOW_LOG_VERBOSE,  /**< verbose log */
    MDF_ESPNOW_LOG_RESTORE,  /**< restore to original ESP_LOG* channel */
```
    其中 `MDF_ESPNOW_LOG_RESTORE` 表示设备 log 恢复到原串口打印通道。
2. llog -t tag -l level
    - 说明：调整 mdf_debug 设备自身的打印日志级别。
        - `-l` 同 log 的 level
        - `-t` 为 log tag。

#### 2.3.4. Coredump 数据请求与擦除

1. dumpreq
    - 说明：请求设备的 coredump 数据，如果设备的 flash 中有 coredump 数据，则开启线程并发送 coredump 数据；若没有 coredump 数据，则回复 `mdf_coredump_get_info no coredump data`。
2. dumperase
    - 说明：请求设备擦除 coredump 数据。

> 此外还包括：`free` 和 `restart` 命令，用于调试。
> * `free`: 显示系统剩余 Heap Memory
> * `restart`: 重启设备

## 3. 性能影响说明

由于 ESP-NOW 和 ESP-MESH 一样，都是通过 Wi-Fi 接口进行数据包收发，因此，当 Mesh 设备的 ESP-NOW Debug 功能打开且数据传输量较大时，会对设备的控制或数据传输产生一些延时。

经实际测试，在网络环境良好的情况下，以下配置参数是设备控制延时可以忽略的阈值：
* 50 个 ESP-MDF 设备（设备数量越多，网络环境越差）
* ESP-NOW 接收端添加 `10` 个设备（接收端添加数量越多，网络环境越差）
* APP 控制命令发送的频率为 `1 秒/次`（控制频率越高，网络环境越差）
* 传输 log 级别为 `info` （log 级别越低，网络环境越差；log 级别从高到底依次为：ERROR > WARN > INFO > DEBUG > VERBOSE）
