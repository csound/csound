#!/bin/sh

MACHINE="$(uname -s)"
case "${MACHINE}" in 
  MINGW*) NDK_BUILD_CMD=$ANDROID_NDK_ROOT/ndk-build.cmd;;
  *) NDK_BUILD_CMD=$ANDROID_NDK_ROOT/ndk-build
esac

echo "NDK_BUILD_COMMAND = $NDK_BUILD_CMD"

flex -B -t ../../Engine/csound_orc.lex > jni/csound_orclex.c 
flex -B ../../Engine/csound_pre.lex > jni/csound_prelex.c 
flex -B ../../Engine/csound_prs.lex > jni/csound_prslex.c 
bison -d -pcsound_orc --report=itemset -o jni/csound_orcparse.c ../../Engine/csound_orc.y

rm -rf src/csnd6
mkdir -p src/csnd6

swig -java -package csnd6 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DPARCS -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H -includeall -verbose -outdir src/csnd6 -c++ -I/usr/local/include -I../../H -I../../include -I../../Engine -I../.. -I../../interfaces -I/System/Library/Frameworks/Python.framework/Headers -I/System/Library/Frameworks/JavaVM.framework/Headers -I./jni -o jni/java_interfaceJAVA_wrap.cpp android_interface.i

# ADJUST SWIG CODE FOR ANDROID and DIRECTORS
sed -i.bak "s/AttachCurrentThread((void \*\*)/AttachCurrentThread(/" jni/java_interfaceJAVA_wrap.cpp 

# Actually build Csound.
cd jni

$NDK_BUILD_CMD V=1 -j 6 $1


