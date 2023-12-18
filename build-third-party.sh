#!/bin/bash

BASE_PATH=$(readlink -f $0)
BASE_DIR=$(dirname $BASE_PATH)

mkdir -p $BASE_DIR/dist/

# Build libsrtp
cd $BASE_DIR/third_party/libsrtp
mkdir -p build && cd build
cmake -DCMAKE_C_FLAGS="-fPIC" -DTEST_APPS=off -DCMAKE_INSTALL_PREFIX=$BASE_DIR/dist ..
make -j4
make install

cd $BASE_DIR/third_party/usrsctp
mkdir -p build && cd build
cmake -DCMAKE_C_FLAGS="-fPIC" -Dsctp_build_programs=off -DCMAKE_INSTALL_PREFIX=$BASE_DIR/dist ..
make -j4
make install

