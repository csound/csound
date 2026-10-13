#!/bin/sh
set -x
NDK_VERSION=28
NDK_NUMBER=13004108
NDK_RELEASE="android-ndk-r$NDK_VERSION-darwin.dmg"
wget -q https://dl.google.com/android/repository/$NDK_RELEASE
hdiutil attach $NDK_RELEASE
ditto "/Volumes/Android NDK r$NDK_VERSION/AndroidNDK$NDK_NUMBER.app" AndroidNDK.app
