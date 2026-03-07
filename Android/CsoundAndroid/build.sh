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

rm -rf src/csnd7
mkdir -p src/csnd7

swig -java -package csnd7 -DUSE_DOUBLE=0 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DPARCS -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H -includeall -verbose -outdir src/csnd7 -c++ -I$HOME/include/c++/v1 -I/usr/local/include -I../../H -I../../include -I../../Engine -I../.. -I../../interfaces -I/System/Library/Frameworks/Python.framework/Headers -I/System/Library/Frameworks/JavaVM.framework/Headers -I./jni -o jni/java_interfaceJAVA_wrap.cpp android_interface.i

# ADJUST SWIG CODE FOR ANDROID and DIRECTORS
sed -i.bak "s/AttachCurrentThread((void \*\*)/AttachCurrentThread(/" jni/java_interfaceJAVA_wrap.cpp 

# Actually build Csound.
cd jni

if [ -n "$CSOUND_VERSION" ]; then
    CSOUND_VERSION=$(echo "$CSOUND_VERSION" | sed -E 's/^([0-9]+)\.([0-9]+)\.([0-9]+).*/\1 \2 \3/')

    ORIG_PARAMS="$@"

	set -- $CSOUND_VERSION

	CSOUND_VERSION_MAJOR=$1
	CSOUND_VERSION_MINOR=$2
	CSOUND_VERSION_PATCH=$3

    set -- $ORIG_PARAMS

    cp version.template.h version.h
    sed -i.bak "s/@CSOUND_VERSION_MAJOR@/${CSOUND_VERSION_MAJOR:-7}/" version.h
    sed -i.bak "s/@CSOUND_VERSION_MINOR@/${CSOUND_VERSION_MINOR:-0}/" version.h
    sed -i.bak "s/@CSOUND_VERSION_PATCH@/${CSOUND_VERSION_PATCH:-0}/" version.h
fi

$NDK_BUILD_CMD V=1 -j 6 $1


