[[English]](README.md)

# 乐鑫 ESP-Mesh 开发框架

乐鑫 ESP-Mesh 开发框架（ESP-MDF）是基于乐鑫芯片 [ESP32](https://www.espressif.com/zh-hans/products/hardware/esp32/overview) 的官方 Mesh 开发框架。

## 1. ESP-MDF 概述

ESP-MDF 是一种多节点组网和数据通信的综合解决方案，基于 [ESP-IDF](https://github.com/espressif/esp-idf) (Espressif IoT Development Framework) 框架和 [ESP-Mesh](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html) 无线通信协议开发，包含设备配网，本地和远程控制，固件升级，设备间联动控制，低功耗方案等一套完整的功能。

以下是 ESP-MDF 主要功能：

* 快速配网：采用 Blufi 与 Wi-Fi 链式配网相结合的方案；
* 多种控制方式：APP 控制、传感器控制、联动控制、远程控制、联动控制；
* 快速稳定升级：采用断点续传功能，提高设备在复杂网络环境下的升级成功率；
* 低功耗方案：可将设备切换至 deepsleep 工作模式，以降低设备功耗；
* 蓝牙应用：蓝牙网关和 iBeacon 功能；
* Sniffer: 用于人流量检测、物品跟踪、室内定位等场景；
* Debug 机制：终端指令配置、设备日志无线传输、设备日志分析；
* 紧急恢复模式：当设备出现不可升级的异常情况时能够进入紧急恢复模式进行固件升级；

以下为 ESP-MDF 的功能框图：

<div align=center>
<img src="docs/_static/esp_mdf_block_diagram.png" width="800">
</div>

## 2. ESP-MDF 特性

ESP-MDF [工程](https://github.com/espressif/esp-mdf) 已全部在 GitHub
开源，您可以基于 ESP-MDF 系统框架开发多种类型的智能设备：智能灯，按键，插座，智能门锁，智能窗帘等。ESP-MDF 主要包含如下功能：

1. 安全高效的配网方式
    * 速度快
        * 结合 [BluFi](https://esp-idf.readthedocs.io/en/latest/api-reference/bluetooth/esp_blufi.html) 与 [ESP-NOW](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html) 链式配网两种配网方式，只有第一个设备采用单连接的 BluFi 配网，之后的所有设备都采用 ESP-NOW 链式配网（已配网成功的设备在一定时间内将配网信息传递给周围未配网的设备）。经测试，对一百个设备进行配网，最快仅需 1 分钟。
    * 操作便捷
        * 配网信息存储于所有已配网的设备中，添加新设备时仅需要在 app 端一键 `添加设备` 即可完成对新设备的配网操作。
    * 完善的异常处理机制
        * 包括路由器密码配置变更、密码错误、路由器损坏等特殊情况的处理。
    * 安全
        * 非对称加密：对 BluFi 和 ESP-NOW 链式配网过程中的网络配置数据进行非对称加密；
        * 白名单机制：扫描设备的蓝牙信号或产品包装上的二维码信息生成待配网设备白名单，配网时仅对白名单上的设备进行配网。

2. 无线控制
    * 局域网控制

    > 关于 ESP-MDF 的局域网控制协议说明，请参考相关协议说明文档: [MDF 设备与 App 之间的通信协议](docs/zh_CN/application-notes/mdf_lan_protocol_guide_cn.md)。

3. 快速稳定的升级过程
    * 专用升级连接：50 个设备同时升级，最快仅需 3 分钟；
    * 支持断点续传：提高设备在复杂网络环境下的升级成功率；

4. 完善的调试工具
    * 指令终端: 通过命令行实现添加、修改、删除和控制 ESP-MDF 设备
    * 概要信息: 通过 mDNS 服务获取 Mesh 网络的 Root IP、MAC、Mesh-ID 等信息
    * 日志统计: 对接收的日志数据进行统计，包括：ERR 和 WARN 日志的数量，设备重启次数，Coredump 接收个数，系统运行时间等
    * 日志保存: 接收设备的日志与 coredump 信息并保存至 SD 卡中

    > 关于调试工具的使用，请参考项目示例 [espnow_debug](https://github.com/espressif/esp-mdf/tree/master/examples/espnow_debug) 的 README.md 说明文档。

5. 支持低功耗模式
    * 叶子结点可选择进入 Deep Sleep 运行模式，降低系统功耗，电流最低可至 5 微安。

6. 支持 iBeacon 功能
    * 可应用于室内定位（弥补 GPS 无法覆盖的室内定位场景）和产品推广（商家可以通过 iBeacon 实时推送产品信息和优惠活动）领域。

7. 支持 BLE & Wi-Fi 监听功能
    * 可应用于人流量统计（通过抓取空气中的无线数据包来统计手持联网设备的数量，进而统计人流量）和路径追踪（通过多个采集点对同一设备的无线数据包在时间上的统计，达到追踪某一联网设备的目的）领域。

8. 支持传感器设备网关
    * ESP32 芯片在运行 Wi-Fi 和 蓝牙协议栈的同时还可以接入多种传感器设备，能很方便的将各种传感器数据传给服务器，因此可以作为多种传感器设备的网关，包括蓝牙设备，红外传感器设备，温度传感器设备等。

9. 设备联动控制
    * ESP-MDF 基于现有的局域网通信协议开发了一种 Mesh 网络内部的设备联动控制方案。用户通过移动端 app 页面设置联动控制方式（比如一打开房门就将客厅的灯全部点亮，离开厨房时关闭厨房的灯，同时打开客厅的灯等）；之后 app 会将用户配置好的条件转换成设备可执行的命令数据发送给设备；设备在运行过程中不断检测环境和自身状态，满足判断条件时，就直接向目标设备发送设定好的控制信息。

## 3. 相关资源

* 查看 ESP-MDF 项目文档请点击 [docs](docs)。
* [ESP-IDF 编程指南](https://esp-idf.readthedocs.io/en/latest/) 是乐鑫物联网开发框架的说明文档。
* [ESP-MESH](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html) 是 ESP-MDF 的无线通信协议基础。
* [BluFi](https://esp-idf.readthedocs.io/en/latest/api-reference/bluetooth/esp_blufi.html) 是一种利用蓝牙配置 Wi-Fi 连接的方式。
* [ESP-NOW](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html) 是乐鑫开发的一种无连接的 Wi-Fi 通信协议。
* 项目示例 [espnow-debug](https://github.com/espressif/esp-mdf/tree/master/examples/espnow_debug) 即为 ESP-MDF 项目的调试工具代码。
* 如您发现 bug 或有功能请求，可在 GitHub 上的 [Issues](https://github.com/espressif/esp-mdf/issues) 提交。请在提交问题之前查看已有的 issue 中是否已经有您的问题。
* 如果您想在 ESP-MDF 上贡献代码，请点击 [贡献代码指南](docs/zh_CN/contribute/contribute_cn.md)。
* 访问 ESP32 官方论坛请点击 [ESP32 BBS](https://esp32.com/) 。
* 关于 ESP32-MeshKit 硬件文档，请至[乐鑫官网](https://www.espressif.com/zh-hans/support/download/documents?keys=&field_technology_tid%5B%5D=18)查看。
