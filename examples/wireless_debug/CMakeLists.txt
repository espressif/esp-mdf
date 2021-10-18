# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)


if(NOT EXISTS ./components/lcd/Adafruit-GFX-Library)
message(FATAL_ERROR "Adafruit-GFX-Library is not downloaded and run, the following command:")
message(FATAL_ERROR "git clone -b v1.2.3 https://github.com/adafruit/Adafruit-GFX-Library.git components/lcd/Adafruit-GFX-Library")
endif()

if(NOT DEFINED ENV{MDF_PATH})
    set(ENV{MDF_PATH} ${CMAKE_CURRENT_LIST_DIR}/../..)
endif()
include($ENV{MDF_PATH}/project.cmake)

project(wireless_debug)
