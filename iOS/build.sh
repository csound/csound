#!/bin/sh
set -x
rm -rf cs6iOS
mkdir cs6iOS
cd cs6iOS
cmake ../.. -G Xcode -T buildsystem=1 -DUSE_GETTEXT=0 -DUSE_DOUBLE=0 -DBUILD_STATIC_LIBRARY=1 -DBUILD_RELEASE=1 -DCMAKE_BUILD_TYPE=Release -DUSE_CURL=0 -DUSE_SSE=0 -DIOS=1 -DCUSTOM_CMAKE="../custom.cmake.ios"
# CMake was adding -mfpmath=sse which makes the build fail. The project needs to be edited in XCode to remove it before
# the build for devices can continue.
# clearing the CMAKE_C_FLAGS on custom.cmake seems to have solved this

xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose
# something goes wrong with the creation of the csound_prslex.c file and it fails to build so we
# run it again to succeed. TODO: see why the build is out of order causing this problem
# xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose

cp Release/libCsoundLib.a ./libcsound-device.a
xcodebuild -sdk iphonesimulator -xcconfig ../simulator.xcconfig -target CsoundLib-static -configuration Release 
lipo -create libcsound-device.a Release/libCsoundLib.a -output ../libcsound.a

