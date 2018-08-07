#!/bin/bash
set -e

if [ ! -d "serial_log" ]; then
    mkdir serial_log
fi

if [ ! -d "bin" ]; then
    mkdir bin
fi

echo -e "\033[32m----------------------"
echo "SDK_PATH: $MDF_PATH"
echo -e "----------------------\033[00m"

# make clean
# make -j5
make


# copy bin
echo -e "\033[32m----------------------"
echo "the esptool.py copy to bin "
echo "the $PROJECT_NAME.bin copy to bin "
echo "the bootloader_bin copy to bin"
echo "the partitions.bin copy to bin"
echo "the $PROJECT_NAME.elf copy to bin"
echo -e "----------------------\033[00m"
cp $MDF_PATH/esp-idf/components/esptool_py/esptool/esptool.py bin
cp build/bootloader/bootloader.bin bin
cp build/partitions.bin bin
cp build/*.bin bin
cp build/*.elf bin
cp build/*.map bin
cp $MDF_PATH/tools/idf_monitor.py bin

cd bin

PROJECT_NAME=$(ls *.elf)
PROJECT_NAME=${PROJECT_NAME%%.*}

xtensa-esp32-elf-objdump -S $PROJECT_NAME.elf > $PROJECT_NAME.s &

python esptool.py --chip esp32 --port /dev/ttyUSB$1 erase_flash
python esptool.py --chip esp32 --port /dev/ttyUSB$1 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 bootloader.bin 0x8000 partitions.bin 0xc0000 $PROJECT_NAME.bin

mac=$(python esptool.py -b 115200 -p /dev/ttyUSB$1 read_mac)
mac_str=${mac##*MAC:}
mac_str=${mac_str//:/-}
log_file_name="../serial_log/"${mac_str:1:17}

python idf_monitor.py -b 115200 -p /dev/ttyUSB$1 -sf "${log_file_name}__ttyUSB$1.log" $PROJECT_NAME.elf
