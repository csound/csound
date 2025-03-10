#!/bin/sh
set -x
BUILD_DIR=Csound-iOS
rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR
cmake ../.. -G Xcode -DCMAKE_PREFIX_PATH="$PWD/.." -DUSE_GETTEXT=0 -DUSE_DOUBLE=0 -DBUILD_STATIC_LIBRARY=1 -DBUILD_RELEASE=0 -DCMAKE_BUILD_TYPE=Release -DUSE_CURL=0 -DUSE_SSE=0 -DIOS=1 -DBUILD_TESTS=0 -DCUSTOM_CMAKE="../custom.cmake.ios"
xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose
cp Release/libCsoundLib.a ./libCsoundLib-device.a
xcodebuild -sdk iphonesimulator -xcconfig ../simulator.xcconfig -target CsoundLib-static -configuration Release -verbose
# build framework
xcodebuild -create-xcframework -library Release/libCsoundLib.a -headers ../../include -library libCsoundLib-device.a -headers ../../include  -output xcframeworks/CsoundiOS.xcframework
# copy generated headers
cp include/float-version.h xcframeworks/CsoundiOS.xcframework/ios-arm64/Headers/
cp include/version.h xcframeworks/CsoundiOS.xcframework/ios-arm64/Headers/
cp include/float-version.h xcframeworks/CsoundiOS.xcframework/ios-arm64-simulator/Headers/
cp include/version.h xcframeworks/CsoundiOS.xcframework/ios-arm64-simulator/Headers/
# remove CMakeLists
rm xcframeworks/CsoundiOS.xcframework/ios-arm64/Headers/CMakeLists.txt
rm xcframeworks/CsoundiOS.xcframework/ios-arm64-simulator/Headers/CMakeLists.txt
# copy framework
cd xcframeworks
cp -rf CsoundiOS.xcframework ../../Csound-For-iOS/.



