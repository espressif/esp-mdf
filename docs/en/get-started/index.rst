*************
Introduction
*************
:link_to_translation:`zh_CN:[中文]`

It is intended to guide users to build a software environment for ESP-MDF. ESP-MDF is a development framework based on ESP-MESH which is encapsulated by ESP-IDF. Therefore, the building of the software environment for ESP-MDF is similar to the building for ESP-IDF. This document focuses on the software environment difference between ESP-MDF and ESP-IDF, as well as provides some related notes. Before developing with ESP-MDF, please read ESP-IDF `Get Started <https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html>`_.


What you need
==============

To develop applications for ESP32 you need:

* **Router**: it is used to connect to the external network
* **Mobile phone**: install an ESP-MESH network configuration app 
* **ESP32 development board**: at least two ESP32 development boards are required to build an ESP-MESH network


Development board guides
=========================

We provide development boards specially designed for the development and testing of ESP-MESH.

.. toctree::
    :maxdepth: 1

    ESP32-Buddy <../hw-reference/esp32-buddy>
    ESP32-MeshKit <../hw-reference/esp32-meshkit>


.. _get-started-get-esp-mdf:

Get ESP-MDF
==============

.. highlight:: bash

Besides the toolchain (that contains programs to compile and build the application), you also need ESP32 specific API / libraries. They are provided by Espressif in `ESP-MDF repository <https://github.com/espressif/esp-mdf>`_.

.. include:: /_build/inc/git-clone.inc


.. _get-started-setup-path:

Set up path to ESP-MDF
=======================

The toolchain progarms access ESP-MDF using ``MDF_PATH`` environment variable. This variable should be set up on your PC, otherwise projects will not build. The setup method is same as ``IDF_PATH``.


.. _get-started-configure:

Configure
===============

In the terminal window, go to the directory of ``get-started`` by typing ``cd ~/esp/get-started``, and then start project configuation utility ``menuconfig``:  ::

    cd ~/esp/get-started
    make menuconfig

* Configure ``examples`` under the submenu ``Example Configuration`` 
* Configure the function modules under the submenu starting with ``MDF`` in ``Component config``


.. _get-started-build-flash:

Build and Flash
=================

You can configure the serial port with `make menuconfig`, or directly use ``ESPPORT`` and ``ESPBAUD`` environment variable on the command line to specify the serial port and baud rate::

    make erase_flash flash -j5 monitor ESPBAUD=921600 ESPPORT=/dev/ttyUSB0

.. _get-started-tools:

Tools
======

You can use the scripts under the ``tool`` directory to simplify your development process.

You can build and flash with ``gen_misc.sh``, which adds timestamp and log saving functions to ``make monitor``::

    cp $MDF_PATH/tools/gen_misc.sh .
    ./gen_misc.sh /dev/ttyUSB0

``multi_downloads.sh`` and ``multi_downloads.sh`` can be used to simultaneously flash multiple devices::

    cp $MDF_PATH/tools/multi_*.sh .
    ./multi_downloads.sh 49
    ./multi_open_serials.sh 49

``format.sh`` can be used to format the codes::

    $MDF_PATH/tools/format.sh .