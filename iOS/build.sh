#!/bin/sh
rm -rf cs6iOS
mkdir cs6iOS
cd cs6iOS
cmake ../.. -G Xcode -DUSE_GETTEXT=0 -DUSE_DOUBLE=0 -DBUILD_STATIC_LIBRARY=1 -DBUILD_CSOUND_AC=0 -DBUILD_RELEASE=1 -DCMAKE_BUILD_TYPE=Release -DUSE_CURL=0  -DIOS=1
xcodebuild -sdk iphoneos -xcconfig ../device.xcconfig -target CsoundLib-static -configuration Release -verbose
cp Release/libCsoundLib.a ./libcsound-device.a
xcodebuild -sdk iphonesimulator -xcconfig ../simulator.xcconfig -target CsoundLib-static -configuration Release 
lipo -create libcsound-device.a Release/libCsoundLib.a -output ../libcsound.a

