Mespnow API
===========

``Mespnow`` (Mesh ESP-NOW) is the encapsulation of `ESP-NOW <https://docs.espressif.com/projects/esp-idf/zh_CN/latest/api-reference/wifi/esp_now.html>`_ APIs, and it adds to ESP-NOW the retransmission filter, Cyclic Redundancy Check (CRC), and data fragmentation features.

.. ---------------------- Mespnow Features --------------------------

.. _Mespnow-Features:

Features
--------

1. **Retransmission filter**: Mespnow adds a 16-bit ID to each fragment, and the redundant fragments with the same ID will be discarded.
2. **Fragmented transmission**: When the data packet exceeds the limit of the maximum packet size, Mespnow splits it into fragments before they are transmitted to the target device for reassembly.
3. **Cyclic Redundancy Check**: Mespnow implements CRC when it receives the data packet to ensure the packet is transmitted correctly.


.. ---------------------- Writing a Mespnow Application --------------------------

.. _Mespnow-writing-mesh-application:

Writing Applications
--------------------

1. Prior to the use of Mwifi, Wi-Fi must be initialized;
2. Max number of the devices allowed in the network: No more than 20 devices, among which 6 encrypted devices at most, are allowed to be network configured; 
3. Security: `CCMP <https://en.wikipedia.org/wiki/CCMP_(cryptography)>`_ is used for encrpytion with a 16-byte key.

.. --------------------- Mespnow Application Examples ------------------------

.. _mespnow-application-examples:

Application Examples
--------------------

For ESP-MDF examples, please refer to the directory :example:`function_demo/mespnow`, which includes:

   * A simple application that demonstrates how to use Mespnow for the communication between two devices.

.. ------------------------- Mespnow API Reference ---------------------------

.. _mespnow-api-reference:

API Reference
--------------

.. include:: /_build/inc/mespnow.inc
