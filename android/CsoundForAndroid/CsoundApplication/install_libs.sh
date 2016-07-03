#!/bin/sh

# Simple script to copy "local" shared libs to the installation libs directories.
# This because I can't seem to get the canonical way to do this to work.

JNILIBS=src/main/jniLibs
PLUGINLIBS=../../pluginlibs

mkdir -p $JNILIBS 
mkdir -p $JNILIBS/armeabi
mkdir -p $JNILIBS/armeabi-v7a

cp -f $PLUGINLIBS/libfluidsynth/libs/armeabi/libfluidOpcodes.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/libstdutil/libs/armeabi/*.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/signalflowgraph/libs/armeabi/*.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/LuaCsound/libs/armeabi/*.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/libscansyn/libs/armeabi/*.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/libOSC/libs/armeabi/*.so $JNILIBS/armeabi/
cp -f $PLUGINLIBS/doppler/libs/armeabi/*.so $JNILIBS/armeabi/

cp -f $PLUGINLIBS/libfluidsynth/libs/armeabi-v7a/libfluidOpcodes.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libstdutil/libs/armeabi-v7a/libstdutil.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/signalflowgraph/libs/armeabi-v7a/libsignalflowgraph.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/LuaCsound/libs/armeabi-v7a/libLuaCsound.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libscansyn/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/libOSC/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/
cp -f $PLUGINLIBS/doppler/libs/armeabi-v7a/*.so $JNILIBS/armeabi-v7a/

rm -f $JNILIBS/armeabi/libsndfile.so
rm -f $JNILIBS/armeabi-v7a/libsndfile.so
rm -f $JNILIBS/armeabi/libgnustl_shared.so
rm -f $JNILIBS/armeabi-v7a/libgnustl_shared.so

find . -name "*.so"

# Also copy other resources used by Csound opcodes.

mkdir -p src/main/assets/samples/
cp -f ../../../samples/* src/main/assets/samples



