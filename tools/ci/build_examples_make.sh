#!/usr/bin/env bash

BUILD_DIR=$1

die() {
    echo "${1:-"Unknown Error"}" 1>$2
    exit 1
}

[ -z ${MDF_PATH} ] && die "MDF_PATH is not set"

CURRENT_DIR=`pwd`
cd ${BUILD_DIR}
make defconfig
make -j4
make print_flash_cmd | tail -n 1 > build/download.config
cd ${CURRENT_DIR}