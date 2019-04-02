# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

execute_process(COMMAND bash -c "cd $ENV{MDF_PATH} && git describe --always --tags --dirty"
                OUTPUT_VARIABLE MDF_VER 
                OUTPUT_STRIP_TRAILING_WHITESPACE)

set($ENV{MDF_VER} ${MDF_VER})
add_definitions(-DMDF_VER=\"${MDF_VER}\")

# Add the ESP-MDF components
set(EXTRA_COMPONENT_DIRS $ENV{MDF_PATH}/components/ $ENV{MDF_PATH}/components/third_party/)

# Set sysroot
set(CMAKE_SYSROOT $ENV{IDF_PATH}/../xtensa-esp32-elf/xtensa-esp32-elf)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

include_directories(
    $ENV{IDF_PATH}/../xtensa-esp32-elf/xtensa-esp32-elf/include/c++/5.2.0
    $ENV{IDF_PATH}/../xtensa-esp32-elf/xtensa-esp32-elf/include/c++/5.2.0/xtensa-esp32-elf
    $ENV{IDF_PATH}/../xtensa-esp32-elf/xtensa-esp32-elf/include/c++/5.2.0/backward
    $ENV{IDF_PATH}/../xtensa-esp32-elf/lib/gcc/xtensa-esp32-elf/5.2.0/include
    $ENV{IDF_PATH}/../xtensa-esp32-elf/lib/gcc/xtensa-esp32-elf/5.2.0/include-fixed
    $ENV{IDF_PATH}/../xtensa-esp32-elf/xtensa-esp32-elf/include
    )

project(get-started-cmake)
