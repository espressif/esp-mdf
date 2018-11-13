Mwifi API
==========

``Mwifi`` (Wi-Fi Mesh) is the encapsulation of `ESP-MESH APIs <https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/mesh/index.html>`_, and it adds to ESP-MESH the retransmission filter, data compression, fragmented transmission, and P2P multicast features.

.. ---------------------- Mwifi Features --------------------------

.. _Mwif-Features:

Features
---------

1. **Retransmission filter**: As ESP-MESH won't perform data flow control when transmitting data downstream, there will be redundant fragments in case of unstable network or wireless interference. For this, Mwifi adds a 16-bit ID to each fragment, and the redundant fragments with the same ID will be discarded.
2. **Fragmented transmission**: When the data packet exceeds the limit of the maximum packet size, Mwifi splits it into fragments before they are transmitted to the target device for reassembly.
3. **Data compression**: When the packet of data is in Json and other similar formats, this feature can help reduce the packet size and therefore increase the packet transmitting speed. 
4. **P2P multicast**: As the multicasting in ESP-MESH may cause packet loss, Mwifi uses a P2P (peer-to-peer) multicasting method to ensure a much more reliable delivery of data packets.

.. ---------------------- Writing a Mesh Application --------------------------

.. _Mwifi-writing-mesh-application:

Writing Applications
--------------------

Mwifi supports using ESP-MESH under self-organized mode, where only the root node that serves as a gateway of Mesh network can require the LwIP stack to transmit/receive data to/from an external IP network.

If you want to call Wi-Fi API to connect/disconnect/scan, please pause Mwifi first.


.. _mesh-initialize-Mwifi:

Initialization
^^^^^^^^^^^^^^^

Wi-Fi Initialization
""""""""""""""""""""
Prior to the use of Mwifi, Wi-Fi must be initialized.
``esp_wifi_set_ps`` (indicates whether Mesh network enters into sleep mode) and ``esp_mesh_set_6m_rate`` (indicates the minimum packet transmittig speed) must be set before ``esp_wifi_start``.


The following code snippet demonstrates how to initialize Wi-Fi.

.. code-block:: c

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    MDF_ERROR_ASSERT(nvs_flash_init());

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

Mwifi Initialization
"""""""""""""""""""""
In general, you don't need to modify Mwifi and just use its default configuration. But if you want to modify it, you may use ``make menuconfig``. See the detailed configuration below:：

1. ROOT

    ======================  ====================
    Parameters              Description
    ======================  ====================
    vote_percentage         Vote percentage threshold above which the node becoms a root
    vote_max_count          Max multiple voting each device can have for the self-healing of a MESH network
    int8_t backoff_rssi     RSSI threshold below which connections to the root node are not allowed
    scan_min_count          The minimum number of times a device should scan the beacon frames from other devices before it becomes a root node
    scan_fail_count         Max fails (60 by default) for a parent node to restore connection to the MESH network before it breaks the connection with its leaf nodes
    monitor_ie_count        Allowed number of changes a parent node can introduce into its information element (IE), before the leaf nodes must update their own IEs accordingly
    root_healing_ms         Time lag between the moment a root node is disconnected from the network and the moment the devices start electing another root node
    root_conflicts_enable   Allow more than one root in one network
    fix_root_enalble        Enable a device to be set as a fixed and irreplaceable root node
    ======================  ====================


2. Capacity
    ======================  ====================
    Parameters              Description
    ======================  ====================
    capacity_num            Network capacity, defining max number of devices allowed in the MESH network
    max_layer               Max number of allowed layers
    max_connection          Max number of MESH softAP connections
    ======================  ====================

3. Stability
    ====================  ====================
    Parameters            Description
    ====================  ====================
    assoc_expire_ms       Period of time after which a MESH softAP breaks its association with inactive leaf nodes
    beacon_interval_ms    Mesh softAP beacon interval
    passive_scan_ms       Mesh station passive scan duration
    monitor_duration_ms   Period (ms) for monitoring the parent's RSSI. If the signal stays weak throughout the period,
                          the node will find another parent offering more stable connection
    cnx_rssi              RSSI threshold above which the connection with a parent is considered strong
    select_rssi           RSSI threshold for parent selection. Its value should be greater than SWITCH_RSSI
    switch_rssi           RSSI threshold below which a node selects a parent with better RSSI
    ====================  ====================

4. Transmission
    ==================  ====================
    Parameters          Description
    ==================  ====================
    xon_qsize           Number of MESH buffer queues
    retransmit_enable   Enable retransmission on mesh stack
    data_drop_enable    If a root is changed, enable the new root to drop the previous packet
    ==================  ====================

The following code snippet demonstrates how to initialize Mwifi.

.. code-block:: c

    /*  Mwifi initialization */
    wifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_ASSERT(mwifi_init(&cfg));


.. _mesh-configuring-Mwifi:

Configuration
^^^^^^^^^^^^^
The modified Mwifi configuration will only be effective after Mwifi is rebooted.

You may configure Mwifi by using the struct mwifi_config_t. See the configuration parameters below:

    ==================  ====================
    Parameters          Description
    ==================  ====================
    ssid                SSID of the router
    password            Router password
    bssid               BSSID is equal to the router's MAC address. This field must be
                        configured if more than one router shares the same SSID. You
                        can avoid using BSSIDs by setting up a unique SSID for each
                        router. This field must also be configured if the router is hidden

    mesh_id             Mesh network ID. Nodes sharing the same MESH ID
                        can communicate with one another
    mesh_password       Password for secure communication between devices in a MESH network
    mesh_type           Only MESH_IDLE, MESH_ROOT, and MESH_NODE device types are supported.
                        MESH_ROOT and MESH_NODE are only used for routerless solutions
    channel             Mesh network and the router will be on the same channel
    ==================  ====================


The following code snippet demonstrates how to configure Mwifi.

.. code-block:: c

    mwifi_config_t config = {
        .ssid          = CONFIG_ROUTER_SSID,
        .password      = CONFIG_ROUTER_PASSWORD,
        .channel       = CONFIG_ROUTER_CHANNEL,
        .mesh_id       = CONFIG_MESH_ID,
        .mesh_password = CONFIG_MESH_PASSWORD,
        .mesh_type     = CONFIG_MESH_TYPE,
    };

    MDF_ERROR_ASSERT(mwifi_set_config(&config));


.. _mesh-start-Mwifi:

Start Mwifi
^^^^^^^^^^^
After starting Mwifi, the application should check for Mwifi events to determine when it has connected to the network.

1. MDF_EVENT_MWIFI_ROOT_GOT_IP: the root node gets the IP address and can communicate with the external network;
2. MDF_EVENT_MWIFI_PARENT_CONNECTED: connects with the parent node for data communication between devices.

.. code-block:: c

    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(mwifi_start());

.. --------------------- Mwifi Application Examples ------------------------

.. _mwifi-application-examples:

Application Examples
--------------------

For ESP-MDF examples, please refer to the directory :example:`function_demo/mwifi`, which includes:

   * Connect to the external network: This can be achieved by the root node via MQTT and HTTP.
   * Routerless: It involves a fixed root node which communicates with the external devices via UART.
   * Channel switching: When the router channel changes, Mesh switches its channel accordingly.
   * Low power consumption: The leaf nodes stop functioning as softAP and enter into sleep mode.

.. ------------------------- Mwifi API Reference ---------------------------

.. _mwifi-api-reference:

API Reference
--------------

.. include:: /_build/inc/mwifi.inc
