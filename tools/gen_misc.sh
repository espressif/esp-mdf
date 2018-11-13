#!/bin/bash
set -e

PROJECT_NAME=$(basename $(pwd))

if [ ! -d "serial_log" ]; then
    mkdir serial_log
fi

if [ ! -d "bin" ]; then
    mkdir bin
fi

export IDF_PATH=$MDF_PATH/esp-idf

echo -e "\033[32m----------------------"
echo "SDK_PATH: $MDF_PATH"
echo -e "----------------------\033[00m"

# copy bin
echo -e "\033[32m----------------------"
echo "the esptool.py copy to bin "
echo "the mesh.bin copy to bin "
echo "the bootloader_bin copy to bin"
echo "the mesh_partitions.bin copy to bin"
echo "the mesh.elf copy to bin"
echo -e "----------------------\033[00m"
cp $MDF_PATH/esp-idf/components/esptool_py/esptool/esptool.py bin
cp build/bootloader/bootloader.bin bin
cp build/*partitions*.bin bin
cp build/*.bin bin
cp build/*.elf bin
cp build/*.map bin
cp $MDF_PATH/tools/idf_monitor.py bin

make erase_flash flash -j5 ESPBAUD=921600 ESPPORT=/dev/ttyUSB$1

cd bin

xtensa-esp32-elf-objdump -S $PROJECT_NAME.elf > $PROJECT_NAME.s &

mac=$(python esptool.py -b 115200 -p /dev/ttyUSB$1 read_mac)
mac_str=${mac##*MAC:}
mac_str=${mac_str//:/-}
log_file_name="../serial_log/"${mac_str:1:17}

python idf_monitor.py -b 115200 -p /dev/ttyUSB$1 -sf "${log_file_name}__ttyUSB$1.log" $PROJECT_NAME.elf
