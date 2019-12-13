[[EN]](./README.md)

# Mwifi 有路由器示例

## 介绍

本示例将介绍如何基于 `Mwifi` 模块 APIs，实现设备连接远程外部服务器。设备首先通过 ESP-WIFI-MESH 将所有数据传输到根节点，根节点使用 LWIP 连接远程服务器。

本示例实现了 mesh 网络中设备数据传输到 TCP 服务器功能，也可在 TCP 服务器中发送数据到指定节点或组。

## 硬件准备

1. 至少两块 ESP32 开发板
2. 一台支持 2.4G 路由器

## 工作流程

### 运行 TCP 服务器

1. 将主机（PC 或手机）连接到路由器。
2. 使用 TCP 测试工具（此工具为任意一个第三方的 TCP 测试软件）来创建 TCP 服务器。

> 注：本示例使用的是 iOS [TCP_UDP](https://itunes.apple.com/cn/app/tcp-udp%E8%B0%83%E8%AF%95%E5%B7%A5%E5%85%B7/id1437239406?mt=8) 测试工具

### 配置设备

输入 `make menuconfig`，在 “Example Configuration” 子菜单下，进行配置：

 * 路由器信息：如果您无法获取路由器的信道，请将路由器信道设置为 0，由设备端自动获取
 * ESP-WIFI-MESH 网络：密码长度要大于 8 位并小于 64 位，设置为空则不加密
 * TCP 服务器：主机上运行的 TCP 服务器信息, 包含：IP 地址、端口

<div align=center>
<img src="device_config.png"  width="800">
<p> 配置设备 </p>
</div>

### 编译和烧录

Make:
```shell
make erase_flash flash -j5 monitor ESPBAUD=921600 ESPPORT=/dev/ttyUSB0
```

CMake:
```shell
idf.py erase_flash flash monitor -b 921600 -p /dev/ttyUSB0
```

### 运行

1. ESP-WIFI-MESH 设备每隔三秒会给 TCP 服务发送当前设备的信息
2. TCP 服务器可以发送数据（数据格式请见下一章节的介绍）到指定地址或者指定组。
	- 当目的地址为 `ff:ff:ff:ff:ff:ff` 时，给所有设备发送。
	- 当使用组地址时，将向该组内所有设备发送。

<div align=center>
<img src="tcp_server.png"  width="500">
<p> TCP 服务器 </p>
</div>

### 数据格式

TCP 服务器通信数据格式：
```
{"dest_addr":"dest mac address","data":"content"}
{"group":"group address","data":"content"}
```

示例:
```
{"dest_addr":"24:0a:c4:08:54:80","data":"Hello ESP-MDF!"}
{"group":"01:00:5e:ae:ae:ae","data":"Hello ESP-MDF!"}
```
