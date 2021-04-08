#!/usr/bin/env bash

BUILD_DIR=$1

die() {
    echo "${1:-"Unknown Error"}"
    exit 1
}

[ -z ${MDF_PATH} ] && die "MDF_PATH is not set"

CURRENT_DIR=`pwd`
cd ${BUILD_DIR}
make defconfig
[ $? != 0 ] && die "make defconfig failed"
make -j4
[ $? != 0 ] && die "make -j4 failed"
make print_flash_cmd | tail -n 1 > build/download.config
[ $? != 0 ] && die "make print_flash_cmd | tail -n 1 > build/download.config failed"
cd ${CURRENT_DIR}