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

echo -e "\033[32m----------------------"
echo "SDK_PATH: $IDF_PATH"
echo -e "----------------------\033[00m"

if [ ! -d "serial_log" ]; then
    mkdir serial_log
else
    rm serial_log/* -rf
fi

device_num=$1
if [ "$device_num" == "" ];then
    device_num=49
fi

echo "============================="
echo " deveice num sum $device_num "
echo "============================="

loop_end=$[ $device_num-1 ]
for i in  $( seq 0 1 $loop_end )
do
    echo "open ttyUSB$i"
    {
        mac=$(python bin/esptool.py -b 115200 -p /dev/ttyUSB$i read_mac)
        mac_str=${mac##*MAC:}
        mac_str=${mac_str//:/-}
        echo $mac_str
        log_file_name="serial_log/"${mac_str:1:17}
        gnome-terminal  --geometry 240x60+$x+$y -x minicom -c on -D /dev/ttyUSB$i -C "${log_file_name}__ttyUSB$i.log"
    }&
done
wait
