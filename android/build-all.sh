#!/bin/bash
#!/bin/bash
clear
echo "Beginning NDK build for Csound for Android..."

if [ -z "$ANDROID_STUDIO_HOME" ]; then
    echo "ERROR: ANDROID_STUDIO_HOME is not set. Please set this variable to point to the root directory of your Android Studio installation to continue.";
    exit;
fi
if [ -z "$NDK" ]; then
    echo "ERROR: NDK is not set. Please set this variable to point to the root directory of your Android Native Development Kit installation to continue.";
    exit;
fi

# Store where we are so we don't have to count cd up and cd down.
cd ..
CSOUND_ROOT=$PWD
cd $CSOUND_ROOT/android
echo "CSOUND_ROOT: $CSOUND_ROOT"

MACHINE="$(uname -s)"
case "${MACHINE}" in 
  MINGW*) NDK_BUILD_CMD=$NDK_BUILD_CMD.cmd;;
  *) NDK_BUILD_CMD=$NDK_BUILD_CMD
esac
echo "NDK_BUILD_COMMAND = $NDK_BUILD_CMD"

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

cd $CSOUND_ROOT/android/pluginlibs

echo "Building luajit-2.1..."
cd $CSOUND_ROOT/android/pluginlibs/luajit-2.0
# The luajit library can't be compiled with the clang NDK, so we cross-compile using gcc.
# We have to turn large file support OFF.
# PREFIX and ARM produce directories compatible with ndk-build.
# Build for arm. 
make clean
make HOST_CC="gcc -m32" BUILD_MODE=static CROSS=arm-linux-gnueabi- TARGET_CFLAGS="-mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -D_FILE_OFFSET_BITS=32"
make install PREFIX=`pwd`/jni/local MULTILIB=libs/armeabi-v7a
# Build for arm64.
make clean
make HOST_CC="gcc" BUILD_MODE=static CROSS=aarch64-linux-gnu- TARGET_CFLAGS="-fPIC -D_FILE_OFFSET_BITS=32"
make install PREFIX=`pwd`/jni/local MULTILIB=libs/arm64-v8a
# Make certain that LuaCsound links only with the STATIC LuaJIT library.
find . -name *.so* -delete


echo "Building the Oboe audio driver library..."
cd $CSOUND_ROOT/android/pluginlibs/android-audio-high-performance/oboe
# PLEASE NOTE: Android Studio's gradle muset be used!
bash ${ANDROID_STUDIO_HOME}/gradle/gradle-4.1/bin/gradle assemble

echo "Building ableton_link_opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/ableton_link_opcodes
$NDK_BUILD_CMD $1

echo "Building LuaCsound opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/LuaCsound
$NDK_BUILD_CMD $1

echo "Building OSC library..."
cd $CSOUND_ROOT/android/pluginlibs/liblo-android
$NDK_BUILD_CMD $1

echo "Building OSC opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libOSC
$NDK_BUILD_CMD $1

echo "Building STK opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/stk-csound
mkdir -p $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf $CSOUND_ROOT/android/pluginlibs/stk/rawwaves/*.raw $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK_BUILD_CMD $1

echo "Building stdutil library..."
cd $CSOUND_ROOT/android/pluginlibs/libstdutil
$NDK_BUILD_CMD $1

echo "Building Doppler opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/doppler
$NDK_BUILD_CMD $1

echo "Building FluidSynth library..."
cd $CSOUND_ROOT/android/pluginlibs/fluidsynth-android
$NDK_BUILD_CMD $1

echo "Building FluidSynth opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libfluidsynth
$NDK_BUILD_CMD $1

echo "Building scanned synthesis opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libscansyn
$NDK_BUILD_CMD $1

echo "Building signal flow graph opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/signalflowgraph
$NDK_BUILD_CMD $1

echo "Building CsoundAndroid library..."
cd $CSOUND_ROOT/android/CsoundAndroid
./build.sh $1

echo "Installing binaries for the Csound for Android app..."
cd $CSOUND_ROOT/android
rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/
rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cd CsoundForAndroid/CsoundApplication
./install_libs.sh
cd $CSOUND_ROOT/android

echo "Finished NDK build for Csound for Android."




