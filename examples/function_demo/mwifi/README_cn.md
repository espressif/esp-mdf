[[EN]](./README_en.md)

# Mwifi 示例

## 介绍

本示例将介绍如何基于 `Mwifi` 模块 APIs，实现设备连接远程外部服务器。设备首先通过 ESP-MESH 将所有数据传输到根节点，根节点使用 LWIP 连接远程服务器。

## 硬件准备

1. 至少两块 ESP32 开发板
2. 一台支持 2.4G 路由器

## 工作流程

### 运行 TCP 服务器

1. 将主机（PC 或手机）连接到路由器
2. 使用 TCP 测试工具（此工具为任意一个第三方的 TCP 测试软件）来创建 TCP 服务器

<div align=center>
<img src="tcp_server_create.png"  width="800">
<p> 创建 TCP 服务器 </p>
</div>

> 注：本示例使用的是安卓[网络测试](https://a.app.qq.com/o/simple.jsp?pkgname=mellow.cyan.nettool)工具

### 配置设备

输入 `make menuconfig`，在 “Example Configuration” 子菜单下，进行配置：

	* 路由器信息：如果您无法获取路由器的信道,请将路由器信道设置为 0，由设备端自动获取
	* ESP-MESH 网络：密码长度要大于 8 位并小于 64 位，设置为空则不加密
	* TCP 服务器：主机上运行的 TCP 服务器信息
	* LED 配置：通过主机控制 GPIO 电平

<div align=center>
<img src="device_config.png"  width="800">
<p> 配置设备 </p>
</div>

### 编译和烧录

```shell
make erase_flash flash -j5 monitor ESPBAUD=921600 ESPPORT=/dev/ttyUSB0
```

### 运行

1. ESP-MESH 设备每隔三秒将会给 TCP 服务发送当前 LED 的状态
2. TCP 服务器可以发送命令控制设备 LED 的状态。当目的地址为 `ff:ff:ff:ff:ff:ff` 时，给所有设备发送

<div align=center>
<img src="tcp_server_send.png"  width="800">
<p> TCP 服务器 </p>
</div>