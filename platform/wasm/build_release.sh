#!/bin/bash

. /emsdk/emsdk_env.sh

dir=$(realpath .)
src_dir=$dir
build_dir=$src_dir/build/wasm/release
prefix=$dir/bin/wasm/release

mkdir -p $build_dir
cd $build_dir

cmake -DCUSTOM_CMAKE=$src_dir/platform/wasm/Custom-wasm.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_VERBOSE_MAKEFILE=1 \
    -DCMAKE_INSTALL_PREFIX:PATH=$prefix \
    -DCMAKE_SYSTEM_NAME=Emscripten \
    -DUSE_VCPKG=1 \
    -DEMSDK=$EMSDK \
    -DEMSDK_NODE=$EMSDK_NODE \
    $src_dir

make
make install
