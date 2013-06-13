#!/bin/sh

cd CsoundAndroid
./build.sh

cd ../pluginlibs

for plugin in *
do
  cd $plugin; ndk-build; cd ..;  
done

