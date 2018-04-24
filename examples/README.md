# Examples

This directory contains a growing number of simple example projects for esp-mdf, intended to show basic esp-mdf functionality and help users build their own projects.

# Using Examples

Building examples is the same as building any other project:

* Follow the setup instructions in the corresponding sections of [ESP-MDF Get Started](docs/get-started.md).

* Run the script `setup_toolchain.sh` in Ubuntu to set up the development environment.
* Set `MDF_PATH` environment variable to point to the path to the esp-mdf top-level directory.
* Change into the directory of the example you'd like to build.
* `make menuconfig` to configure the example. Most examples require a simple WiFi SSID & password via this configuration.
* `make` to build the example.
* Follow the printed instructions to flash, or run `make flash`.

# Copying Examples

Each example is a standalone project. The examples *do not have to be inside the esp-mdf directory*. You can copy an example directory to anywhere on your computer in order to make a copy that you can modify and work with.

The `MDF_PATH` environment variable is the only thing that connects the example to the rest of the `esp-mdf` system.

# Contributing Examples

If you have a new example you think we'd like, please consider sending it to us as a Pull Request.

Please read the esp-mdf [Contribute](docs/contribute.md) file which lays out general contribution rules.

In addition, here are some tips for creating good examples:

* A good example is documented and the basic options can be configured.
* A good example does not contain a lot of code. If there is a lot of generic code in the example, consider refactoring that code into a standalone component and then use the component's API in your example.
* Names (of files, functions, variables, etc.) inside examples should be distinguishable from names of other parts of ESP-IDF and ESP-MDF (ideally, use `example` in names.)
* Functions and variables used inside examples should be declared static where possible.
* Examples should demonstrate one distinct thing each. Avoid multi-purposed "demo" examples, split these into multiple examples instead.
* Examples must be licensed under the Apache License 2.0 or (preferably for examples) if possible you can declare the example to be Public Domain / Creative Commons Zero.
