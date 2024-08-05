#!/bin/bash

$(eval $BASE_DIR/ioscross/ioscross_conf.sh)

dir=$(realpath .)
src_dir=$dir
arch=arm64

build_dir=$src_dir/build/ioscross/debug
prefix=$dir/bin/ioscross/debug

mkdir -p $build_dir
cd $build_dir

cmake -DCUSTOM_CMAKE=$src_dir/platform/ioscross/custom-ios.cmake \
    -DIOS=1 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=1 \
    -DCMAKE_INSTALL_PREFIX:PATH=$prefix \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DUSE_VCPKG=1 \
    -DOSXCROSS_TARGET_DIR=${OSXCROSS_TARGET_DIR} \
    -DOSXCROSS_SDK=${OSXCROSS_SDK} \
    -DOSXCROSS_TARGET=${OSXCROSS_TARGET} \
    -Darch=${arch} \
    -D CMAKE_OSX_ARCHITECTURES=${arch} \
    $src_dir

make
make install
