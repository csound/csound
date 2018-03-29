#!/bin/sh 

export RELEASE_DIR=csound-android-6.11.0

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

for plugin in pluginlibs/*
do
  rm -r $plugin/obj
done

#rm -rf pluginlibs/libfluidsynth/obj

cd docs
pdflatex csound_android_manual.tex
pdflatex csound_android_manual.tex
cd ..

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cd $RELEASE_DIR


# Copy and Clean CsoundForAndroid
cp -R ../CsoundForAndroid .
cd CsoundForAndroid
./gradlew clean
rm -r .gradle
cd ..

cp ../COPYING .
cp ../CHANGELOG .
cp ../docs/csound_android_manual.pdf .
cp -R ../pluginlibs .

for plugin in pluginlibs/*
do
  rm -r $plugin/obj
done

rm -r pluginlibs/luajit-2.0
rm -r pluginlibs/libsndfile-android
rm -r pluginlibs/fluidsynth-android
rm -r pluginlibs/liblo-android
rm -r pluginlibs/stk
rm -r pluginlibs/patches

cd ..

rm ${RELEASE_DIR}.zip
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR} 
