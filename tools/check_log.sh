#!/bin/bash

param=$1

if [ "$param" = "crash" ] ; then
    grep -n "Backtrace:" $MDF_PATHserial_log/*.log
elif [ "$param" = "reboot" ] ; then
    grep -n "boot:" $MDF_PATH/serial_log/*.log | grep "2nd stage bootloader"
elif [ "$param" = "wdt" ] ; then
    grep -n "watchdog" $MDF_PATH/serial_log/*.log
elif [ "$param" = "heap" ] ; then
    grep -n "Heap summary for capabilities" ./serial_log/*.log
elif [ "$param" = "clr" ] ; then
    for file in $MDF_PATH/serial_log/*.log
    do
        echo "clear log content ..." > $file
    done
else
    echo -e "\033[32m-------------------help info-----------------"
    echo " paramters:"
    echo "   'crash': detect whether devices crash"
    echo "  'reboot': detect whether devices reboot"
    echo "     'wdt': detect whether watch dog triggered"
    echo "    'heap': detect whether devices malloc error"
    echo "     'clr': clear log files in directory ./serial_log/*"
    echo -e "---------------------------------------------\033[00m"
fi

