#!/bin/bash
set -e

# install softwares
sudo apt-get install git wget make libncurses-dev flex bison gperf python python-serial

# download toochain
echo "create directory: esp/ in root directory"
mkdir -p ~/esp
cd ~/esp

echo "download toochain fror esp official website"
sys_bit=$(sudo uname --m)
if [ "$sys_bit" = "x86_64" ] ; then
    wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
    tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
elif [ "$sys_bit" = "i686" ] ; then
    wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz
    tar -xzf xtensa-esp32-elf-linux32-1.22.0-80-g6c4433a-5.2.0.tar.gz
else
    echo "please checkout you system, it is neither 32bit nor 64bit"
    exit 0
fi

# configure env_param
echo "configure env_param"
echo 'export PATH=$PATH:$HOME/esp/xtensa-esp32-elf/bin' >> ~/.profile
source ~/.profile
