#!/bin/bash

dir=$(realpath .)
src_dir=$dir

for ARCH in arm arm64 x86 x64; do
    build_dir=$src_dir/build/android-$ARCH/debug
    prefix=$dir/bin/android-$ARCH/debug

    mkdir -p $build_dir
    cd $build_dir

    cmake -DCUSTOM_CMAKE=$src_dir/platform/android/custom-android.cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_VERBOSE_MAKEFILE=1 \
        -DCMAKE_INSTALL_PREFIX:PATH=$prefix \
        -DCMAKE_SYSTEM_NAME=Android \
        -DANDROID=1 \
        -DUSE_VCPKG=1 \
        -DCMAKE_TARGET_ARCHITECTURE=${ARCH} \
        $src_dir

    make
    make install
done
