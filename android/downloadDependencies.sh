#!/bin/sh

if [ -z "$NDK_MODULE_PATH" ]; then
    echo "ERROR: NDK_MODULE_PATH is not set. Please set this variable to continue.\n";
    exit;
fi

echo "Using NDK_MODULE_PATH: $NDK_MODULE_PATH\n"
cd $NDK_MODULE_PATH

# LIBSNDFILE
LIBSNDFILE_REPO=https://bitbucket.org/kunstmusik/libsndfile-android.git
if [ -e libsndfile-android ]; then
  echo "libsndfile-android already exists, doing a pull to get the latest";
  cd libsndfile-android;
  git pull;
  cd ..;
else
  echo "Cloning libsndfile-android...";
  git clone $LIBSNDFILE_REPO
fi

# FLUIDSYNTH
FLUIDSYNTH_REPO=https://bitbucket.org/kunstmusik/fluidsynth-android.git
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
  git pull;
  cd ..;
else
  echo "Cloning libluajit...";
  git clone $LUAJIT_REPO 
fi

