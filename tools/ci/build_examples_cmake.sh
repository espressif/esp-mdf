#!/usr/bin/env bash

BUILD_DIR=$1

die() {
    echo "${1:-"Unknown Error"}"
    exit 1
}

[ -z ${MDF_PATH} ] && die "MDF_PATH is not set"

CURRENT_DIR=`pwd`
cd ${BUILD_DIR}
idf.py build
[ $? != 0 ] && die "idf.py build failed"
cd ${CURRENT_DIR}