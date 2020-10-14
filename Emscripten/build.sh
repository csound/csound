#!/bin/bash 
set -x

mkdir -p dist
mkdir -p build
cd build

export CFLAGS="-O3 -flto"
export CXXFLAGS="-O3 -flto"

emcmake cmake -DCMAKE_VERBOSE_MAKEFILE=1 -DBUILD_PLUGINS_DIR="plugins" -DUSE_COMPILER_OPTIMIZATIONS=0 -DWASM=1 -DINIT_STATIC_MODULES=0 -DUSE_DOUBLE=NO -DBUILD_MULTI_CORE=0 -DBUILD_JACK_OPCODES=0 -DEMSCRIPTEN=1 -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short"  -DHAVE_STRTOD_L=0 -DBUILD_STATIC_LIBRARY=YES -DHAVE_ATOMIC_BUILTIN=0 -DHAVE_SPRINTF_L=NO -DUSE_GETTEXT=NO -DLIBSNDFILE_LIBRARY=../deps/lib/libsndfile.a -DSNDFILE_H_PATH=../deps/include ../..



emmake make csound-static -j6 

emcc -s LINKABLE=1 -s ASSERTIONS=0 ../src/FileList.c -Iinclude -o FileList.bc
emcc -s LINKABLE=1 -s ASSERTIONS=0 ../src/CsoundObj.c -I../../include -Iinclude -o CsoundObj.bc

# Total memory for a WebAssembly module must be a multiple of 64 KB so...
# 1024 * 64 = 65536 is 64 KB
# 65536 * 1024 * 4 is 268435456

# Keep exports in alphabetical order please, to correlate with CsoundObj.js.

## First build for WASM/ScriptProcessorNode (async compilation = 1, assertions = 1)
emcc -v -O3 -flto -DINIT_STATIC_MODULES=0 -s WASM=1 -s ASSERTIONS=1 -s LINKABLE=1 -s RESERVED_FUNCTION_POINTERS=1 -s TOTAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s SINGLE_FILE=1 --pre-js ../src/FileList.js -s BINARYEN_ASYNC_COMPILATION=1 -s MODULARIZE=1 -s EXPORT_NAME=\"'libcsound'\" -s EXTRA_EXPORTED_RUNTIME_METHODS='["FS", "ccall", "cwrap"]' -s ENVIRONMENT=web  CsoundObj.bc FileList.bc libcsound.a ../deps/lib/libsndfile.a ../deps/lib/libogg.a ../deps/lib/libvorbis.a ../deps/lib/libvorbisenc.a ../deps/lib/libFLAC.a -o libcsound.js
 
## Second build for WASM/AudioWorklet (async compilation = 0, assertions = 0)
emcc -v -O3 -flto -DINIT_STATIC_MODULES=0 -s WASM=1 -s ASSERTIONS=0 -s LINKABLE=1 -s RESERVED_FUNCTION_POINTERS=1 -s TOTAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 -s SINGLE_FILE=1 --pre-js ../src/FileList.js -s BINARYEN_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_NAME=\"'libcsound'\"  -s EXTRA_EXPORTED_RUNTIME_METHODS='["FS", "ccall", "cwrap"]' CsoundObj.bc FileList.bc libcsound.a ../deps/lib/libsndfile.a ../deps/lib/libogg.a ../deps/lib/libvorbis.a ../deps/lib/libvorbisenc.a ../deps/lib/libFLAC.a -o libcsound-worklet.js

# node ../convert.js

cd ..

## Update source for npm module ## 

cp build/libcsound.js module/src/
cp build/libcsound-worklet.js module/src/CsoundProcessor.js

#echo "export default libcsound;" >> module/src/libcsound.js

# --post-js does not work with MODULARIZE, use this for ES6 Module 
cat src/CsoundProcessor.js >> module/src/CsoundProcessor.js

## BUILD SINGLE-FILE CsoundObj.js using npm and rollup ##

cd module
npm install 
npm run-script build
cd ..

## Update dist folder ##
cp src/csound.js dist 
cp module/CsoundObj.js dist 
