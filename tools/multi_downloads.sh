#!/bin/bash
set -e
dirt=`pwd | grep 'tools' | wc -l`
if [ "$dirt" -gt 0 ];then
    cd ..
fi

ps_out=`ps -ef | grep minicom | grep -v 'grep' | wc -l`
if [ "$ps_out" -gt 0 ];then
    echo "close minicom"
    pkill minicom
    sleep 4
fi

if [ ! -d "serial_log" ]; then
    mkdir serial_log
fi

if [ ! -d "bin" ]; then
    mkdir bin
fi

# make clean
make -j8

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
cp build/partitions.bin bin
cp build/*.bin bin
cp build/*.elf bin
cp build/*.map bin
cp $MDF_PATH/tools/idf_monitor.py bin

cd bin

PROJECT_NAME=$(ls *.elf)
PROJECT_NAME=${PROJECT_NAME%%.*}

xtensa-esp32-elf-objdump -S $PROJECT_NAME.elf > $PROJECT_NAME.s &

device_num=$1
if [ "$device_num" == "" ];then
    device_num=49
fi

loop_end=$[$device_num-1]

echo "============================="
echo "deveice num sum $device_num"
echo "============================="
for i in $(seq 0 1 $loop_end )
do
{
    echo "download ttyUSB$i"
    {
        make erase_flash flash monitor -j5 ESPPORT=/dev/ttyUSB$i ESPBAUD=921600
    }>/dev/null
}&
done
wait
