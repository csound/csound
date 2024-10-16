#!/bin/sh

git clone https://github.com/libsndfile/libsndfile.git
cd libsndfile
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$PWD/../.." -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 -DENABLE_MPEG=0 -DENABLE_EXTERNAL_LIBS=OFF -DBUILD_SHARED_LIBS=OFF ..
make && make install
#xcodebuild -sdk iphoneos -xcconfig ../../device.xcconfig -target sndfile -configuration Release -verbose
#cd Release
#cp libsndfile.a libsndfile-device.a
#cd ..
#echo "========================== Creating simulator =========================="
#xcodebuild -sdk iphonesimulator -xcconfig ../../simulator.xcconfig -target sndfile -configuration Release 
#echo "========================== Creating xcframework =========================="
#xcodebuild -create-xcframework  -library Release/libsndfile.a -headers ../include -library Release/libsndfile-device.a -headers# ../include  -output xcframeworks/libSndfileiOS.xcframework
