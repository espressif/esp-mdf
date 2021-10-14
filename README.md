# ESP-WIFI-MESH Development Framework [[中文]](./README_cn.md)

[![Documentation Status](https://readthedocs.com/projects/espressif-esp-mdf/badge/?version=latest)](https://docs.espressif.com/projects/esp-mdf/en/latest/?badge=latest)

ESP-MDF, or Espressif Mesh Development Framework, is a development framework for [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html), a networking protocol built on top of the Wi-Fi protocol. ESP-MDF is based on the [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview) chip.

## Matters need attention

1. This version of MDF is based on the IDF master branch and is not recommended for product development. If you need a stable version of MDF, please use branch release/v1.0.
1. ESP-MDF master branch already supports ESP32S2, but some example can not build and run on ESP32S2 now. We will support these in the future. They are:
    - function_demo/mconfig
    - development_kit/buddy
    - development_kit/button
    - development_kit/light
    - development_kit/sense
    - wireless_debug

## Overview

ESP-MDF is based on the [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) protocol stack to facilitate your development of ESP-WIFI-MESH. ESP-MDF provides the following features:

* **Fast network configuration**: In addition to manual configuration with the network configuration apps, such as ESP-WIFI-MESH App or similar third-party apps, ESP-MDF offers a chained way of network configuration, during which devices autonomously and quickly establish a network, and form a greater coverage area.

* **Stable upgrade**: The upgrading process has become more efficient with such features as automatic retransmission of failed fragments, data compression, reverting to an earlier version, firmware check, etc.

* **Efficient debugging**: Various debugging approaches are supported, such as wireless transmission of logs and wireless debugging, debugging through a command terminal, etc.

* **LAN control**: Network can be controlled by an app, sensor, etc.

* **Various application demos**: It offers comprehensive solutions based on ESP-WIFI-MESH in the areas of lighting, etc.

## Framework

ESP-MDF consists of Utils, Components and Examples (see the below figure). Utils is the encapsulation and third-party library of ESP-IDF APIs. Components are the ESP-MDF functional modules that use Utils APIs. Examples are the ESP-WIFI-MESH solutions based on the Components.

<img src="docs/_static/mdf_framework.jpg">

- **Utils**：
    - Third Party: the third-party items
        - [Driver](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/third_party/index.html): drivers for different devices, such as frequently used buttons and LEDs
        - [Miniz](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/third_party/index.html): lossless, high performance data compression library
        - [Aliyun](https://github.com/espressif/esp-aliyun): Aliyun IoT kit

    - Transmission: the way of data transmission between devices
        - [Mwifi](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/mwifi/index.html): adds to ESP-WIFI-MESH the retransmission filter, data compression, fragmented transmission, and P2P multicast features
        - [Mespnow](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/mespnow/index.html): adds to ESP-NOW the retransmission filter, Cyclic Redundancy Check (CRC), and data fragmentation features

    - Mcommon: modules shared by all ESP-MDF components
        - Event loop: deals with ESP-MDF events
        - Error Check: manages ESP-MDF's code errors
        - Memory Management: Memory Management for ESP-MDF
        - Information Storage: Store configuration information in flash

- **Components**:
    - [Mconfig](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html): network configuration module
    - [Mupgrade](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mupgrade.html): upgrade module
    - [Mdebug](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mupgrade.html): debugging module
    - [Mlink](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mlink.html): LAN control module

- **Examples**:
    - [Function demo](examples/function_demo/): Example of use of each function module
        - [Mwifi Example](examples/function_demo/mwifi): An example of common networking methods: no router, no router. First develop based on this example, then add distribution, upgrade, wireless test and other functions based on it.
        - [Mupgrade Example](examples/function_demo/mupgrade): Upgrade example of the device
        - [Mconfig Example](examples/function_demo/mconfig): Example of network configuration of the device
        - [Mcommon Examples](examples/function_demo/mcommon): Common Module Example, Event Processing Memory Management Example of Using Information Store

    - Debug: Performance Testing and Debugging Tools
        - [Console Test](examples/function_demo/mwifi/console_test): Test the ESP-WIFI-MESH throughput, network configuration, and packet delay by entering commands through the serial port.
        - [Wireless Debug](examples/wireless_debug/): ESP-MDF debugging via wireless
     - [Development Kit](examples/development_kit/): ESP32-MeshKit usage example for research and understanding of ESP-WIFI-MESH
        - [ESP32-MeshKit-Light](examples/development_kit/light/): Smart lighting solution with ESP-WIFI-MESH functioning as the master network. The kit consists of light bulbs with integrated ESP32 chips. Support BLE + ESP-MDF for BLE gateway, ibeacon and BLE scanning
        - [ESP32-MeshKit-Sense](examples/development_kit/sense/): Development board, specifically designed for applications where ESP-WIFI-MESH is in Light-sleep or Deep-sleep mode. The board provides solutions for:
           - Monitoring the power consumption of MeshKit peripherals
           - Controlling MeshKit peripherals based on the data from multiple onboard sensors. 
        - [ESP32-MeshKit-Button](examples/development_kit/button/): Smart button solution, tailored for ESP-WIFI-MESH applications with ultra-low power consumption. The device wakes up only for a short time when the buttons are pressed and transmits packets to ESP-WIFI-MESH devices via [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/network/esp_now.html).


     - Cloud Platform: ESP-MDF docking cloud platform
        - [Aliyun Linkkit](examples/maliyun_linkkit/): Example of ESP-MDF access to Alibaba Cloud platform
        - AWS: ESP-MDF Access AWS Platform Example
## Develop with ESP-MDF

You first need to read [ESP-WIFI-MESH Communication Protocol](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) and [ESP-MDF Programming Guide](Https://docs.espressif.com/projects/esp-mdf/en/latest/?badge=latest) and research and learn about ESP-WIFI-MESH through the ESP32-MeshKit development kit. Secondly, based on [Function demo](examples/function_demo/) for your project development, when you can encounter problems in development, you can first go to [BBS](https://esp32.com/viewforum.php?f=21&sid=27bd50a0e45d47b228726ee55437f57e) and [Issues](https://github.com/espressif/esp-mdf/issues) to find out if a similar problem already exists. If there is no similar problem, you can also ask directly on the website.

### Development Boards

#### ESP32-MeshKit Development board

ESP32-MeshKit offers a complete [ESP-WIFI-MESH Lighting Solution](https://www.espressif.com/en/products/software/esp-mesh/overview) (see the below figure), complemented by ESP-Mesh App ([iOS version](https://itunes.apple.com/cn/app/esp-mesh/id1420425921?mt=8) and [Android](https://github.com/EspressifApp/Esp32MeshForAndroid/raw/master/release/mesh.apk)) for research, development and better understanding of ESP-WIFI-MESH.

<table>
    <tr>
        <td ><img src="docs/_static/ESP32-MeshKit_Light.jpg" width="550"><p align=center>ESP32-MeshKit Light</p></td>
        <td ><img src="docs/_static/ESP32-MeshKit_Sense.jpg" width="600"><p align=center>ESP32-MeshKit Sense</p></td>
    </tr>
</table>

* Products:
    * [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf): The RGBCW smart lights that show control results visually. They can be used to test network configuration time, response speed, stability performance, and measure distance, etc.

    * [ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md): This kit is equipped with a light sensor as well as a temperature & humidity sensor. It can measure power consumption and develop low power applications. The kit may also be used with ESP-Prog for firmware downloading and debugging.

    * [ESP32-MeshKit-Button](examples/development_kit/button/docs/ESP32-MeshKit-Button_Schematic.pdf): Serves as an on/off controller, ready for the development of low power applications. It can be used with ESP-Prog for firmware downloading and debugging.

#### ESP32-Buddy Development board

ESP32-Buddy is a development board specifically designed to test the development of ESP-WIFI-MESH. With its small size and USB power input, the board can be conveniently used for testing a large number of devices and measure distances between them.

* Functions:
    * 16 MB flash: stores logs
    * OLED screen: displays information about the device, such as its layer, connection status, etc.
    * LED: indicates the board's status
    * Temperature & humidity sensor: collects environmental parameters

### Quick Start

This section provides the steps for quick start with your development of ESP-MDF applications. For more details, please refer to [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/v4.0.1/get-started/index.html#get-started).

The directory ``~/esp`` will be used further to install the compiling toolchain, ESP-MDF and demo programs. You can use another directory, but make sure to modify the commands accordingly.

1. [**Setup Toolchain**](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html#step-1-set-up-the-toolchain): please set up according to your PC's operating system ([Windows](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/windows-setup.html), [Linux](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/linux-setup.html) or [Mac OS](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/macos-setup.html)). If you use linux, you can use this commands.

    ```shell
    git clone -b v4.3.1 --recursive https://github.com/espressif/esp-idf.git
    cd ~/esp/esp-idf
    ./install.sh
    . ./export.sh
    ```

1. **Get ESP-MDF**:

    ```shell
    git clone --recursive https://github.com/espressif/esp-mdf.git
    ```

    If you clone without the `--recursive` option, please navigate to the esp-mdf directory and run the command `git submodule update --init`

1. **Set up ESP-MDF Path**: Toolchain uses the environment variable ``MDF_PATH`` to access ESP-MDF. The setup of this variable is similar to that of the variable ``IDF_PATH``. Please refer to [`Add IDF_PATH & idf.py PATH to User Profile`](https://docs.espressif.com/projects/esp-idf/en/v4.0.1/get-started/index.html#step-4-set-up-the-environment-variables). If you use linux, you can use this commands.

    ```shell
    cd ~/esp/esp-mdf
    export MDF_PATH=~/esp/esp-mdf
    ```

1. **Start a Project**: The word *project* refers to the communication example between two ESP-WIFI-MESH devices.

    ```shell
    cp -r $MDF_PATH/examples/get-started/ .
    cd  get-started/
    ```

1. **Build and Flash**: For the rest, just keep the default configuration untouched.

    ```shell
    idf.py menuconfig
    idf.py -p [port] -b [baudrate] erase_flash flash
    ```

1. [**Monitor/Debugging**](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/idf-monitor.html): If you want to exit the monitor, please use the shortcut key ``Ctrl+]``.

    ```shell
    idf.py monitor
    ```

1. **Update ESP-MDF**:

    ```shell
    cd ~/esp/esp-mdf
    git pull
    git submodule update --init --recursive
    ```

## ESP-WIFI-MESH Highlights

* **Easy setup**: ESP-WIFI-MESH expands the original Wi-Fi hotspot range to the reach of the most distant node in the mesh cloud. Such a network is automatically formed, self-healing and self-organizing. It saves the efforts of laying cables. All you need to do is configure the router password.

* **Gateway free**: The decentralized structure of ESP-WIFI-MESH with the absence of a gateway precludes the overall network breakdown if one single node fails. Even if there is a single ESP-WIFI-MESH device, the network still works as usual.

* **Safer transmission**: Both the data link layer and the application layer can be encrypted.

* **More reliable transmission**: The transmission and data flow control between two devices are more reliable. Also, unicast, multicast and broadcast transmissions are supported.

* **Large network capacity**: ESP-WIFI-MESH takes the form of a tree topology, so one single device can connect to 10 devices at maximum, and an entire network can have over 1,000 nodes.

* **Wider transmission coverage**: The transmission distance between two devices is 30 m through walls, and 200 m without any obstacles in between (relevant to ESP32-DevKitC).
    * **Smart Home**: Even if there are only three to five devices in your home, they can form a network and communicate with one another through walls.
    * **Street light**: If ESP-WIFI-MESH is used for the street lighting scenario, two long-distance devices can communicate with each other.

* **High transmission speed**: For Wi-Fi transmission, the speed can reach up to 10 Mbps.
    * **Environment Control System**: Directly transfers the raw data collected by sensors and analyzes mass data for calibration of algorithms, thereby improving sensors' accuracy.
    * **Background Music System**: Both audio and video transmissions are supported.
    
* **Simultaneously run Wi-Fi and BLE protocol stacks**: ESP32 chips can run both Wi-Fi and BLE protocol stacks side by side and use ESP-WIFI-MESH as the main network to transmit data, receive BLE probe beacon, send BLE broadcasts and connect BLE devices.
    * **Items tracing**: Monitors the BLE or Wi-Fi data packets from a device at multiple selected spots.
    * **Pedestrian counting**: Through monitoring Wi-Fi probe request frames.
    * **Indoor positioning**: Each device functions as a Beacon AP, continuously sending Bluetooth signal to the surroundings. The network can analyze a device's signal intensity and calculate its current position.
    * **Product promotion**: Sends real-time product information and promotions through iBeacon.
    * **Bluetooth gateway**: With each device serving as a Bluetooth gateway, traditional Bluetooth devices can also be connected to an ESP-WIFI-MESH network.

## Related Documentation

* For ESP-MDF related documents, please go to [ESP-MDF Programming Guide](https://docs.espressif.com/projects/esp-mdf/en/latest/?badge=latest).
* [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) is the basic wireless communication protocol for ESP-MDF.
* [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/) describes Espressif's IoT development framework.
* To report a bug or request a function, please go to [Issues](https://github.com/espressif/esp-mdf/issues) on GitHub to submit them. Before submitting an issue, please check if it has already been covered.
* If you want to contribute ESP-MDF related codes, please refer to [Code Contribution Guide](docs/en/contribute/index.rst).
* To visit ESP32 official forum, please go to [ESP32 BBS](https://esp32.com/).
* For the hardware documents related to ESP32-MeshKit, please visit [Espressif Website](https://www.espressif.com/en/support/download/documents).
* ESP32-MeshKit-Light purchase link: [Taobao](https://item.taobao.com/item.htm?spm=a230r.1.14.1.55a83647K8jlrh&id=573310711489&ns=1&abbucket=3#detail).
* ESP32-Buddy purchase link: Coming soon.
