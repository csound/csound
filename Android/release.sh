#!/bin/sh
set -x
export RELEASE_DIR=csound-android-7.0.0

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf CsoundForAndroid/CsoundAndroid/src/main/java/csnd7
cp -r CsoundAndroid/src/csnd7  CsoundForAndroid/CsoundAndroid/src/main/java/

rm -rf CsoundForAndroid/CsoundAndroid/src/main/jniLibs
cp -r CsoundAndroid/libs  CsoundForAndroid/CsoundAndroid/src/main/jniLibs

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cd $RELEASE_DIR

# Copy and Clean CsoundForAndroid
cp -R ../CsoundForAndroid .
cd CsoundForAndroid
./gradlew clean
rm -r .gradle
cd ..

rm -f ${RELEASE_DIR}.zip
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR} 


