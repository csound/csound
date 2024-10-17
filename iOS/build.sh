#!/bin/sh
set -x
rm -rf cs7iOS
mkdir cs7iOS
cd cs7iOS
cmake ../.. -G Xcode -DCMAKE_PREFIX_PATH="$PWD/.." -DUSE_GETTEXT=0 -DUSE_DOUBLE=0 -DBUILD_STATIC_LIBRARY=1 -DBUILD_RELEASE=0 -DCMAKE_BUILD_TYPE=Release -DUSE_CURL=0 -DUSE_SSE=0 -DIOS=1 -DBUILD_TESTS=0 -DCUSTOM_CMAKE="../custom.cmake.ios"
# CMake was adding -mfpmath=sse which makes the build fail. The project needs to be edited in XCode to remove it before
# the build for devices can continue.
# clearing the CMAKE_C_FLAGS on custom.cmake seems to have solved this

xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose
# something goes wrong with the creation of the csound_prslex.c file and it fails to build so we
# run it again to succeed. TODO: see why the build is out of order causing this problem
# xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose

cp Release/libCsoundLib.a ./libcsound-device.a
xcodebuild -sdk iphonesimulator -xcconfig ../simulator.xcconfig -target CsoundLib-static -configuration Release
xcodebuild -create-xcframework -library Release/libCsoundLib.a -headers include -library libcsound-device.a -headers include    -output xcframeworks/CsoundiOS.xcframework

lipo -create libcsound-device.a Release/libCsoundLib.a -output ../libcsound.a


