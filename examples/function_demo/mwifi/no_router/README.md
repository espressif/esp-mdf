[[中文]](./README_cn.md)

# No Router Example

## Introduction

It introduces a quick way to build an ESP-MESH network without a router, and transparently transmitted to the specified node through the serial port.For another detailed network configuration method, please refer to [examples/function_demo/mwifi](../function_demo/mwifi/README.md). Before running the example, please read the documents [README](../../README_en.md) and [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html).

## Configure

To run this example, you need at least two development boards, one configured as a root node, and the other a non-root node. In this example, all the devices are non-root nodes by default.

- Root node: There is only one root node in an ESP-MESH network. `MESH` networks can be differentiated by their `MESH_ID` and channels.
- Non-root node: Include leaf nodes and intermediate nodes, which automatically select their parent nodes according to the network conditions.

You need to go to the submenu `Example Configuration` and configure one device as a root node, and the others as non-root nodes with `make menuconfig`(Make) or `idf.py menuconfig`(CMake).

You can also go to the submenu `Component config -> MDF Mwifi`, and configure the ESP-MESH related parameters like max number of layers, the number of the connected devices on each layer, the broadcast interval, etc.

<div align=center>
<img src="menuconfig.png" width="800">
<p> Configure the device type </p>
</div>

## Run

1. Set the event callback function; 
2. Initialize Wi-FI, and start ESP-MESH according to configuration;
3. Create an event handler task:
	- Root node:
		- Create a serial port processing task, receive JSON format data from the serial port, parse and forward to the destination address
		- Receive data from a non-root node (the destination address may be the MAC address of the root node itself or the root node address specified by the MESH network) and forward data to the serial port
	- Non-root nodes:
		- Create a serial port processing task, receive JSON format data from the serial port, parse and forward to the destination address
		- Receive data from the root node and forward data to the serial port
4. Create a timer: print at the specified time the ESP-MESH network information about layers, the signal strength and the remaining memory of the parent node.

## Serial data format

Format:
```
{"addr":"dest mac address","data":"content"}
```

Example:
```
{"addr":"30:ae:a4:80:4c:3c","data":"Hello ESP-MESH!"}
```
