ESP-MDF Contributions Guide
=============================

:link_to_translation:`zh_CN:[中文]`

This guide introduces how to add code to ESP-MDF. For details, please refer to `Contributions Guide <https://docs.espressif.com/projects/esp-idf/en/latest/contribute/index.html#related-documents>`_.

Add Code
---------

When you add new functions to ESP-MDF, please add the corresponding examples as well to the directory ``examples``. Also, please add two variables ``ARTIFACTS_NAME`` and ``EXAMPLE_PATH`` to the file ``.gitlab-ci.yml``, and test the examples in ``gitlab``. Please refer to the ``get_started`` example below::

    build_example_get_started:
      <<: *build_template
      variables:
        ARTIFACTS_NAME: "get_started"
        EXAMPLE_PATH: "examples/get_started"

Format Code
------------

Make sure to format the code prior to your submission, with either one of the formatting tools for ESP-MDF: `astyle <http://astyle.sourceforge.net/>`_ and `dos2unix <https://waterlan.home.xs4all.nl/dos2unix.html>`_. Please install such tools first. For the installation on Linux, please refer to the command below::

    sudo apt-get install astyle
    sudo apt-get install dos2unix

Run the code formatting command below::

    tools/format.sh new_file.c

Generate API Documentation
---------------------------

For the code comments, please follow the `Doxygen Manual <http://www.doxygen.nl/manual/docblocks.html#specialblock>`_. You may use ``Doxygen`` to automatically generate API documentation according to the following steps::

    sudo apt-get install doxygen
    export MDF_PATH=~/esp/esp-mdf
    cd ~/esp/esp-mdf/docs
    pip install -r requirements.txt
    cd ~/esp/esp-mdf/docs/en
    make html