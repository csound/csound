#!/bin/sh

if [ "$1" = "clean" ]
then
    echo "Cleaning up shared libraries..."
    find . -name "*.so" -delete
    echo "Cleaning up static libraries..."
    find . -name "*.a" -delete
    echo "Cleaning up object files..."
    find . -name "*.o" -delete
fi

cd ${CSOUND_HOME}/android/pluginlibs/ableton_link_opcodes
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/luajit-2.0
# The luajit library can't be compiled with the clang NDK, so we cross-compile using gcc.
# This requires the gcc-arm-linux-gnueabi package.
make clean
# We have to turn large file support OFF.
make HOST_CC="gcc -m32" BUILD_MODE=static CROSS=arm-linux-gnueabi- TARGET_CFLAGS="-mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -D_FILE_OFFSET_BITS=32"
# Make certain that LuaCsound links only with the static LuaJIT library.
rm -f src/libluajit.so

cd ${CSOUND_HOME}/android/pluginlibs/LuaCsound
$NDK/ndk-build clean
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/liblo-android
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libOSC
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/stk-csound
mkdir -p ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf ../stk/rawwaves/*.raw ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libstdutil
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/doppler
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/fluidsynth-android
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libfluidsynth
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/libscansyn
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/pluginlibs/signalflowgraph
$NDK/ndk-build $1

cd ${CSOUND_HOME}/android/CsoundAndroid
./build.sh $1
cd ..

rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/

rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs

cd ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication

./install_libs.sh



