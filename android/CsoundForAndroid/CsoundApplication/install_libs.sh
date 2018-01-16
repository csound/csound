#!/bin/sh

# Simple script to copy "local" shared libs to the installation libs directories.
# This because I can't seem to get the canonical way to do this to work.

JNILIBS=src/main/jniLibs
PLUGINLIBS=../../pluginlibs

mkdir -p $JNILIBS 
mkdir -p $JNILIBS/arm64-v8a
mkdir -p $JNILIBS/armeabi-v7a

cp -f $PLUGINLIBS/libfluidsynth/libs/arm64-v8a/libfluidOpcodes.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/libstdutil/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/signalflowgraph/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/LuaCsound/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/libscansyn/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/libOSC/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/doppler/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/stk-csound/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/ableton_link_opcodes/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/
cp -f $PLUGINLIBS/oboe/libs/arm64-v8a/*.so $JNILIBS/arm64-v8a/

cp -f $PLUGINLIBS/libfluidsynth/libs/armeabi-v7a/libfluidOpcodes.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libstdutil/libs/armeabi-v7a/libstdutil.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/signalflowgraph/libs/armeabi-v7a/libsignalflowgraph.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/LuaCsound/libs/armeabi-v7a/libLuaCsound.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libscansyn/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libOSC/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/doppler/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/stk-csound/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/ableton_link_opcodes/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/oboe/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/

rm -f $JNILIBS/arm64-v8a/libsndfile.so
rm -f $JNILIBS/armeabi-v7a/libsndfile.so
rm -f $JNILIBS/arm64-v8a/libgnustl_shared.so
rm -f $JNILIBS/armeabi-v7a/libgnustl_shared.so

echo "These are the built and copied libs for the Csound for Android app:"
find ../../CsoundAndroid/libs -name "*.so" -ls
find $JNILIBS -name "*.so" -ls

# Also copy other resources used by Csound opcodes.

mkdir -p src/main/assets/samples/
cp -f ../../../samples/* src/main/assets/samples



