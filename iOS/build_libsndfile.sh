#!/bin/sh
git clone https://github.com/libsndfile/libsndfile.git
cd libsndfile
mkdir build && cd build
cmake -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 -DENABLE_MPEG=0 -DENABLE_EXTERNAL_LIBS=OFF -DBUILD_SHARED_LIBS=OFF ..
echo "========================== Creating iphone lib =========================="
xcodebuild -sdk iphoneos -xcconfig ../../device.xcconfig -target sndfile -configuration Release -verbose
echo "========================== Copying libsndfile.a and headers =========================="
mkdir ../../lib
cp Release/libsndfile.a ../../lib/.
cp -r ../include ../..
cd Release
cp libsndfile.a libsndfile-device.a
cd ..
echo "========================== Creating simulator lib =========================="
xcodebuild -sdk iphonesimulator -xcconfig ../../simulator.xcconfig -target sndfile -configuration Release 
echo "========================== Creating xcframework =========================="
xcodebuild -create-xcframework  -library Release/libsndfile.a -headers ../include -library Release/libsndfile-device.a -headers ../include  -output xcframeworks/libSndfileiOS.xcframework
