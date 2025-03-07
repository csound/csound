#!/bin/sh
set -x
NDK_VERSION=28
NDK_NUMBER=13004108
NDK_RELEASE="android-ndk-r$NDK_VERSION-darwin.dmg"

wget https://dl.google.com/android/repository/$NDK_RELEASE
open $NDK_RELEASE
ditto "/Volumes/Android NDK r$NDK_VERSION/AndroidNDK$NDK_NUMBER.app" AndroidNDK.app
export ANDROID_NDK_ROOT="$PWD/AndroidNDK.app/Contents/NDK"
export NDK_MODULE_PATH=$PWD/modules
sh downloadDependencies.sh
cd CsoundAndroid
bison --version
sh build.sh
cd ..
sh update.sh
sh release.sh

