#!/usr/bin/env bash

BUILD_DIR=$1
BUILD_TARGET=$2

die() {
    echo "${1:-"Unknown Error"}"
    exit 1
}

[ -z ${MDF_PATH} ] && die "MDF_PATH is not set"

CURRENT_DIR=`pwd`
cd ${BUILD_DIR}
idf.py set-target ${BUILD_TARGET}
idf.py build
cd ${CURRENT_DIR}