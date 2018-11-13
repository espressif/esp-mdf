Basics and Common Questions About ESP-MESH
===============================================

This document is intended to facilitate new users get started with ESP-MESH.

Get Started
------------

1. About ESP32 Chips:
    * `ESP32 Technical Reference Manual <https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf>`_
    * `ESP32 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>`_
2. About `ESP-IDF Programming Guide <https://docs.espressif.com/projects/esp-idf/en/latest/>`_
3. About FreeRTOS: ESP32 uses the modified version of dual-core based `FreeRTOS <https://www.freertos.org/>`_

Frequently Asked Questions
----------------------------

Root Node
^^^^^^^^^^^^^^^^^^^^
1. What is a root node? How is it generated?

    The root node is a top node that serves as the only interface between the ESP-MESH network and an external IP network (by connecting to a router or a server). There are two ways to generate a root node:

    - Automatic root node election via the physical layer protocol. For details, please refer to ESP-MESH `General Process of Building a Network <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-building-a-network>`_. For this method, the mobile phone or the server needs to inform the devices about the router information that will be used as a reference for them to elect a root node.
    - User designated root node on the application layer: you may designate a specific device or a certain type of device as a fixed root node, and other devices nearby can automatically connect with it without any assistance from the router or the mobile phone. Please note that two or more devices becoming fixed root nodes will lead to the building of multiple networks, and any two of them cannot be built into one network.   

2. Will the network crash when the root node breaks down?

    - In case of the network without a fixed root node: the devices within the network will re-elect a new root node after they confirm the old one is broken. For details, please refer to `Root Node Failure <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-managing-a-network>`_.
    - In case of the network with a fixed root node: the network will get back to normal after users re-designate a new fixed root node. 

Network Configuration
^^^^^^^^^^^^^^^^^^^^^^^

1. Do we need a router for network configuration? 

    ESP-MESH network configuration is independent of routers if there already is a root node.

2. How long will the network configuration take?

    It depends on many factors, such as the number of the devices to be configured, the distance between the devices (i.e. Wi-Fi signal strength), device layout, etc. To be specific, the fewer the devices are, or the stronger the Wi-Fi signal is, or the more evenly scattered the devices are, the less configuration time it will take.

3. Is only one MESH network allowed for one router?

    It depends on the following scenarios.

    - The devices within the router range can communicate with each other (directly or indirectly) 
        - In case of the different MESH ID, different MESH network will be built accordingly.
        - In case of an identical MESH ID
            - If the root node conflict is allowed, a new root node generated as a result of unstable network will co-exist with the old one. In this case, multiple MESH networks will be built with the same ID, and each network has its own root node.
            - If the root node conflict is not allowed, a new root node generated as a result of unstable network will make the old one a non-root node. In this case, there will always be only one MESH network with the same ID, and it has only one root node.
    - The devices within the router range cannot communicate with each other (directly or indirectly). In this case, each device will build its own network as they cannot detect any other network within this range.

4. Is there only one root node within an ESP-MESH network?

    Yes. There is and should only be one root node within an ESP-MESH network.


5. If two MESH network have the same MESH ID, which network will a new device connect to?

    If there isn't a designated parent node for this new device, it will automatically associate with the node which has the strongest signal.

ESP-MESH Performance
^^^^^^^^^^^^^^^^^^^^^^^

1. Will the surroundings affect ESP-MESH performance?

    As ESP-MESH is a solution based on Wi-Fi communication protocol, its performance will be affected by any factors that interfere with Wi-Fi signal strength. It is recommended to use an idle channel and an antenna with better performance that can increase Wi-Fi signal strength.

2. How far is the distance supported for the communication between two devices? 

    The communication distance depends on Wi-Fi signal strength and bandwidth demand. With stronger signal strength and less demand, the communication distance will be much farther.

Miscellaneous
^^^^^^^^^^^^^^^^

1. What is ESP-MDF? What's its relation with ESP-MESH and how is it different from ESP-MESH?

    MDF, or Mesh Development Framework, is a development framework built around the ESP-MESH protocol in IDF. In addition to the basic functions of ESP-MESH network configuration and data transmission, it provides the modules and application examples related to network configuration, OTA, Cloud connect, local control, device driver, etc. It's very convenient for users to develop based on IDF or MDF as they desire.

2. Since an ESP32 chip can read various Mac addresses, which one should be used for ESP-MESH communication?

    An ESP32 chip has four Mac addresses for STA, AP, BLE and LAN respectively. Their address values are incremented by one, i.e. LAN Mac = BLE Mac+1 = AP Mac+2 = STA Mac+3. For example:
    Given that `xx:xx:xx:xx:xx:00` is STA Mac,
    then `xx:xx:xx:xx:xx:01` is AP Mac, 
    `xx:xx:xx:xx:xx:02` is BLE Mac, and
    `xx:xx:xx:xx:xx:03` is LAN Mac.
    The device's STA address is used For ESP-MESH communication. 

3. Will ESP-MESH work as usual without a router?

    Once ESP-MESH network is formed, the network will work as usual because the communication within the network is independent of routers.

Feedback
---------

1. You may offer feedback on the following platforms:
    * Espressif `ESP-MDF Forum <https://www.esp32.com/viewforum.php?f=21>`_
    * Github ESP-MDF `Issues <https://github.com/espressif/esp-mdf/issues>`_

2. When offering the feedback, please provide the following information:
    * Hardware: development board type 
    * Error description: the steps and conditions to reproduce the issue, as well as the issue's probabilities 
    * Version: use ``git commit`` in ESP-MDF to acquire the version information
    * Log: the completed log files about the devices and the elf files under ``build`` folder