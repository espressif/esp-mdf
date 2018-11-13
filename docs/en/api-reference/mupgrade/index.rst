Mupgrade API
=============

``Mupgrade`` (Mesh Upgrade) is a solution for simultaneous over-the-air (OTA) upgrading of multiple ESP-MESH devices on the same wireless network by efficient routing of data flows. It features automatic retransmission of failed fragments, data compression, reverting to an earlier version, firmware check, etc.

.. ---------------------- Writing a Mupgrade Application --------------------------

.. _writing-Mupgrade-application:


.. --------------------- Mwifi Application Examples ------------------------

.. _Mupgrade-application-examples:

Application Examples
--------------------

For ESP-MDF examples, please refer to the directory :example:`function_demo/mupgrade`, which includes:

   * Send a request for data to server via AP, in order to complete the upgrading of the overall Mesh network.

.. ------------------------- Mespnow API Reference ---------------------------

.. _mupgrade-api-reference:

API Reference
-------------

.. include:: /_build/inc/mupgrade.inc
