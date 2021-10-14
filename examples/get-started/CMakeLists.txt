# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED $ENV{MDF_PATH})
    set(ENV{MDF_PATH} ${CMAKE_CURRENT_LIST_DIR}/../..)
endif()
include($ENV{MDF_PATH}/project.cmake)

project(get_started)
