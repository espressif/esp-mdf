[[中文]](README_cn.md)

# Espressif Mesh Development Framework

Espressif Systems Mesh Development Framework (ESP-MDF) is the official mesh development framework for the [ESP32](https://espressif.com/en/products/hardware/esp32/overview) chip.

## 1. Overview of ESP-MDF

ESP-MDF is a multi-hop mesh networking solution, based on the [ESP-IDF](https://github.com/espressif/esp-idf) (Espressif IoT Development Framework) and [ESP-Mesh](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html) communication protocol. Its functions include device distribution network, local and remote control, firmware upgrade, linkage control between devices, low power consumption solution, etc.

Below are the main features of ESP-MDF:

* Fast networking: combining Blufi networking and ESP-NOW chain networking
* Various control methods: app control, sensor control, linkage control, and remote control
* Fast and stable upgrade: continuous transmission of breakpoints is adopted to improve the success rate of equipment upgrades in complex network environments
* Low-power solutions: reduced system power consumption through a variety of low-power solutions
* Bluetooth application: Bluetooth gateway and iBeacon function
* Sniffer: applicable for pedestrian flow monitoring, tracking of personal belongings, indoor positioning, etc.
* Debug mechanism: terminal instruction configuration, wireless transmission of device log, device log analysis, etc.
* Emergency recovery mode: In case  the device is a situation that cannot be upgraded normally, it can enter the emergency recovery mode for firmware upgrade.

Below is the functional block of ESP-MDF:

<div align=center>
<img src="docs/_static/esp_mdf_block_diagram.png" width="800">
</div>

## 2. ESP-MDF Features

[ESP-MDF](https://github.com/espressif/esp-mdf) is open source on GitHub.

You can develop multiple types of smart devices based on the ESP-MDF system framework: smart lights, buttons, sockets, smart door locks, smart curtains, etc. ESP-MDF has the following features:

1. Safe and efficient distribution network
    * High speed
        * ESP-Mesh implements a networking scenario combining [BluFi](https://esp-idf.readthedocs.io/en/latest/api-reference/bluetooth/esp_blufi.html) and [ESP-NOW](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html). The first device is connected to the router via BluFi, and all subsequent devices are connected via ESP-NOW. (A device that has been successfully deployed in the network will transmit the networking information to devices that are yet to be connected to the network.) In a test we've conducted, the shortest time to network 100 devices is one minute.
    * Convenient operation
        * The networking information is stored in all the devices that have been networked. Adding a new device to the mesh network can be easily accomplished by tapping `add devices` on the ESP-Mesh app.
    * Comprehensive error handling mechanism
        * This includes router password changes, password errors, router damage, etc.
    * Security
        * Asymmetric encryption: Asymmetric encryption: Networking information gets encrypted asymmetrically during the BluFi and ESP-NOW networking;
        * White lists: Scan the device's Bluetooth signal or the QR code, a unique identifier printed on the device packaging, to generate a white list of devices to be networked, and configure the devices exclusively on the white list when networking.

2. Wireless control
    * LAN control

    > For details of the ESP-MDF LAN control protocol, please refer to the document [Communication Protocol Between MDF Devices and the App](docs/en/application-notes/mdf_lan_protocol_guide_en.md).

3. Rapid and stable upgrades
    * Dedicated upgrade connection: it takes as short as 3 minutes to upgrade 50 devices at the same time;
    * Support for breakpoint resumption: improves the success rate of upgrading the devices in a complex network environment;

4. Comprehensive debugging tools
    * Command terminal: Add, modify, delete, and control ESP-MDF devices using command line
    * Acquisition of networking information: Get the Root IP, MAC, Mesh-ID, and other information of the Mesh network through the mDNS service
    * Log statistics: statistics of the received log, including: the number of ERR and WARN logs, the number of device restarts, the number of coredumps received, the system running time, etc.
    * Log reservation: Receives device's log and coredump information and save it to SD card

    > For details on using the debugging tool, please refer to the README.md document of the project [espnow_debug](https://github.com/espressif/esp-mdf/tree/master/examples/espnow_debug).

5. Support for low power mode
    * The leaf nodes can enter Deep Sleep mode when necessary to reduce system power consumption, in which the current consumption can be as low as 5 μA.

6. Support for iBeacon function
    * Suitable for indoor positioning (especially for indoor positioning scenarios in which GPS is unavailable) and product promotion (whereby sellers push product information and promotions in real time via iBeacon to handheld devices).

7. Support for BLE & Wi-Fi Sniffer monitoring
    * Applicable to pedestrian flow monitoring (which can be done by calculating the number of portable IOT devices using ESP32 to sniffer data packets in the air) and path tracking (which can be done by sampling the wireless data packets of the same device at multiple locations and note down the time the packets are sent from the device at different locations).

8. Support for sensor device gateway
   * ESP32 can be connected to various sensor devices while running Wi-Fi and Bluetooth protocol stacks. It can easily transmit various sensor data to the server, thus serving as a gateway for a variety of sensor devices, such as Bluetooth devices, infrared sensors, temperature sensors, etc.

9. Device Connectivity control
    * ESP-MDF contains a device connectivity control scheme within the Mesh network based on the existing LAN communication protocols. The user configures the connectivity control operations in the ESP-Mesh app (such as turning on the lights in the living room when the door is opened, turning off the lights in the kitchen while turning on the lights in the living room when the user leaves the kitchen, etc.). Then the app converts the configurations into commands and sends them to the device. While running, the device continuously monitors the environment and its own state. When the condition for triggering the connectivity control operation is satisfied, the commands are directly sent to the target device.

## 3. Resources

* ESP-MDF Documents are stored in [docs](docs) directory.
* [ESP-IDF Programming Guide](https://esp-idf.readthedocs.io/en/latest/) is the guide to Espressi's IoT Development Framework.
* [ESP-Mesh](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html) is the wireless communication protocol for ESP-MDF.
* [BluFi](https://esp-idf.readthedocs.io/en/latest/api-reference/bluetooth/esp_blufi.html) is a method to configure Wi-Fi networking via Bluetooth.
* [ESP-NOW](https://esp-idf.readthedocs.io/en/latest/api-reference/wifi/esp_now.html) is a wireless communication protocol developed by Espressif.
* The project [espnow-debug](https://github.com/espressif/esp-mdf/tree/master/examples/espnow_debug) is the codes of debug tools for ESP-MDF.
* If you find a bug or have a feature request, please check the [Issues](https://github.com/espressif/esp-mdf/issues) section on GitHub prior to opening a new issue.
* The [ESP32 BBS](https://esp32.com/) is a place to ask questions and find community resources.
* If you're interested in contributing to ESP-MDF, please go to [Contributions Guide](docs/en/contribute/contribute_en.md).
* For ESP32-MeshKit-related documents, please go to [Espressif's Official Website](https://www.espressif.com/en/support/download/documents?keys=&field_technology_tid%5B%5D=18).
