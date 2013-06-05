#!/bin/sh

# Simple script to copy "local" shared libs to the installation libs directories.
# This because I can't seem to get the canonical way to do this to work.

cp -f ../pluginlibs/libfluidsynth/libs/armeabi/libfluidOpcodes.so libs/armeabi/
cp -f ../pluginlibs/libstdutil/libs/armeabi/libstdutil.so libs/armeabi/
cp -f ../pluginlibs/luajit-2.0/src/libluajit.so libs/armeabi/
cp -f ../pluginlibs/signalflowgraph/libs/armeabi/libsignalflowgraph.so libs/armeabi/

cp -f ../pluginlibs/libfluidsynth/libs/armeabi-v7a/libfluidOpcodes.so libs/armeabi-v7a
cp -f ../pluginlibs/libstdutil/libs/armeabi-v7a/libstdutil.so libs/armeabi-v7a
cp -f ../pluginlibs/luajit-2.0/src/libluajit.so libs/armeabi-v7a
cp -f ../pluginlibs/signalflowgraph/libs/armeabi-v7a/libsignalflowgraph.so libs/armeabi-v7athe
