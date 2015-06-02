#!/bin/bash


MINGW64_ROOT=`pwd`/mingw64/usr/local
rm -rf csound-w64
mkdir csound-w64
cp -r csound-mingw64/dist/* csound-w64

cp csound-mingw64/csound64.dll csound-w64/bin
cp csound-mingw64/csnd6.dll csound-w64/lib

cd csound-w64/bin

cp $MINGW64_ROOT/bin/libvorbis-0.dll . 
cp $MINGW64_ROOT/bin/libogg-0.dll . 
cp $MINGW64_ROOT/bin/libvorbisenc-2.dll .
cp $MINGW64_ROOT/bin/libportaudio-2.dll  . 
cp $MINGW64_ROOT/bin/libvorbisfile-3.dll .
cp $MINGW64_ROOT/bin/libsndfile-1.dll .
cp $MINGW64_ROOT/lib/pthreadGC2.dll .

cd ../..
zip -r csound-w64.zip csound-w64
