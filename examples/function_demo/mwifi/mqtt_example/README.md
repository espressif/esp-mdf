[[中文]](./README_cn.md)

# Mwifi Demo

## Overview

This demo will show how to connect a device to a remote external server based on `Mwifi` APIs. The device first transfers all data through ESP-WIFI-MESH network to the root node that connects to remote server (mqtt://iot.eclipse.org) using MQTT.

## Hardware

1. At least two ESP32 development boards
2. One 2.4 G router
3. One phone or PC with MQTT debugging tool

## Workflow

### Configure Your Device

Type `make menuconfig`, and configure the device in “Example Configuration” submenu:

<div align=center>
<img src="config.png"  width="800">
<p> Configure Your Device </p>
</div>

### Compile and Flash

```shell
make erase_flash flash -j5 monitor ESPBAUD=921600 ESPPORT=/dev/ttyUSB0
```

### Run Your Device

1. The ESP-WIFI-MESH device advertises its information to Topic:"/topic/subdev/MAC/send" (MAC: node MAC address) every three seconds.
2. When routing table in ESP-WIFI-MESH network changes, the ESP-WIFI-MESH device will advertises relevant information about the changed node to Topic:"/topic/gateway/MAC/update" (MAC: root node MAC address).
3. The device receives server data from Topic:"/topic/subdev/MAC/recv".

For example:

- If you subscribe `/topic/subdev/240ac4085480/send` topic in MQTT debugging tool, you will receive data from the device 240ac4085480.
- If you send data to `/topic/subdev/240ac4085480/recv` topic in MQTT debugging tool, the device with the MAC address 240ac4085480 will receive the data.

<div align=center>
<img src="running.png"  width="600">
</div>