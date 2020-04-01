[[EN]](./README.md)

# Mdebug log打包上传示例

## 介绍

本示例将介绍如何基于 `http_client` 模块 APIs，实现设备连接远程外部服务器。设备作为根节点将所有数据传输至远程服务器。

本示例实现了 http 网络中设备中日志数据传输到 TCP 服务器功能，设备会将数据打包成一个文件传输到服务器上，根据宏 `MWIFI_PAYLOAD_LEN` 的大小进行上传。另外还有一个 `console` 控制的命令，可以通过串口命令得到设备基本信息。

## 硬件准备

1. 一块 ESP32 开发板
2. 一台支持 2.4G 路由器

## 工作流程

### 运行 TCP 服务器

1. 将主机（ PC 或手机）连接到路由器。
2. 使用 TCP 测试工具（此工具为任意一个第三方的 TCP 测试软件）来创建 TCP 服务器。

> 注：本示例使用的是 iOS [TCP_UDP](https://itunes.apple.com/cn/app/tcp-udp%E8%B0%83%E8%AF%95%E5%B7%A5%E5%85%B7/id1437239406?mt=8) 测试工具

### 配置设备

输入 `make menuconfig`，在 “Example Configuration” 子菜单下，进行配置：

 * 设置 `Router SSID` 和 `Router password`
 * 设置 `flash log url`，当您的 TCP 服务器的 IP 地址和设备在同一网段下，才能传输。
 * 例如：

    <div align=center>
    <img src="docs/_static/enter_config.png" width="800">
    <p>配置网络</p>
    </div>

### 编译和烧录

Make:
```shell
make erase_flash flash  ESPBAUD=921600 ESPPORT=/dev/ttyUSB0 monitor
```

CMake:
```shell
idf.py erase_flash flash monitor -b 921600 -p /dev/ttyUSB0
```
> 注意:这里需要根据客户的PC端的串口是多少，从而更改串口进行烧录。

### 操作流程

1. 用户需要打开网络助手工具，连接好网络。
2. 用户通过 console 命令行进行信息控制，通过 help 可以查看输入的格式。
* 主要的串口命令如下所示：
    ```
    console -i     // 这里会输出设备的基本信息
    log -S         // 查看的日志状态，是 flash、espnow、uart 模式
    log -e flash   // 使能flash
    log -r         // 读取日志   
    log -d flash   // 禁止日志写入flash
    ```
> 注意：在 `components/mdebug/include/mdebug.h` 里可以根据实际需求来确定是否开启 log 程序本身的调试输出，以此来确定 log 信息本身是否正确。

<div align=center>
<img src="docs/_static/mdebug_print.png" width="800">
<p>调试使能</p>
</div>

### 运行结果

* 这是串口成功打印数据，表示上传成功，以及通过 console 命令行进行信息控制。

    <table>
    <tr>
        <td ><img src="docs/_static/serial_send_info1.png" width="700"><center>串口数据</center></td>
        <td ><img src="docs/_static/serial_send_info2.png" width="700"><center>串口数据</center></td>
        <td ><img src="docs/_static/console_serial.png" width="700"><center>串口控制指令</center></td>
    </tr>
    </table>

* 这是手机端的显示界面，表示接收到 log 数据。

    <table>
    <tr>
        <td ><img src="docs/_static/tcp_tool1.jpg" width="700"><center>server接受的数据包</center></td>
        <td ><img src="docs/_static/tcp_tool2.jpg" width="700"><center>server接受的数据内容</center></td>
    </tr>
    </table>

### 实验结果

1. 设置了 `MWIFI_PAYLOAD_LEN = 1456` ，则一个数据包最大容纳 MWIFI_PAYLOAD_LEN 字节。

### 注意

1. 手机连接的网络需要和 ESP32 芯片连接的网络相同，这样才能在网络助手工具上收集数据信息。
2. 当对 ESP32 进行烧录的时候，需要将先进行擦除操作，然后烧录。
3. 本示例的分区表里添加了 log_info 区域作为保存日志的空间，烧录前需要对芯片进行擦除。
4. 数据的头是时间戳，但没有进行实时校准，用户可以根据自己的需求进行修改。
5. Console 命令中获取 `uart`、`flash`、`espnow` 状态，不能使用 `console` 命令将其禁止，因为主程序已经强制使能，如果用户想使用这样的功能，可以将主程序中进行关闭。
