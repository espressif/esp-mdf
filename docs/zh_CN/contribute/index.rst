ESP-MDF 贡献指南
================

:link_to_translation:`en:[English]`

本指南简要说明如何在 ESP-MDF 中添加代码，详细说明参见 `贡献指南 <https://docs.espressif.com/projects/esp-idf/en/latest/contribute/index.html#related-documents>`_。

添加代码
---------

若您在 ESP-MDF 中增加了新的功能，请在 ``examples`` 下增加使用示例，并将示例名称和路径添加到文件 ``.gitlab-ci.yml`` 中，使其能在 ``gitlab`` 测试并运行。请参考如下方式添加::

    build_example_get_started:
      <<: *build_template
      variables:
        ARTIFACTS_NAME: "get_started"
        EXAMPLE_PATH: "examples/get_started"

格式化代码
----------

您在提交代码之前需要对代码进行格式化，ESP-MDF 使用的格式化式工具为：`astyle <http://astyle.sourceforge.net/>`_ 和 `dos2unix <https://waterlan.home.xs4all.nl/dos2unix.html>`_。您需要先安装他们，在 Linux 安装命令如下::

    sudo apt-get install astyle
    sudo apt-get install dos2unix

运行如下代码格式化命令::

    tools/format.sh new_file.c

API 文档生成
-------------

在编写代码注释时，请遵循 `Doxygen 格式 <http://www.doxygen.nl/manual/docblocks.html#specialblock>`_。您可以通过 ``Doxygen`` 按如下步骤自动生成 API 文档::

    sudo apt-get install doxygen
    export MDF_PATH=~/esp/esp-mdf
    cd ~/esp/esp-mdf/docs
    pip install -r requirements.txt
    cd ~/esp/esp-mdf/docs/en
    make html