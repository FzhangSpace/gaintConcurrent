#!/bin/sh

set -x  #跟踪所有执行的命令

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}

rm -rf $BUILD_DIR

mkdir -p $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake $SOURCE_DIR \
    && make $*
