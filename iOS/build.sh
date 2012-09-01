#!/bin/sh
rm -rf cs5iOS
mkdir cs5iOS
cd cs5iOS
cmake ../.. -G Xcode -DUSE_GETTEXT=0 -DUSE_DOUBLE=0 -DBUILD_STATIC_LIBRARY=1 -DBUILD_CSOUND_AC=0 -DBUILD_RELEASE=1 -DCMAKE_BUILD_TYPE=Release
xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target csound-static -configuration Release
cp Release/libcsound.a ./libcsound-device.a
xcodebuild -sdk iphonesimulator5.1 -xcconfig ../simulator.xcconfig -target csound-static -configuration Release 
lipo -create libcsound-device.a Release/libcsound.a -output ../libcsound.a

