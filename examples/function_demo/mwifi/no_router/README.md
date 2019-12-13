[[中文]](./README_cn.md)

# No Router Example

## Introduction

It introduces a quick way to build an ESP-WIFI-MESH network without a router, and transparently transmitted to the specified node through the serial port. For details on other network configuration methods, please refer to [README](../README.md). Before running this example, please firstly go through [README](../../README.md) and [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html).

## Configure

To run this example, you need at least two development boards, one configured as a root node, and the other a non-root node. In this example, all the devices are non-root nodes by default.

- Root node: There is only one root node in an ESP-WIFI-MESH network. `MESH` networks can be differentiated by their `MESH_ID` and channels.
- Non-root node: Includes leaf nodes and intermediate nodes, which automatically select their parent nodes according to the network conditions.
	- Leaf node: A leaf node cannot also be an intermediate node, which means leaf node cannot has any child nodes.

You need to go to the submenu `Example Configuration` and configure one device as a root node, and the others as non-root nodes with `make menuconfig`(Make) or `idf.py menuconfig`(CMake). 

You can also go to the submenu `Component config -> MDF Mwifi`, and configure the ESP-WIFI-MESH related parameters such as the max number of layers allowed, the number of the connected devices on each layer, and the broadcast interval.


<div align=center>
<img src="menuconfig.png" width="800">
<p> Configure the device type </p>
</div>

## Run

1. Set the event callback function; 
2. Initialize Wi-Fi, and start ESP-WIFI-MESH according to configuration;
3. Create an event handler task:
	- Root node:
		- Creates a serial port processing task, receives JSON format data from the serial port, parses and forwards to the destination address
		- Receives data from a non-root node (the destination address may be the MAC address of the root node itself or the root node address specified by the MESH network) and forward data to the serial port
	- Non-root nodes:
		- Create a serial port processing task, receive JSON format data from the serial port, parse and forward to the destination address
		- Receive data from the root node and forward data to the serial port
4. Create a timer: print at the specified time the ESP-WIFI-MESH network information about layers, and the parent node's signal strength and the remaining memory.

<div align=center>
<img src="serial_port.png" width="800">
<p> 串口终端 </p>
</div>

## Serial data format

Format:
```
{"dest_addr":"dest mac address","data":"content"}
{"group":"group address","data":"content"}
```
Example:
```
{"dest_addr":"30:ae:a4:80:4c:3c","data":"Hello ESP-WIFI-MESH!"}
{"group":"01:00:5e:ae:ae:ae","data":"Hello ESP-WIFI-MESH!"}
```