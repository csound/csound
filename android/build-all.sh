#!/bin/sh

MACHINE="$(uname -s)"
case "${MACHINE}" in 
  MINGW*) NDK_BUILD_CMD=$ANDROID_NDK_ROOT/ndk-build.cmd;;
  *) NDK_BUILD_CMD=$ANDROID_NDK_ROOT/ndk-build
esac

echo "NDK_BUILD_COMMAND = $NDK_BUILD_CMD"

if [[ $1 == "clean" ]]
then
    echo "Cleaning up shared libraries..."
    find . -name "*.so" -delete  
fi

cd pluginlibs

for plugin in *
do
    echo "Building in " ${plugin}
    cd $plugin $1
    $NDK_BUILD_CMD $1
    cd ..
done

cd ../CsoundAndroid
./build.sh $1


