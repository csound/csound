#!/bin/sh

export ANDROID_NDK_ROOT=$HOME/work/android-ndk-r7
export NDK_MODULE_PATH=../../android
flex -B -t ../Engine/csound_orc.l > jni/csound_orclex.c 
flex -B ../Engine/csound_pre.lex > jni/csound_prelex.c 
bison -d -pcsound_orc --report=itemset -o jni/csound_orcparse.c ../Engine/csound_orc.y

rm -rf src/csnd
mkdir -p src/csnd

swig -java -package csnd -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DPARCS -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H -includeall -verbose -outdir src/csnd -c++ -I/usr/local/include -I../H -I../Engine -I.. -I../interfaces -I/System/Library/Frameworks/Python.framework/Headers -I/System/Library/Frameworks/JavaVM.framework/Headers -I./jni -o jni/java_interfaceJAVA_wrap.cpp android_interface.i

# ADJUST SWIG CODE FOR ANDROID and DIRECTORS
sed -i "" 's/AttachCurrentThread((void \*\*)/AttachCurrentThread(/' jni/java_interfaceJAVA_wrap.cpp 

$ANDROID_NDK_ROOT/ndk-build TARGET_PLATFORM=android-9



