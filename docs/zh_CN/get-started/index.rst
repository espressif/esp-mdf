***********
快速入门
***********
:link_to_translation:`en:[English]`

本文档旨在指导用户创建 ESP-MDF 的软件环境。ESP-MDF 是基于 ESP-IDF 封装的 ESP-MESH 开发构架，因此 ESP-MDF 的软件环境搭建与 ESP-IDF 基本相同，本文将主要阐述 ESP-MDF 与 ESP-IDF 环境区别和注意事项。在您使用 ESP-MDF 开发前，请先详细阅读 ESP-IDF `快速入指南 <https://docs.espressif.com/projects/esp-idf/zh_CN/latest/get-started/index.html>`_。


准备工作
=========

开发 ESP32 应用程序需要准备：

* **路由器**：用于连接外部网络
* **手机**：安装 ESP-MESH 配网 app
* **ESP32 开发板**：运行 ESP-MESH 至少需要两块以上的 ESP32 开发板，以构成一个网络


开发板指南
==========

为了方便 ESP-MESH 的开发和测试，我们专为 ESP-MESH 开发测试而设计的开发板

.. toctree::
    :maxdepth: 1

    ESP32-Buddy <../hw-reference/esp32-buddy>
    ESP32-MeshKit <../hw-reference/esp32-meshkit>


.. _get-started-get-esp-mdf:

获取 ESP-MDF
==============

.. highlight:: bash

工具链（包括用于编译和构建应用程序的程序）安装完后，你还需要 ESP32 相关的 API/库。API/库在 `ESP-MDF 仓库 <https://github.com/espressif/esp-mdf>`_ 中。

.. include:: /_build/inc/git-clone.inc


.. _get-started-setup-path:

设置 ESP-MDF 路径
==================

工具链程序使用环境变量 ``MDF_PATH`` 来访问 ESP-MDF。这个变量应该设置在你的 PC 中，否则工程将不能编译。其设置方法与 ``IDF_PATH`` 相同。


.. _get-started-configure:

配置
======

在终端窗口中，输入 ``cd ~/esp/get-started`` 进入 ``get-started`` 所在目录，然后启动工程配置工具 ``menuconfig``： ::

    cd ~/esp/get-started
    make menuconfig

* ``examples`` 的配置在 ``Example Configuration`` 子菜单下
* 功能模块的配置在 ``Component config`` 中以 ``MDF`` 开头的子菜单下


.. _get-started-build-flash:

编译和烧写
===========

您可以通过 `make menuconfig` 来配置串口，也可以通过直接在命令行上使用 ``ESPPORT`` 和 ``ESPBAUD`` 环境变量来指定串口和波特率::

    make erase_flash flash -j5 monitor ESPBAUD=921600 ESPPORT=/dev/ttyUSB0


.. _get-started-tools:

工具
======

您也可以选择使用 ``tools`` 目录下的脚本来简化开发流程。

``gen_misc.sh`` 进行编译和烧写，它在 ``make monitor`` 基础上加入了时间戳和日志保存::

    cp $MDF_PATH/tools/gen_misc.sh .
    ./gen_misc.sh /dev/ttyUSB0

``multi_downloads.sh`` 和 ``multi_downloads.sh`` 进行批量设备的烧录::

    cp $MDF_PATH/tools/multi_*.sh .
    ./multi_downloads.sh 49
    ./multi_open_serials.sh 49

``format.sh`` 进行代码格式化::

    $MDF_PATH/tools/format.sh .