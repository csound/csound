#!/bin/bash 
set -x

# Use EMSCRIPTEN variable, set in Emscripten SDK's emsdk_set_env.sh
if [ -z "${EMSCRIPTEN+xxx}" ]; then
  echo "EMSCRIPTEN is not set. Please set it and try again."
  exit 1
fi

echo "Using EMSCRIPTEN: $EMSCRIPTEN"
#export EMCC_DEBUG=1

mkdir -p build-web-csound
cd build-web-csound

cmake -DCMAKE_VERBOSE_MAKEFILE=1 -DUSE_COMPILER_OPTIMIZATIONS=0 -DWASM=1 -DINIT_STATIC_MODULES=0 -DUSE_DOUBLE=NO -DBUILD_MULTI_CORE=0 -DBUILD_JACK_OPCODES=0 -DEMSCRIPTEN=1 -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_MODULE_PATH=$EMSCRIPTEN/cmake -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short"  -DHAVE_STRTOD_L=0 -DBUILD_STATIC_LIBRARY=YES -DHAVE_ATOMIC_BUILTIN=0 -DHAVE_SPRINTF_L=NO -DUSE_GETTEXT=NO -DLIBSNDFILE_LIBRARY=../deps/libsndfile-1.0.25/libsndfile-wasm.a -DSNDFILE_H_PATH=../deps/libsndfile-1.0.25/src ../..

emmake make csound-static -j6 

emcc -s LINKABLE=1 -s ASSERTIONS=0 ../src/FileList.c -Iinclude -o FileList.bc
emcc -s LINKABLE=1 -s ASSERTIONS=0 ../src/CsoundObj.c -I../../include -Iinclude -o CsoundObj.bc

# Total memory for a WebAssembly module must be a multiple of 64 KB so...
# 1024 * 64 = 65536 is 64 KB
# 65536 * 1024 * 4 is 268435456

# Keep exports in alphabetical order please, to correlate with CsoundObj.js.


## First build for WASM/ScriptProcessorNode (async compilation = 1, assertions = 1)
emcc -v -O2 -g4 -DINIT_STATIC_MODULES=0 -s WASM=1 -s ASSERTIONS=1 -s "BINARYEN_METHOD='native-wasm'" -s LINKABLE=1 -s RESERVED_FUNCTION_POINTERS=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s BINARYEN_ASYNC_COMPILATION=1 -s MODULARIZE=1 -s EXPORT_NAME=\"'libcsound'\" -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' CsoundObj.bc FileList.bc libcsound.a ../deps/libsndfile-1.0.25/libsndfile-wasm.a -o libcsound.js
 
## Second build for WASM/AudioWorklet (async compilation = 0, assertions = 0)
emcc -v -O2 -g4 -DINIT_STATIC_MODULES=0 -s WASM=1 -s ASSERTIONS=0 -s "BINARYEN_METHOD='native-wasm'" -s LINKABLE=1 -s RESERVED_FUNCTION_POINTERS=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s BINARYEN_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_NAME=\"'libcsound'\" -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' CsoundObj.bc FileList.bc libcsound.a ../deps/libsndfile-1.0.25/libsndfile-wasm.a -o libcsound-worklet.js


sh ../convert.sh

echo "AudioWorkletGlobalScope.libcsound = libcsound" >> libcsound.js
echo "AudioWorkletGlobalScope.libcsound = libcsound" >> libcsound-worklet.js

cd ..
rm -rf dist-web-csound
mkdir dist-web-csound
cp src/FileList.js dist-web-csound/
cp src/CsoundProcessor.js dist-web-csound/CsoundProcessor.js
cp src/CsoundObjNew.js dist-web-csound/CsoundObj.js
cp build-web-csound/libcsound.js dist-web-csound/
cp build-web-csound/libcsound.wasm dist-web-csound/
cp build-web-csound/libcsound-worklet.js dist-web-csound/
cp build-web-csound/libcsound-worklet.base64.js dist-web-csound/
#cp build-wasm/libcsound.js.map dist-wasm/
