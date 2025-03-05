#!/bin/sh

rm -rf  CsoundForAndroid/CsoundAndroid/src/main/java/com
cp -r CsoundAndroid/src/com  CsoundForAndroid/CsoundAndroid/src/main/java/

rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd7
cp -r CsoundAndroid/src/csnd7  CsoundForAndroid/CsoundAndroid/src/main/java/

rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs
