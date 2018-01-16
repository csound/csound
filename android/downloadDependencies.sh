#!/bin/sh

export LUAJIT_JNI=`pwd`/pluginlibs/patches/luajit-2.0/jni
cd pluginlibs

# LIBSNDFILE
LIBSNDFILE_REPO=http://bitbucket.org/kunstmusik/libsndfile-android.git
if [ -e libsndfile-android ]; then
  echo "libsndfile-android already exists, doing a pull to get the latest";
  cd libsndfile-android;
  git pull;
  cd ..;
else
  echo "Cloning libsndfile-android...";
  git clone $LIBSNDFILE_REPO
fi

OBOE_REPO=https://github.com/google/oboe.git
if [ -e oboe ]; then
  echo "oboe already exists, doing a pull to get the latest";
  cd oboe;
  git pull;
  cd ..;
else
  echo "Cloning oboe...";
  git clone $OBOE_REPO
fi

# FLUIDSYNTH
FLUIDSYNTH_REPO=http://bitbucket.org/kunstmusik/fluidsynth-android.git
if [ -e fluidsynth-android ]; then
  echo "fluidsynth-android already exists, doing a pull to get the latest";
  cd fluidsynth-android;
  git pull;
  cd ..;
else
  echo "Cloning fluidsynth-android...";
  git clone $FLUIDSYNTH_REPO
fi

# LUAJIT
LUAJIT_REPO=http://luajit.org/git/luajit-2.0.git
if [ -e luajit-2.0 ]; then
  echo "libluajit already exists, doing a pull to get the latest";
  cd luajit-2.0;
  git checkout v2.1;
  git pull;
  cp -R $LUAJIT_JNI .
  cd ..;
else
  echo "Cloning libluajit...";
  git clone $LUAJIT_REPO -b v2.1
  cp -R $LUAJIT_JNI luajit-2.0/
fi

# STK
STK_REPO=http://github.com/thestk/stk.git
if [ -e stk ]; then
  echo "STK already exists, doing a pull to get the latest";
  cd stk;
  git pull;
  cd ..;
else
  echo "Cloning STK...";
  git clone $STK_REPO
fi

# OpenSoundControl
OSC_REPO=https://bitbucket.org/michael_gogins/liblo-android
if [ -e liblo-android ]; then
  echo "liblo-android already exists, doing a pull to get the latest";
  cd liblo-android;
  git pull;
  cd ..;
else
  echo "Cloning liblo...";
  git clone $OSC_REPO
fi

echo "Copying all patched files into build tree..."
# Copy any patched files.
cd patches
cp -rf * ..
cd ../..
