#!/bin/bash
clear
echo "Beginning NDK build for Csound for Android..."
echo
if [ -z "ANDROID_SDK_ROOT" ]; then
    echo "ERROR: ANDROID_SDK_ROOT is not set. Please set this variable to point to the root directory of your Android SDK installation to continue.";
    exit;
fi

if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "ERROR: ANDROID_NDK_ROOT is not set. Please set this variable to point to the root directory of your Android Native Development Kit installation to continue.";
    exit;
fi

if [ -z "NDK_MODULE_PATH" ]; then
    echo "ERROR: NDK_MODULE_PATH is not set. Please set this variable to point to the android/pluginlibs directory of this project to continue.";
    exit;
fi

MACHINE="$(uname -s)"
case "${MACHINE}" in 
  MINGW*) NDK_BUILD_CMD=$ANDROID_NDK_ROOT/ndk-build.cmd;;
  *) NDK_BUILD_CMD="$ANDROID_NDK_ROOT/ndk-build -j6"
esac
echo "NDK_BUILD_COMMAND: $NDK_BUILD_CMD"

if [ "$1" = "clean" ]
then
    echo "Cleaning shared libraries..."
    find . -name "*.so" -delete
    echo "Cleaning static libraries..."
    find . -name "*.a" -delete
    echo "Cleaning object files..."
    find . -name "*.o" -delete
    exit
fi
echo

cd pluginlibs

echo "Building Oboe audio driver library..."
cd oboe-android
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building luajit-2.1..."
cd luajit-2.0
# The luajit library can't be compiled with the clang NDK, so we cross-compile using gcc.
# We have to turn large file support OFF.
# PREFIX and ARM produce directories compatible with ndk-build.
# Build for arm. 

make clean
make HOST_CC="gcc -m32" BUILD_MODE=static CROSS=arm-linux-gnueabi- TARGET_CFLAGS="-mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -D_FILE_OFFSET_BITS=32" -j6
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
make install PREFIX=`pwd`/jni/local MULTILIB=libs/armeabi-v7a
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
# Build for arm64.
make clean
make HOST_CC="gcc" BUILD_MODE=static CROSS=aarch64-linux-gnu- TARGET_CFLAGS="-fPIC -D_FILE_OFFSET_BITS=32" -j6
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
make install PREFIX=`pwd`/jni/local MULTILIB=libs/arm64-v8a
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
# Make certain that LuaCsound links only with the STATIC LuaJIT library.
find . -name *.so* -delete
cd ..

echo "Building ableton_link_opcodes..."
cd ableton_link_opcodes
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building LuaCsound opcodes..."
cd LuaCsound
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building OSC opcodes..."
cd libOSC
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building STK opcodes..."
cd stk-csound
mkdir -p CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf ../stk/rawwaves/*.raw CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building stdutil library..."
cd libstdutil
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building Doppler opcodes..."
cd doppler
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building FluidSynth opcodes..."
cd libfluidsynth
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building scanned synthesis opcodes..."
cd libscansyn
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..

echo "Building signal flow graph opcodes..."
cd signalflowgraph
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi
cd ..
cd ..

echo "Building CsoundAndroid library..."
cd CsoundAndroid
./build.sh $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi 
cd ..

echo "Installing binaries for the Csound for Android app..."
rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/
rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cd CsoundForAndroid/CsoundApplication
./install_libs.sh
cd ..
cd ..

echo "Finished NDK build for Csound for Android."




