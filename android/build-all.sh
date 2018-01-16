#!/bin/bash
#!/bin/bash
clear
echo "Beginning NDK build for Csound for Android..."
echo
if [ -z "ANDROID_SDK_ROOT" ]; then
    echo "ERROR: ANDROID_SDK_ROOT is not set. Please set this variable to point to the root directory of your Android SDK installation to continue.";
    exit;
fi
#if [ -z "ANDROID_STUDIO_ROOT" ]; then
#    echo "ERROR: ANDROID_STUDIO_ROOT is not set. Please set this variable to point to the root directory of your #Android Studio installation to continue.";
#    exit;
#fi
if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "ERROR: ANDROID_NDK_ROOT is not set. Please set this variable to point to the root directory of your Android Native Development Kit installation to continue.";
    exit;
fi

# Store where we are so we don't have to count cd up and cd down.
cd ..
export CSOUND_ROOT=$PWD
cd $CSOUND_ROOT/android
echo "CSOUND_ROOT:       $CSOUND_ROOT"
export NDK_MODULE_PATH=$CSOUND_ROOT/android/pluginlibs
echo "NDK_MODULE_PATH:   $NDK_MODULE_PATH"
export ANDROID_HOME=$ANDROID_SDK_ROOT
echo "ANDROID_HOME:      $ANDROID_HOME"
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
#echo "Building the Oboe audio driver library..."
#$cd $CSOUND_ROOT/android/pluginlibs/android-audio-high-performance/oboe
## PLEASE NOTE: Android Studio's gradle muset be used!
#bash ${ANDROID_STUDIO_ROOT}/gradle/gradle-4.1/bin/gradle assemble
#if [ $? -eq 0 ]; then
#    echo OK
#else
#    echo FAIL
#    exit
#fi

echo "Building Oboe audio driver library..."
cd $CSOUND_ROOT/android/pluginlibs/oboe-android
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi


echo "Building luajit-2.1..."
cd $CSOUND_ROOT/android/pluginlibs/luajit-2.0
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

echo "Building ableton_link_opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/ableton_link_opcodes
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building LuaCsound opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/LuaCsound
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building OSC opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libOSC
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building STK opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/stk-csound
mkdir -p $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
cp -rf $CSOUND_ROOT/android/pluginlibs/stk/rawwaves/*.raw $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication/src/main/assets/rawwaves/
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building stdutil library..."
cd $CSOUND_ROOT/android/pluginlibs/libstdutil
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building Doppler opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/doppler
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building FluidSynth opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libfluidsynth
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building scanned synthesis opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/libscansyn
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building signal flow graph opcodes..."
cd $CSOUND_ROOT/android/pluginlibs/signalflowgraph
$NDK_BUILD_CMD $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Building CsoundAndroid library..."
cd $CSOUND_ROOT/android/CsoundAndroid
./build.sh $1
if [ $? -eq 0 ]; then
    echo OK
else
    echo FAIL
    exit
fi

echo "Installing binaries for the Csound for Android app..."
cd $CSOUND_ROOT/android
rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/
rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cd $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication
./install_libs.sh
cd $CSOUND_ROOT/android

#echo "Building Csound for Android app..."
#cd $CSOUND_ROOT/android/CsoundForAndroid/CsoundApplication
## PLEASE NOTE: Android Studio's gradle muset be used!
#bash ${ANDROID_STUDIO_ROOT}/gradle/gradle-4.1/bin/gradle assemble
#if [ $? -eq 0 ]; then
#    echo OK
#else
#    echo FAIL
#    exit
#fi

echo "Finished NDK build for Csound for Android."




