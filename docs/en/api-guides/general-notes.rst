ESP-MESH Basic Information and FAQ
===============================================

This document provides reference links to some basic information on ESP-MESH, followed by the FAQ section.

Reference links
---------------------

ESP-MESH is specifically designed for ESP32 chips. These chips use the modified version of dual-core based FreeRTOS and have an official development framework, which is called ESP-IDF.

1. About the ESP-MESH protocol:
    * `ESP-MESH <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html>`_

2. About ESP32 Chips:
    * `ESP32 Technical Reference Manual <https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf>`_
    * `ESP32 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>`_

3. About ESP-IDF:
    * `ESP-IDF Programming Guide <https://docs.espressif.com/projects/esp-idf/en/latest/>`_

4. About FreeRTOS:
    * `FreeRTOS <https://www.freertos.org/>`_

Frequently Asked Questions
----------------------------

Root Node
^^^^^^^^^^^^^^^^^^^^
1. What is a root node? How is it established?

    The root node is the top node. It gets connected to a server or a router and serves as the only interface between the ESP-MESH network in which it is included and an external IP network.

    There are two methods to establish a root node:

    - **Automatic root node election via the physical layer protocol**. This method implies that a mobile phone or a server transmits the router information to devices. They use this information as a reference to elect a root node. For details, please refer to ESP-MESH `General Process of Building a Network <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-building-a-network>`_.
    - **Manual root node selection on the application layer**. You may designate a specific device or a certain type of devices as a fixed root node. Other devices nearby will automatically connect to the designated root node and build a network without any assistance from the router or the mobile phone. Please note that if more than one device is designated as a fixed root node, it will lead to creation of multiple networks.

2. Will the root node failure lead to the network crash?

    - **In case of automatic root node election**: When devices in the network detect that a root node is broken, they will elect a new one. For details, please refer to `Root Node Failure <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-managing-a-network>`_.
    - **In case of manual root node selection**: Network will be down until a new fixed root node is designated. 

Network Configuration
^^^^^^^^^^^^^^^^^^^^^^^

1. Is a router necessary for network configuration? 

    As soon as a root node is established, ESP-MESH network configuration process becomes independent of routers.

2. How much time does the configuration process take?

    It depends on, but not limited to, the following factors:

    - **Number of devices**: The fewer the number of devices to be configured, the shorter the configuration time.
    - **Arrangement of devices**: The more even the distribution of devices, the shorter the configuration time.
    - **Wi-Fi signal strength of devices**: The stronger the Wi-Fi signal from devices, the shorter the configuration time.

3. Is one router allowed to have only one ESP-MESH network?

    The number depends on the following scenarios:

    - The devices within the router range can communicate either directly or indirectly
        - In case of different MESH IDs, different MESH networks will be built.
        - In case of an identical MESH ID:
            - **If a root node conflict is allowed**: Devices experiencing unstable connection with the existing root node can elect their own root node which will co-exist with the old one. In this case, the network will be split into multiple MESH networks sharing the same ID, but having different root nodes.
            - **If a root node conflict is not allowed**: Devices experiencing unstable connection with the existing root node initiate the election of a new root node, making the old one a non-root node. In this case, there will always be only one MESH network with the same ID and with one root node.
    - The devices within the router range cannot communicate either directly or indirectly. In this case, not being able to detect any existing networks within their range, each device will build its own network.

4. Is an ESP-MESH network allowed to have only one root node?

    Yes. An ESP-MESH network must have only one root node.


5. If two MESH networks have the same MESH ID, to which of the two networks will a new device connect?

    If the new device is not assigned a parent node, it will automatically connect to the node with the strongest signal.

ESP-MESH Performance
^^^^^^^^^^^^^^^^^^^^^^^

1. Does the surrounding environment affect ESP-MESH performance?

    ESP-MESH is a solution based on the Wi-Fi communication protocol, so its performance can be affected in the same manner as Wi-Fi's performance. In order to overcome any interferences and boost signal strength, please use a less-crowded channel and a high-performance antenna.

2. How far apart can two devices be from each other to maintain a stable connection on an ESP-MESH network?

    The communication distance depends on the Wi-Fi signal strength and the bandwidth demands. The higher the signal strength and the less the bandwidth demands, the greater the communication distance can be.

Miscellaneous
^^^^^^^^^^^^^^^^

1. What is ESP-MDF? How is it related to ESP-MESH? How is it different from ESP-MESH?

    ESP-MDF (Espressif's Mesh Development Framework) is a development framework built on ESP-MESH - a communication protocol module for MESH networks in ESP-IDF. In addition to its basic functionality of ESP-MESH network configuration and data transmission, ESP-MDF provides the modules and application examples related to network configuration, Over-the-air (OTA) feature, Cloud connect, local control, device driver, etc. All these features provide convenience for developers using the ESP-IDF or ESP-MDF development frameworks.

2. Since an ESP32 chip has various MAC addresses, which one should be used for ESP-MESH communication?

    Each ESP32 chip has MAC addresses for Station (STA), access point (AP), Bluetooth low energy (BLE) and local area network (LAN).

    Their address values are incremented by one, i.e. LAN Mac = BLE Mac + 1 = AP Mac + 2 = STA Mac + 3.

    For example:
    - MAC for STA: `xx:xx:xx:xx:xx:00`
    - MAC for AP: `xx:xx:xx:xx:xx:01`
    - MAC for BLE: `xx:xx:xx:xx:xx:02`
    - MAC for LAN: `xx:xx:xx:xx:xx:03`

    The device's STA address is used For ESP-MESH communication. 

3. Can an ESP-MESH network function without a router?

    Once an ESP-MESH network is formed, it can operate on its own, meaning that the communication within the network is independent of any routers.

Feedback
---------

1. You can provide your feedback on the following platforms:
    * Espressif `ESP-MDF Forum <https://www.esp32.com/viewforum.php?f=21>`_
    * Github ESP-MDF `Issues <https://github.com/espressif/esp-mdf/issues>`_

2. When providing your feedback, please list the following information:
    * **Hardware:** Type of your development board
    * **Error description**: Steps and conditions to reproduce the issue, as well as the issue occurrence probabilities 
    * **ESP-MDF Version**: Use ``git commit`` to acquire the version details of ESP-MDF installed on your computer
    * **Logs**: Complete log files for your devices and the ``.elf`` files from the ``build`` folder