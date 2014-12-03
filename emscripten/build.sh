#!/bin/bash

if [ -z "${EMSCRIPTEN_ROOT+xxx}" ]; then
  echo "EMSCRIPTEN_ROOT is not set. Please set it and try again."
  exit 1
fi

echo "Using EMSCRIPTEN_ROOT: $EMSCRIPTEN_ROOT"

mkdir -p build
cd build
cmake -DCMAKE_C_FLAGS="-Wno-warn-absolute-paths" -DBUILD_MULTI_CORE=0 -DEMSCRIPTEN=1 -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN_ROOT/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_MODULE_PATH=$EMSCRIPTEN_ROOT/cmake -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short"  -DHAVE_STRTOD_L=0 -DBUILD_STATIC_LIBRARY=YES -DHAVE_ATOMIC_BUILTIN=0 -DHAVE_SPRINTF_L=NO -DUSE_GETTEXT=NO -DUSE_OPENMP=0  -DLIBSNDFILE_LIBRARY=../deps/libsndfile-1.0.25/src/.libs/libsndfile.a -DSNDFILE_H_PATH=../deps/libsndfile-1.0.25/src ../.. 
emmake make csound64-static -j6

# build_CsoundObj.sh
emcc -s LINKABLE=1 ../src/CsoundObj.c -I../../include -Iinclude -o CsoundObj.bc


# !/bin/bash

# build_libcsound64.js.sh
#emcc -O3  -s LINKABLE=1 -s VERBOSE=1 -s EXPORTED_FUNCTIONS="['_CsoundObj_new', '_CsoundObj_compileCSD', '_CsoundObj_process', '_CsoundObj_reset', '_CsoundObj_getInputChannelCount', '_CsoundObj_getOutputChannelCount']"  CsoundObj.bc libcsound64.a ../deps/libsndfile-1.0.25/src/.libs/libsndfile.a -o libcsound.js
emcc -O0 -g2 -s ASSERTIONS=2  -s LINKABLE=1 -s VERBOSE=1 -s EXPORTED_FUNCTIONS="['_CsoundObj_new', '_CsoundObj_compileCSD', '_CsoundObj_process', '_CsoundObj_reset', '_CsoundObj_getInputChannelCount', '_CsoundObj_getOutputChannelCount']"  CsoundObj.bc libcsound64.a ../deps/libsndfile-1.0.25/src/.libs/libsndfile.a -o libcsound.js


cd ..
rm -rf dist
mkdir dist
cp build/libcsound.js dist/
cp build/libcsound.js.mem dist/
cp src/CsoundObj.js dist/
