#!/bin/sh

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
    $NDK/ndk-build $1
    cd ..
done

cd ../CsoundAndroid
./build.sh $1


