#!/bin/bash
#!/bin/bash
clear
echo "Beginning NDK build for Csound for Android..."
if [ -z "$ANDROID_STUDIO_HOME" ]; then
    echo "ERROR: ANDROID_STUDIO_HOME is not set. Please set this variable to point to the root directory of your Android Studio installation to continue.";
    exit;
fi
#if [ -z "$CSOUND_HOME" ]; then
#    echo "ERROR: CSOUND_HOME is not set. Please set this variable to point to the root directory of your Csound git repository to continue.";
#    exit;
#fi
if [ -z "$NDK" ]; then
    echo "ERROR: NDK is not set. Please set this variable to point to the root directory of your Android Native Development Kit installation to continue.";
    exit;
fi


MACHINE="$(uname -s)"
case "${MACHINE}" in 
  MINGW*) NDK_BUILD_CMD=$NDK/ndk-build.cmd;;
  *) NDK_BUILD_CMD=$NDK/ndk-build
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

cd pluginlibs

echo "Building luajit-2.1..."
cd luajit-2.0
# The luajit library can't be compiled with the clang NDK, so we cross-compile using gcc.
# We have to turn large file support OFF.
# PREFIX and ARM produce directories compatible with ndk-build.
# Build for arm. 
make clean
make HOST_CC="gcc -m32" BUILD_MODE=static CROSS=arm-linux-gnueabi- TARGET_CFLAGS="-mcpu=cortex-a8 -mfloat-abi=softfp -fPIC -D_FILE_OFFSET_BITS=32"
make install PREFIX=`pwd`/local MULTILIB=libs/armeabi-v7a
# Build for arm64.
make clean
make HOST_CC="gcc" BUILD_MODE=static CROSS=aarch64-linux-gnu- TARGET_CFLAGS="-fPIC -D_FILE_OFFSET_BITS=32"
make install PREFIX=`pwd`/local MULTILIB=libs/arm64-v8a
# Make certain that LuaCsound links only with the STATIC LuaJIT library.
find . -name *.so* -delete
cd ..

echo "Building the Oboe audio driver library..."
cd android-audio-high-performance/oboe
# PLEASE NOTE: Android Studio's gradle muset be used!
bash ${ANDROID_STUDIO_HOME}/gradle/gradle-4.1/bin/gradle assemble
cd ../..

for plugin in *
do
    echo "Building in " ${plugin}
    cd $plugin $1
    $NDK_BUILD_CMD $1
    cd ..
done

echo "Building CsoundAndroid library..."
cd ../CsoundAndroid
./build.sh $1
cd ..

echo "Installing binaries for the Csound for Android app..."
rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd6
cp -r CsoundAndroid/src/csnd6  CsoundForAndroid/CsoundAndroid/src/main/java/
rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cd CsoundForAndroid/CsoundApplication
./install_libs.sh
cd ..

echo "Finished NDK build for Csound for Android."




