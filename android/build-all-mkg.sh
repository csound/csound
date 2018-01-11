#!/bin/bash
clear
echo "Beginning NDK build for Csound for Android..."
if [ -z "$ANDROID_STUDIO_HOME" ]; then
    echo "ERROR: ANDROID_STUDIO_HOME is not set. Please set this variable to point to the root directory of your Android Studio installation to continue.";
    exit;
fi
if [ -z "$CSOUND_HOME" ]; then
    echo "ERROR: CSOUND_HOME is not set. Please set this variable to point to the root directory of your Csound git repository to continue.";
    exit;
fi
if [ -z "$NDK" ]; then
    echo "ERROR: NDK is not set. Please set this variable to point to the root directory of your Android Native Development Kit installation to continue.";
    exit;
fi

export ANDROID_NDK_ROOT=$NDK

if [ "$1" = "clean" ]
then
    echo "Cleaning up shared libraries..."
    find . -name "*.so" -delete
    echo "Cleaning up static libraries..."
    find . -name "*.a" -delete
    echo "Cleaning up object files..."
    find . -name "*.o" -delete
    exit
fi

echo "Building ableton_link_opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/ableton_link_opcodes
$NDK/ndk-build $1

echo "Building luajit-2.1..."
cd ${CSOUND_HOME}/android/pluginlibs/luajit-2.0
# The luajit library can't be compiled with the clang NDK, so we cross-compile using gcc.
# This requires the gcc-arm-linux-gnueabi package.
# We have to turn large file support OFF.
make HOST_CC="gcc -m32" BUILD_MODE=static CROSS=arm-linux-gnueabi- TARGET_CFLAGS="-mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -D_FILE_OFFSET_BITS=32"
# Make certain that LuaCsound links only with the static LuaJIT library.
rm -f src/libluajit.so

echo "Building Oboe library..."
cd ${CSOUND_HOME}/android/pluginlibs/android-audio-high-performance/oboe
pwd
# PLEASE NOTE: Must use Android Studio's gradle!
bash ${ANDROID_STUDIO_HOME}/gradle/gradle-4.1/bin/gradle assemble

echo "Building LuaCsound opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/LuaCsound
$NDK/ndk-build $1

echo "Building OSC library..."
cd ${CSOUND_HOME}/android/pluginlibs/liblo-android
$NDK/ndk-build $1

echo "Building OSC opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/libOSC
$NDK/ndk-build $1

echo "Building STK opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/stk-csound
mkdir -p ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf ../stk/rawwaves/*.raw ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK/ndk-build $1

echo "Building stdutil library..."
cd ${CSOUND_HOME}/android/pluginlibs/libstdutil
$NDK/ndk-build $1

echo "Building Doppler opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/doppler
$NDK/ndk-build $1

echo "Building FluidSynth library..."
cd ${CSOUND_HOME}/android/pluginlibs/fluidsynth-android
$NDK/ndk-build $1

echo "Building FluidSynth opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/libfluidsynth
$NDK/ndk-build $1

echo "Building scanned synthesis opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/libscansyn
$NDK/ndk-build $1

echo "Building signal flow graph opcodes..."
cd ${CSOUND_HOME}/android/pluginlibs/signalflowgraph
$NDK/ndk-build $1

echo "Building CsoundAndroid library..."
cd ${CSOUND_HOME}/android/CsoundAndroid
./build.sh $1
cd ..

echo "Installing binaries for the Csound for Android app..."
rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/
rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cd ${CSOUND_HOME}/android/CsoundForAndroid/CsoundApplication
./install_libs.sh

echo "Finished NDK build for Csound for Android."



