# Examples

This directory contains a range of example ESP-MDF projects. These are intended to demonstrate parts of ESP-MDF functionality, and to provide code that you can copy and adapt into your own projects.

# Example Layout

The examples are grouped into subdirectories by category. Each category directory contains one or more example projects:

* `get_started`: contains some very simple examples with minimal functionality.
* `function_demo`: demonstrates how functions can be used.
* `development_kit`: provides ESP32-MeshKit and ESP32-Buddy application demos.
* `solution`: offers a routerless solution, as well as the solutions for indoor positioning, street light control, etc.

# Using Examples

Building an example is the same as building any other project:

* Follow the Getting Started instructions which include building the "get_started" example.
* Change into the directory of the new example you'd like to build.
* `make menuconfig` to configure the example. Most examples have a project-specific "Example Configuration" section here (for example, to set the WiFi ssid & password to use).
* `make` to build the example.
* Follow the printed instructions to flash, or run `make flash`.

# Copying Examples

Each example is a standalone project. The examples *do not have to be inside the esp-idf directory*. You can copy an example directory to anywhere on your computer in order to make a copy that you can modify and work with.

The `MDF_PATH` environment variable is the only thing that connects the example to the rest of ESP-MDF.

If you're looking for a more bare-bones project to start from, try [esp-idf-template](https://github.com/espressif/esp-idf-template).

# Contributing Examples

If you have a new example you think we'd like, please consider sending it to us as a Pull Request.

In the ESP-MDF documentation, you can find a "Creating Examples" page which lays out the steps to creating a top quality example.
