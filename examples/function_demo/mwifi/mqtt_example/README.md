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

### Run on Your Device

1. ESP-WIFI-MESH sub-device will send heartbeat information to the root node every three seconds, the information content is as follows

    ```json
    {
        "type":"heartbeat",
        "self":"840d8ee3b1d8",
        "parent":"a45602473007",
        "layer":1
    }
    ```

    > type is the message type, self is the MAC address of the current device, parent is the MAC address of the parent node of the current device, and layer is the layer book of the current device. It is strongly recommended that developers retain this mechanism when designing MESH systems that communicate based on MQTT. In this way, it is convenient for Cloud to judge whether the device is online, and it is easy to see the complete TOPO structure of the MESH network

1. After the ESP-WIFI-MESH root node receives the data from the child node, it will forward the message from the child node to Topic: "mesh/{MAC}/toCloud" (MAC: MAC address of the root node). The content of the information is as follows

    ```json
    {
        "addr":"840d8ee3b1d8",
        "type":"json",
        "data":{
            "type":"heartbeat",
            "self":"840d8ee3b1d8",
            "parent":"a45602473007",
            "layer":1
        }
    }
    ```

    > addr is the MAC address of the child node, type is the message type, and data is the message from the child device.

1. When the routing table changes in the MESH network, the root node will push the changed node-related information to Topic: "mesh/{MAC}/topo" (MAC: the root node's MAC address). The information content is as follows:

    ```json
    [
        "840d8ee3b1d8",
        "30aea4800ea0"
    ]
    ```

    > The list completely copies the routing table of the current root node

1. The root node receives data from the server from Topic: "mesh/{MAC}/toDevice" (MAC can be 3 values, respectively the root node MAC address, "ffffffffffff" and "ff0000010000")

For example, there are two nodes running the case, namely 840d8ee3b1d8 (root node) and 30aea4800ea0 (child node).

- Subscribing to `mesh/+/toCloud` topic in mosquitto_sub will receive messages from the device as follows:

    ```bash
    mesh/840d8ee3b1d8/toCloud {"addr":"840d8ee3b1d8","type":"json","data":{"type":"heartbeat", "self": "840d8ee3b1d8", "parent":"a45602473007","layer":1}}
    mesh/840d8ee3b1d8/toCloud {"addr":"30aea4800ea0","type":"json","data":{"type":"heartbeat", "self": "30aea4800ea0", "parent":"840d8ee3b1d9","layer":2}}
    mesh/840d8ee3b1d8/toCloud {"addr":"840d8ee3b1d8","type":"json","data":{"type":"heartbeat", "self": "840d8ee3b1d8", "parent":"a45602473007","layer":1}}
    mesh/840d8ee3b1d8/toCloud {"addr":"30aea4800ea0","type":"json","data":{"type":"heartbeat", "self": "30aea4800ea0", "parent":"840d8ee3b1d9","layer":2}}
    mesh/840d8ee3b1d8/toCloud {"addr":"840d8ee3b1d8","type":"json","data":{"type":"heartbeat", "self": "840d8ee3b1d8", "parent":"a45602473007","layer":1}}
    mesh/840d8ee3b1d8/toCloud {"addr":"30aea4800ea0","type":"json","data":{"type":"heartbeat", "self": "30aea4800ea0", "parent":"840d8ee3b1d9","layer":2}}
    mesh/840d8ee3b1d8/toCloud {"addr":"840d8ee3b1d8","type":"json","data":{"type":"heartbeat", "self": "840d8ee3b1d8", "parent":"a45602473007","layer":1}}
    ```

- Subscribing to `mesh/+/topo` topic in mosquitto_sub will receive messages from the device as follows:

    ```bash
    mesh/840d8ee3b1d8/topo ["840d8ee3b1d8"]
    mesh/840d8ee3b1d8/topo ["840d8ee3b1d8","30aea4800ea0"]
    ```

- Send a message to `mesh/840d8ee3b1d8/toDevice` topic in mosquitto_pub, the content of the message is as follows

    ```json
    {
        "addr":["30aea4800ea0","840d8ee3b1d8"],
        "type":"json",
        "data":{
            "key1":"123",
            "key2":"456"
        }
    }
    ```

- At this point, both nodes receive the contents of the data field, and the log is as follows:

    ```bash
    I (987708) [mqtt_examples, 116]: Node receive: 84:0d:8e:e3:b1:d8, size: 27, data: {"key1":"123","key2":"456"}
    ```

    ```bash
    I (623724) [mqtt_examples, 118]: Node receive: 84:0d:8e:e3:b1:d8, size: 27, data: {"key1":"123","key2":"456"}
    ```
