#!/bin/sh

if [[ $1 == "clean" ]]
then
    echo "Cleaning up shared libraries..."
    find . -name "*.so" -delete
fi

cd pluginlibs

cd libstdutil
$NDK/ndk-build $1
cd ..
cd doppler
$NDK/ndk-build $1
cd ..
cd libfluidsynth
$NDK/ndk-build $1
cd ..
cd libosc
$NDK/ndk-build $1
cd ..
cd libscansyn
$NDK/ndk-build $1
cd ..
cd libscansyn
$NDK/ndk-build $1
cd ..
cd luajit-2.0
make HOST_CC="gcc -m32" CROSS=$NDKP TARGET_FLAGS="$NDKF $NDKARCH" TARGET_SYS=linux $1
cd ..
cd LuaCsound
$NDK/ndk-build $1
cd ..
cd signalflowgraph
$NDK/ndk-build $1
cd ..

cd ../CsoundAndroid
./build.sh $1
cd ../CSDPlayer
./install_libs.sh
cd ..



