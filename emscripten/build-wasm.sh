#!/bin/bash

if [ -z "${EMSCRIPTEN_ROOT+xxx}" ]; then
  echo "EMSCRIPTEN_ROOT is not set. Please set it and try again."
  exit 1
fi

echo "Using EMSCRIPTEN_ROOT: $EMSCRIPTEN_ROOT"
#export EMCC_DEBUG=1

mkdir -p build-wasm
cd build-wasm
cmake -DCMAKE_C_FLAGS="-Wno-warn-absolute-paths" -DWASM=1 -DUSE_DOUBLE=NO -DBUILD_MULTI_CORE=0 -DEMSCRIPTEN=1 -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN_ROOT/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_MODULE_PATH=$EMSCRIPTEN_ROOT/cmake -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short"  -DHAVE_STRTOD_L=0 -DBUILD_STATIC_LIBRARY=YES -DHAVE_ATOMIC_BUILTIN=0 -DHAVE_SPRINTF_L=NO -DUSE_GETTEXT=NO -DLIBSNDFILE_LIBRARY=../deps/libsndfile-1.0.25/src/.libs/libsndfile.a -DSNDFILE_H_PATH=../deps/libsndfile-1.0.25/src ../..
emmake make csound-static -j6

# build_CsoundObj.sh
emcc -s LINKABLE=1 ../src/CsoundObj.c -I../../include -Iinclude -o CsoundObj.bc
emcc -s LINKABLE=1 ../src/FileList.c -Iinclude -o FileList.bc


# build_libcsound.js.sh
emcc -O3 -g4 -s WASM=1 -s "BINARYEN_METHOD='native-wasm'" -s LINKABLE=1 -s RESERVED_FUNCTION_POINTERS=1 -s EXPORTED_FUNCTIONS="['_strlen', '_CsoundObj_new', '_CsoundObj_compileCSD', '_CsoundObj_compileOrcSco', '_CsoundObj_render', '_CsoundObj_evaluateCode', '_CsoundObj_start', '_CsoundObj_compileOrc', '_CsoundObj_readScore', '_CsoundObj_getOutputBuffer', '_CsoundObj_getControlChannel', '_CsoundObj_setControlChannel', '_CsoundObj_getInputBuffer', '_CsoundObj_getKsmps', '_CsoundObj_performKsmps' , '_CsoundObj_reset', '_CsoundObj_getInputChannelCount', '_CsoundObj_getOutputChannelCount', '_CsoundObj_getTableLength', '_CsoundObj_getTable','_CsoundObj_getZerodBFS', '_CsoundObj_pushMidiMessage', '_CsoundObj_setMidiCallbacks', '_CsoundObj_setOutputChannelCallback', '_FileList_getFileCount', '_FileList_getFileNameString']"  CsoundObj.bc FileList.bc libcsound.a ../deps/libsndfile-1.0.25/src/.libs/libsndfile.a -o libcsound.js

cd ..
rm -rf dist
mkdir dist
cp build-wasm/libcsound.js dist/
cp src/*.js dist/
cp build-wasm/libcsound.wasm dist/

