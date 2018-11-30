ESP-MESH 基本概念困惑解答
===========================

本文用于回答初次接触 ESP-MESH 时产生的疑惑。

准备工作
---------

ESP-MESH 是特为 ESP32 芯片研发的自组网技术。ESP32 使用基于双核的修定版本的 FreeRTOS，并使用 ESP-IDF 作为其官方开发框架。

1. 了解 `ESP-MESH 网络协议 <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html>`_
2. 了解 ESP32 芯片:
    * `ESP32 技术参考手册 <https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_cn.pdf>`_
    * `ESP32 技术规格书 <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_cn.pdf>`_
3. 熟悉 `ESP-IDF 开发指南 <https://docs.espressif.com/projects/esp-idf/zh_CN/latest/index.html>`_ 
4. 了解 `FreeRTOS <https://www.freertos.org/>`_

常见问题
--------

ESP-MESH 根节点相关
^^^^^^^^^^^^^^^^^^^^
1. 根节点是什么意思？如何生成？

    根节点是一个 ESP-MESH 网络内唯一与外界（路由器或服务器）连接的节点，其产生方式可以有多种：

    - **通过底层协议自行选举产生**，具体流程请参考 ESP-MESH `组网流程说明 <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-building-a-network>`_。此方式下，需要手机或服务器告诉设备路由器的信息，作为选举的依据；
    - **通过应用层设计固定某一个或某一类设备作为根节点**，此时周围的设备可以自动连上该类设备，此种情况即不需要路由或手机参与。但需注意，若多个设备都固定为根节点，则相互之间无法组网

2. 根节点挂了，网络是不是就崩了？

    分为两种情况：

    - **若网络为非固定根节点方案**，则网络内设备会在确认根节点无法链接的情况下，重新选出新的根节点，具体流程请参考 `Root Node Failure <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/mesh.html#mesh-managing-a-network>`_。
    - **若网络为固定根节点方案**，在用户重新指派出新的根节点之前，网络无法正常工作。

ESP-MESH 组网相关
^^^^^^^^^^^^^^^^^^

1. 组网是否可以不需要路由器？

    ESP-MESH 组网不依赖于路由器，只要确认了根节点即可。

2. 设备组网需要多久？

    设备组网需要的时间与设备的数量、相互之间的距离（相互之间的 Wi-Fi 信号强弱）、设备分布方式等因素有关系，数量越少、Wi-Fi 信号越强、分布越均匀，设备组网越快。

3. 一个路由器下，是否只能有一个 MESH 网络？

    分以下几种情形讨论：

    - 路由范围内的设备相互之间能通讯（直接或间接），则：
        - 若 MESH ID 不同，则以 MESH ID 为标识组成不同的 MESH 网络。
        - 若 MESH ID 相同，则判断是否允许根节点冲突：
            - **若允许根节点冲突**，但当网络不稳定导致生成其他根节点时，则不会进行排除操作，最终允许多个相同 ID 的 MESH 网络，每个网络各有一个根节点；
            - **若不允许根节点冲突**，但当网络不稳定导致生成其他根节点时，则会进行排除操作，最终只允许一个相同 ID 的 MESH 网络，该网络只有一个根节点；
    - 路由范围内的设备相互之间无法通讯（直接或间接），则：
    - 由于设备不知道其他网络的存在，则会单独形成各自的网络。

4. 一个 MESH 网络内，是否只能有一个根节点？

    是的，一个 MESH 网络内只能有一个根节点。


5. 假设有两个相同 MESH ID 的 MESH 网络，则新加入一个设备，会加到哪个网络？

    若不指定连接的父节点，则根据信号强度跟信号更强的节点产生关联。

ESP-MESH 性能相关
^^^^^^^^^^^^^^^^^^

1. 网络环境会不会影响 ESP-MESH 性能？

    ESP-MESH 是基于 Wi-Fi 通讯协议的方案，因此任何会影响 Wi-Fi 信号强弱的因素都会影响 ESP-MESH 的性能。使用时建议选择较为空闲的信道，使用性能更好的天线等来提高 Wi-Fi 信号强度。

2. 设备支持多远距离的通讯？

    通讯距离取决于 Wi-Fi 信号强度和数据吞吐量需求，两个设备间信号越好，吞吐量要求越低，则通信距离越大。

其余
^^^^^^

1. ESP-MDF 是什么，和 ESP-MESH 有什么关系或区别？

    MDF，Mesh Development Framework，是基于 ESP-MESH 的开发框架。ESP-MESH 是 ESP-IDF 中的 MESH 网络通信协议模块。在 ESP-MESH 组网、数据传输的基本功能上，添加了配网模块、OTA 模块、连云功能、本地控制、设备驱动等模块和应用示例，方便用户进行开发。用户可以根据需求选择基于 IDF 或 MDF 进行开发。

2. ESP32 芯片能读出来多个 Mac 地址，请问 ESP-MESH 通讯是以哪个 Mac 地址

    ESP32 芯片有 STA、AP、BLE、Ethernet 4 个 Mac 地址，其依次 +1，即 Ethernet Mac= BLE Mac+1 = AP Mac+2 = STA Mac+3，例如假设：
    `xx:xx:xx:xx:xx:00` 为 STA Mac，则
    `xx:xx:xx:xx:xx:01` 为 AP
    `xx:xx:xx:xx:xx:02` 为 BLE
    `xx:xx:xx:xx:xx:03` 为 Ethernet。
    
    在 ESP-MESH 通讯时，以设备的 STA 地址为目标通讯。

3. 没有路由器，ESP-MESH 能不能正常工作？

    ESP-MESH 网络形成后，网络内部的通讯不依赖路由器，可以正常工作。

问题反馈
---------

1. 可以通过以下途径进行问题反馈：
    * 乐鑫 `ESP-MDF 论坛页面 <https://www.esp32.com/viewforum.php?f=21>`_
    * Github ESP-MDF `问题反馈页面 <https://github.com/espressif/esp-mdf/issues>`_

2. 进行问题反馈时，请提供以下信息：
    * **硬件**：使用的开发板型号
    * **错误描述**：问题复现的步骤、条件和出现的概率
    * **ESP-MDF 版本信息**：使用 ``git commit`` 获取 ESP-MDF 的版本信息
    * **日志**：设备完整的日志文件及 ``build`` 文件夹下的 elf 文件