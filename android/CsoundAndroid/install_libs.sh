#!/bin/sh

# Simple script to copy "local" shared libs to the installation libs directories.
# This because I can't seem to get the canonical way to do this to work.

cd ../CSDPlayer
mkdir assets/armeabi/
mkdir assets/armeabi/OPCODE6DIR/
cp -f ../pluginlibs/libfluidsynth/libs/armeabi/libfluidOpcodes.so assets/armeabi/OPCODE6DIR/
cp -f ../pluginlibs/libstdutil/libs/armeabi/libstdutil.so assets/armeabi/OPCODE6DIR/
cp -f ../pluginlibs/luajit-2.0/src/libluajit.so assets/armeabi/OPCODE6DIR/
cp -f ../pluginlibs/signalflowgraph/libs/armeabi/libsignalflowgraph.so assets/armeabi/OPCODE6DIR/
cp -f ../pluginlibs/LuaCsound/libs/armeabi/libLuaCsound.so assets/armeabi/OPCODE6DIR/

mkdir assets/armeabi-v7a/
mkdir assets/armeabi-v7a/OPCODE6DIR/
cp -f ../pluginlibs/libfluidsynth/libs/armeabi-v7a/libfluidOpcodes.so assets/armeabi-v7a/OPCODE6DIR/
cp -f ../pluginlibs/libstdutil/libs/armeabi-v7a/libstdutil.so assets/armeabi-v7a/OPCODE6DIR/
cp -f ../pluginlibs/luajit-2.0/src/libluajit.so assets/armeabi-v7a/OPCODE6DIR/
cp -f ../pluginlibs/signalflowgraph/libs/armeabi-v7a/libsignalflowgraph.so assets/armeabi-v7a/OPCODE6DIR/
cp -f ../pluginlibs/LuaCsound/libs/armeabi-v7a/libLuaCsound.so assets/armeabi-v7a/OPCODE6DIR/

# Also copy other resources used by Csound opcodes.

mkdir assets/samples/
cp -f ../../samples/* assets/samples
