[[中文]](./README_cn.md)

# ESP32-MeshKit Guide

---

## Overview

ESP32-MeshKit is a network configuration solution for smart homes based on [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html).

ESP32-MeshKit currently allows you to integrate the following hardware components:

* [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf): Smart lighting solution with ESP-WIFI-MESH functioning as the master network. The kit consists of light bulbs with integrated ESP32 chips.

* [ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md): Development board, specifically designed for applications where ESP-WIFI-MESH is in Light-sleep or Deep-sleep mode. The board provides solutions for:
   * Monitoring the power consumption of MeshKit peripherals
   * Controlling MeshKit peripherals based on the data from multiple onboard sensors. 
  
* [ESP32-MeshKit-Button](button/README.md): Smart button solution, tailored for ESP-WIFI-MESH applications with ultra-low power consumption. The device wakes up only for a short time when the buttons are pressed and transmits packets to ESP-WIFI-MESH devices via [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/network/esp_now.html).

To configure and network these hardware components you need:

* Android or iOS phone with installed ESP-Mesh App (See [section ESP-Mesh App](#esp-mesh-app)).
* 2.4 GHz Wi-Fi network to which you connect your phone and one of the ESP-WIFI-MESH devices.

## Functions

1. [Mconfig](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html) (MESH Network Configuration)

	A network configuration solution, which uses ESP-Mesh App to add the first device to ESP-WIFI-MESH network via Bluetooth. After that, the added device transfers network configuration information to other devices waiting to be added.

2. [Mlink](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mlink.html) (MESH LAN Communication)

	A LAN control solution for ESP-WIFI-MESH, where the root node initiates communication between the network configuration app and the HTTP server, and transfers the communication information to other devices.

3. [Mupgrade](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mupgrade.html) (MESH Upgrade)

	A solution for simultaneous over-the-air (OTA) upgrading of multiple ESP-WIFI-MESH devices on the same wireless network. This solution provides the following functions:

## ESP-Mesh App

* **Android**: [source code](https://github.com/EspressifApp/EspMeshForAndroid), [apk](https://www.espressif.com/zh-hans/support/download/apps?keys=&field_technology_tid%5B%5D=18) (installation package)
* **iOS**: Go to *App Store* and search for `ESP-Mesh`.
* **WeChat mini program**: Open *WeChat* and search for `ESPMesh`. The mini program currently supports only network configuration.

> **Note**: The Android version updates are given a higher priority.

ESP-Mesh App is a useful tool for researching the ESP-WIFI-MESH protocol and will help better understand the protocol's potential.

The shared ESP-Mesh App's source code will be helpful in development of your own applications.

## Hardware Preparation

* Turn on Bluetooth and Wi-Fi on your smartphone and connect it to the router.

* Make sure the device you want to add is in Network Configuration mode. 
    * To establish a network, you have to use one or more ESP32-MeshKit-Light devices, because only ESP32-MeshKit-Light can serve as a root node (master nodes, similar to gateways). You can bring ESP32-MeshKit-Light into Network Configuration mode by turning it off and on for three consecutive times.
    
    * ESP32-MeshKit-Button and ESP32-MeshKit-Sense can be added to an existing network only. Please refer to their respective guides for the information on how to bring them into Network Configuration mode.

## Network Configuration

### 1. Initial Configuration

* Launch ESP-Mesh App, and it performs scanning via Bluetooth and notifies you about nearby devices in Network Configuration mode.

* Tap on the `Add device` button to see the list of the found ESP-WIFI-MESH devices.

* Tap on the down arrow to the left of the search bar to reveal two filtering options:
    * `RSSI` to filter devices based on their signal strength
    
    * `Only favorites` to display favorite devices only (to add a device to favorites, tap on the device's icon).
   
* Choose the devices you want to add and tap `Next`

    <table>
        <tr>
            <td ><img src="docs/_static/en/enter_configuration_network.png" width="300"><p align=center>Add devices for network configuration</p></td>
            <td ><img src="docs/_static/en/get_device_list.png" width="300"><p align=center>Get the device list</p></td>
        </tr>
    </table>

* Enter the required network configuration information:

    * **Wi-Fi name**: Shows the name of the Wi-Fi network to which the smartphone is connected. Note that only 2.4 GHz Wi-Fi networks are supported.
    
    * **MESH ID**: Suggests the name for the ESP-WIFI-MESH network, which equals to the router's MAC address. If you want to have several ESP-WIFI-MESH networks on the same router, please give them unique names by modifying the initial `MESH ID`. Multiple networks with an identical MESH ID are merged into one network.
   
    * **Password**: Input the password of the current Wi-Fi network.
   
    * **More**: Tap to modify the default configuration parameters of the ESP-WIFI-MESH network. For more information on the parameters, please check the [ESP-WIFI-MESH Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/network/esp_mesh.html).
* After you fill out the required fields, tap `Next`

    <table>
        <tr>
            <td ><img src="docs/_static/en/router_configuration.png" width="300"><p align=center>Enter the router information</p></td>
            <td ><img src="docs/_static/en/more_configuration.png" width="300"><p align=center>Enter ESP-WIFI-MESH network configuration information</p></td>
        </tr>
    </table>

ESP-Mesh App starts uploading the network configuration information and performs the following actions:

* App chooses a device with the strongest Bluetooth signal, connects to it, and sends the network configuration information and device whitelist. The whitelist contains the devices chosen to be added.

* When the device receives the network configuration information, it connects to the router to verify if the information is correct.

* After successful verification, the device notifies App via Bluetooth that configuration is completed, and the device can be networked.

* When the device is successfully networked via Bluetooth, it performs network configuration for the whitelisted devices.

    <table>
        <tr>
            <td ><img src="docs/_static/en/BLE_transmission.png" width="300"><p align=center>BLE Transmission</p></td>
            <td ><img src="docs/_static/en/waiting_for_networking.png" width="300"><p align=center>Wait to be networked</p></td>
        </tr>
    </table>

### 2. Adding Devices to an Existing Network

If App finds a new ESP-WIFI-MESH device in Network configuration mode, it shows a prompt. You can add the device by tapping on `Add the device to the mesh network`.

<table>
    <tr>
        <td ><img src="docs/_static/en/discovery_device.png" width="300"><p align=center>Add the device to MESH network</p></td>
        <td ><img src="docs/_static/en/select_device.png" width="300"><p align=center>Select the device to be added to MESH network</p></td>
    </tr>
</table>

### 3. Device Tab

Go to the list of added devices and do the following:

* Tap on an ESP-MeshKit-Light device to open its lighting settings.    

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/light_control.png" width="300"><p align=center>Light control interface</p></td>
            <td ><img src="docs/_static/zh_CN/button_control.png" width="300"><p align=center>Button control interface</p></td>
        </tr>
   </table>

* Long press on a device to edit its configuration settings:
    * **Device association**: Any ESP-WIFI-MESH device can be associated with other devices. 

    > Note: an association works in one direction only.
    >
    > For example, if you set a *light A > light B* association, then as soon as you turn on *light A*, *light B* will come on. But turning off light B will not affect light A, until you set a *light B > light A* association.

     An *ESP32-MeshKit-Button > ESP32-MeshKit-Light* or *ESP32-MeshKit-Sense > ESP32-MeshKit-Light* association gives you direct control over the lighting settings of the *ESP32-MeshKit-Light*.

    * **Send command**: You can send device debugging commands or your own commands.


    <table>
        <tr>
            <td ><img src="docs/_static/en/editing_device.png" width="300"><p align=center>Select device association</p></td>
            <td ><img src="docs/_static/en/more_configuration.png" width="300"><p align=center>Device association</p></td>
            <td ><img src="docs/_static/en/send_command.jpg" width="300"><p align=center>Send command</p></td>
        </tr>
   </table>

### 4. Group Tab

* **Default group**: App groups the added devices according to their type, which means the number of default groups equals to the number of device types. A default group cannot be deleted.

* **Custom group**: User-defined group for simultaneous control of included devices.

    <table>
        <tr>
            <td ><img src="docs/_static/en/add_group.png" width="300"><p align=center>Add a group</p></td>
            <td ><img src="docs/_static/en/group_name.png" width="300"><p align=center>Edit the group name</p></td>
        </tr>
   </table>

### 5. User Tab

* **Settings**: The following options are available
    * App version
    * Update App
    * Help provides the FAQ section
    
* **Topology**: Layout of the current ESP-WIFI-MESH network structure. You can long press on a certain node to open the device's network configuration information.

<table>
    <tr>
        <td ><img src="docs/_static/en/version_information.png" width="300"><p align=center>App version</p></td>
        <td ><img src="docs/_static/en/topology.png" width="300"><p  align=center>Topology</p></td>
        <td ><img src="docs/_static/en/network_configuration.png" width="300"><p align=center>Network configuration information for an ESP-WIFI-MESH device</p></td>
    </tr>
</table>

### 6. Firmware Update

Long press on a certain ESP-WIFI-MESH device on the list of added devices and in the pop-up menu choose *Firmware update*. Choose one of the two ways to update the firmware:

* **Download the bin**: Directly copy the firmware update to your smartphone's folder `File Management/Phone Storage/Espressif/Esp32/upgrade`.

* **Enter bin URL**: Save the firmware update on a cloud, such as GitHub, or on an HTTP server created within LAN, and enter the link to the saved firmware into the appeared dialog box.

<table>
    <tr>
        <td ><img src="docs/_static/en/add_firmware.png" width="300"><p align=center>Download the bin</p></td>
        <td ><img src="docs/_static/en/url_upgrade.png" width="300"><p align=center>Enter bin URL</p></td>
        <td ><img src="docs/_static/en/upgrading.png" width="300"><p align=center>Update in Process</p></td>
    </tr>
</table>

## Drivers

All the hardware drivers for ESP32-MeshKit use the corresponding driver code in [esp-iot-solution](https://github.com/espressif/esp-iot-solution). You can visit this repository for any code update.
