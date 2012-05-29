#!/bin/sh 

export RELEASE_DIR=csound-android-5.17.11 

rm -rf CsoundAndroid/obj

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cd $RELEASE_DIR

cp ../COPYING .
cp -R ../CSDPlayer .
cp -R ../CsoundAndroid .
cp -R ../CsoundAndroidExamples .

cd ..

zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR} 
