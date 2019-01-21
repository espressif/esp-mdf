# ESP-MESH Development Framework [[中文]](./README_cn.md)

[![alt text](https://readthedocs.org/projects/docs/badge/?version=latest "Documentation Status")](https://docs.espressif.com/projects/esp-mdf/en/latest/?badge=latest)

ESP-MDF, or Espressif Mesh Development Framework, is a development framework for [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html), a networking protocol built on top of the Wi-Fi protocol. ESP-MDF is based on the [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview) chip.

## Overview
ESP-MDF is based on the [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html) protocol stack to facilitate your development of ESP-MESH. ESP-MDF provides the following features:

* **Fast network configuration**: In addition to manual configuration with the network configuration apps, such as ESP-MESH App or similar third-party apps, ESP-MDF offers a chained way of network configuration, during which devices autonomously and quickly establish a network, and form a greater coverage area.

* **Stable upgrade**: The upgrading process has become more efficient with such features as automatic retransmission of failed fragments, data compression, reverting to an earlier version, firmware check, etc.

* **Efficient debugging**: Various debugging approaches are supported, such as wireless transmission of logs and wireless debugging, debugging through a command terminal, etc.

* **LAN control**: Network can be controlled by an app, sensor, etc.

* **Various application demos**: It offers comprehensive solutions based on ESP-MESH in the areas of lighting, indoor positioning, etc.

## Framework
ESP-MDF consists of Utils, Components and Examples (see the below figure). Utils is the encapsulation and third-party library of ESP-IDF APIs. Components are the ESP-MDF functional modules that use Utils APIs. Examples are the ESP-MESH solutions based on the Components.

<img src="docs/_static/mdf_framework.jpg">

- **Utils**：
    - Third Party: the third-party items
        - [Driver](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/third_party/index.html): drivers for different devices, such as frequently used buttons and LEDs
        - [Miniz](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/third_party/index.html): lossless, high performance data compression library
        - [Aliyun](https://github.com/espressif/esp-aliyun): Aliyun IoT kit

    - Transmission: the way of data transmission between devices
        - [Mwifi](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/mwifi/index.html): adds to ESP-MESH the retransmission filter, data compression, fragmented transmission, and P2P multicast features
        - [Mespnow](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/mespnow/index.html): adds to ESP-NOW the retransmission filter, Cyclic Redundancy Check (CRC), and data fragmentation features

    - Mcommon: modules shared by all ESP-MDF components
        - Event loop: deals with ESP-MDF events
        - Error Check: manages ESP-MDF's code errors

- **Components**:
    - [Mconfig](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html): network configuration module
    - [Mupgrade](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mupgrade.html): upgrade module
    - Mdebug: debugging module
    - [Mlink](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mlink.html): LAN control module

- **Examples**:
    - Function demo: demonstrates how functions can be used
    - Development Kit: provides ESP32-MeshKit and ESP32-Buddy application demos
    - Solution: offers a routerless solution, as well as the solutions for indoor positioning, street light control, etc.

## Develop with ESP-MDF
### Development Boards
#### ESP32-MeshKit
ESP32-MeshKit offers a complete [ESP-MESH Lighting Solution](https://www.espressif.com/en/products/software/esp-mesh/overview) (see the below figure), complemented by ESP-Mesh App ([iOS version](https://itunes.apple.com/cn/app/esp-mesh/id1420425921?mt=8) and [Android](https://github.com/EspressifApp/Esp32MeshForAndroid/raw/master/release/mesh.apk)) for research, development and better understanding of ESP-MESH.

<table>
        <tr>
            <td ><img src="docs/_static/ESP32-MeshKit_Light.jpg" width="550"><p align=center>ESP32-MeshKit Light</p></td>
            <td ><img src="docs/_static/ESP32-MeshKit_Sense.jpg" width="600"><p align=center>ESP32-MeshKit Sense</p></td>
        </tr>
    </table>

* Products:
    * [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf): The RGBCW smart lights that show control results visually. They can be used to test network configuration time, response speed, stability performance, and measure distance, etc.

    * [ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md): This kit is equipped with a light sensor as well as a temperature & humidity sensor. It can measure power consumption and develop low power applications. The kit may also be used with ESP-Prog for firmware downloading and debugging.

    * ESP32-MeshKit-Button: Serves as an on/off controller, ready for the development of low power applications. It can be used with ESP-Prog for firmware downloading and debugging (Coming soon).

#### ESP32-Buddy
ESP32-Buddy is a development board specifically designed to test the development of ESP-MESH. With its small size and USB power input, the board can be conveniently used for testing a large number of devices and measure distances between them (Coming soon).

* Functions:
    * 16 MB flash: stores logs
    * OLED screen: displays information about the device, such as its layer, connection status, etc.
    * LED: indicates the board's status
    * Temperature & humidity sensor: collects environmental parameters

### Quick Start
This section provides the steps for quick start with your development of ESP-MDF applications. For more details, please refer to [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

The directory ``~/esp`` will be used further to install the compiling toolchain, ESP-MDF and demo programs. You can use another directory, but make sure to modify the commands accordingly.

1. [**Setup Toolchain**](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#get-started-setup-toolchain): please set up according to your PC's operating system ([Windows](http://esp-idf.readthedocs.io/en/latest/get-started/windows-setup.html), [Linux](http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html) or [Mac OS](http://esp-idf.readthedocs.io/en/latest/get-started/macos-setup.html)).

2. **Get ESP-MDF**:
    ```shell
    git clone --recursive https://github.com/espressif/esp-mdf.git
    ```
    If you clone without the `--recursive` option, please navigate to the esp-mdf directory and run the command `git submodule update --init`

3. **Set up ESP-MDF Path**: Toolchain uses the environment variable ``MDF_PATH`` to access ESP-MDF. The setup of this variable is similar to that of the variable ``IDF_PATH``. Please refer to [`Add IDF_PATH to User Profile`](https://esp-idf.readthedocs.io/en/latest/get-started/add-idf_path-to-profile.html).
    ```shell
    export MDF_PATH=~/esp/esp-mdf
    ```

4. **Start a Project**: The word *project* refers to the communication example between two ESP-MESH devices.
    ```shell
    cp -r $MDF_PATH/examples/get-started/ .
    cd  get-started/
    ```

5. **Build and Flash**: Only the serial port number needs to be modified. For the rest, just keep the default configuration untouched.
    ```shell
    make menuconfig
    make erase_flash flash
    ```

6. [**Monitor/Debugging**](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/idf-monitor.html): If you want to exit the monitor, please use the shortcut key ``Ctrl+]``.
    ```shell
    make monitor
    ```

7. **Update ESP-MDF**:
    ```shell
    cd ~/esp/esp-mdf
    git pull
    git submodule update --init --recursive
    ```

## ESP-MESH Highlights

* **Easy setup**: ESP-MESH expands the original Wi-Fi hotspot range to the reach of the most distant node in the mesh cloud. Such a network is automatically formed, self-healing and self-organizing. It saves the efforts of laying cables. All you need to do is configure the router password.

* **Gateway free**: The decentralized structure of ESP-MESH with the absence of a gateway precludes the overall network breakdown if one single node fails. Even if there is a single ESP-MESH device, the network still works as usual.

* **Safer transmission**: Both the data link layer and the application layer can be encrypted.

* **More reliable transmission**: The transmission and data flow control between two devices are more reliable. Also, unicast, multicast and broadcast transmissions are supported.

* **Large network capacity**: ESP-MESH takes the form of a tree topology, so one single device can connect to 10 devices at maximum, and an entire network can have over 1,000 nodes.

* **Wider transmission coverage**: The transmission distance between two devices is 30 m through walls, and 200 m without any obstacles in between (relevant to ESP32-DevKitC).
    * **Smart Home**: Even if there are only three to five devices in your home, they can form a network and communicate with one another through walls.
    * **Street light**: If ESP-MESH is used for the street lighting scenario, two long-distance devices can communicate with each other.
* **High transmission speed**: For Wi-Fi transmission, the speed can reach up to 10 Mbps.
    * **Environment Control System**: Directly transfers the raw data collected by sensors and analyzes mass data for calibration of algorithms, thereby improving sensors' accuracy.
    * **Background Music System**: Both audio and video transmissions are supported.
* **Simultaneously run Wi-Fi and BLE protocol stacks**: ESP32 chips can run both Wi-Fi and BLE protocol stacks side by side and use ESP-MESH as the main network to transmit data, receive BLE probe beacon, send BLE broadcasts and connect BLE devices.
    * **Items tracing**: Monitors the BLE or Wi-Fi data packets from a device at multiple selected spots.
    * **Pedestrian counting**: Through monitoring Wi-Fi probe request frames.
    * **Indoor positioning**: Each device functions as a Beacon AP, continuously sending Bluetooth signal to the surroundings. The network can analyze a device's signal intensity and calculate its current position.
    * **Product promotion**: Sends real-time product information and promotions through iBeacon.
    * **Bluetooth gateway**: With each device serving as a Bluetooth gateway, traditional Bluetooth devices can also be connected to an ESP-MESH network.

## Related Documentation

* For ESP-MDF related documents, please go to [ESP-MDF Programming Guide](https://docs.espressif.com/projects/esp-mdf/en/latest/?badge=latest).
* [ESP-IDF Programming Guide](https://esp-idf.readthedocs.io/en/latest/) describes Espressif's IoT development framework.
* [ESP-MESH](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html) is the basic wireless communication protocol for ESP-MDF.
* To report a bug or request a function, please go to [Issues](https://github.com/espressif/esp-mdf/issues) on GitHub to submit them. Before submitting an issue, please check if it has already been covered.
* If you want to contribute ESP-MDF related codes, please refer to [Code Contribution Guide](docs/en/contribute/index.rst).
* To visit ESP32 official forum, please go to [ESP32 BBS](https://esp32.com/).
* For the hardware documents related to ESP32-MeshKit, please visit [Espressif Website](https://www.espressif.com/en/support/download/documents).
* ESP32-MeshKit-Light purchase link: [Taobao](https://item.taobao.com/item.htm?spm=a230r.1.14.1.55a83647K8jlrh&id=573310711489&ns=1&abbucket=3#detail).
* ESP32-Buddy purchase link: Coming soon.

