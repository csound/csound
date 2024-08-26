#!/bin/bash

$(eval /osxcross/tools/osxcross_conf.sh)

dir=$(realpath .)
src_dir=$dir

for ARCH in x86_64 arm64; do
    build_dir=$src_dir/build/osxcross-$ARCH/debug
    prefix=$dir/bin/osxcross-$ARCH/debug

    mkdir -p $build_dir
    cd $build_dir

    cmake -DCUSTOM_CMAKE=$src_dir/platform/osxcross/custom-osx.cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_VERBOSE_MAKEFILE=1 \
        -DCMAKE_INSTALL_PREFIX:PATH=$prefix \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DUSE_VCPKG=1 \
        -DOSXCROSS_SDK=${OSXCROSS_SDK} \
        -DOSXCROSS_TARGET=${OSXCROSS_TARGET} \
        -DCMAKE_OSX_ARCHITECTURES=${ARCH} \
        $src_dir

    make
    make install
done

prefix=$dir/bin/osxcross/debug
prefix_x64=$dir/bin/osxcross-x86_64/debug
prefix_arm64=$dir/bin/osxcross-arm64/debug

mkdir -p $prefix
cp -r $prefix_arm64/* $prefix

for opcode in $(ls $prefix/Library/Frameworks/CsoundLib64.framework/Versions/7.0/Resources/Opcodes64); do
    lipo -create \
        $prefix_x64/Library/Frameworks/CsoundLib64.framework/Versions/7.0/Resources/Opcodes64/$opcode \
        $prefix_arm64/Library/Frameworks/CsoundLib64.framework/Versions/7.0/Resources/Opcodes64/$opcode \
        -output $prefix/Library/Frameworks/CsoundLib64.framework/Versions/7.0/Resources/Opcodes64/$opcode
done

lipo -create \
    $prefix_x64/Library/Frameworks/CsoundLib64.framework/Versions/7.0/CsoundLib64 \
    $prefix_arm64/Library/Frameworks/CsoundLib64.framework/Versions/7.0/CsoundLib64 \
    -output $prefix/Library/Frameworks/CsoundLib64.framework/Versions/7.0/CsoundLib64

lipo -create \
    $prefix_x64/lib/libCsoundLib64.a \
    $prefix_arm64/lib/libCsoundLib64.a \
    -output $prefix/lib/libCsoundLib64.a

for program in $(ls $prefix/bin); do
    lipo -create \
        $prefix_x64/bin/$program \
        $prefix_arm64/bin/$program \
        -output $prefix/bin/$program
done
