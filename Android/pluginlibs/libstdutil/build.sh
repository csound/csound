#!/bin/sh

export ANDROID_NDK_ROOT=$HOME/work/android-ndk-r8b
export NDK_MODULE_PATH=../../../../android

$ANDROID_NDK_ROOT/ndk-build TARGET_PLATFORM=android-9



