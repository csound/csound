# Csound for Emscripten and WebAssembly

Authors: Edward Costello, Steven Yi, Henri Manson

## Introduction

These are the sources for Web-based Csound. They were first developed to use 
the emscripten compiler toolchain to produce a pure JavaScript version of 
Csound through asm.js. With the introduction of WebAssembly (WASM), we are now 
able to produce a version of this build targetting that API. WASM code runs 
roughly twice as fast as pure JavaScript or half as fast as native code.

## Requirements

* [Emscripten SDK/toolchain](https://github.com/kripken/emscripten): install 
  the Emscripten tools using these \
  [instructions](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)
  thoroughly. Make sure all steps are followed to ensure a correct
  installation of the toolchain. 
* If you are primarily using WebAssembly you can install the toolchain 
  from source [using these instructions](http://webassembly.org/getting-started/developers-guide/). 
  Building the toolchain is compute and memory intensive. If the build does not 
  complete or produces a zero size clang file, run the build step using only 
  one thread: `./emsdk install sdk-incoming-64bit binaryen-master-64bit -j1`.

## Build Instructions for asm.js

1. First you will need to build libsndfile. Use the 
   `download_and_build_libsndfile.sh` script. In the emscripten folder, run 
   `sh ./download_and_build_libsndfile.sh`. This will create a deps folder, 
   download libsndfile 1.0.25, unarchive the tarball, and then compile libsndfile 
   with Emscripten.
2. Run the build.sh script using `sh ./build.sh`. This will create a build 
   folder; run cmake from there with the Csound source, then compile Csound 
   with Emscripten. The script will then link the output with libsndfile and 
   generate a libcsound.js script file. Finally, the script will copy the 
   libcsound.js and src/CsoundObj.js files into a dist folder.

At this point, the two .js files in the dist folder are all that are necessary to run Csound in a 
Web browser that supports WebAudio.

### Release Instructions

1. Run `sh build.sh`
2. Run `sh update_example_libs_from_dist.sh`
3. Update CS_VERSION in build.sh if necessary
4. Run `sh release.sh`

## Build Instructions for WebAssembly

1. In the `emsdk` directory run `source emsdk_env.sh` and 
   `export EMSCRIPTEN_ROOT=$EMSCRIPTEN`.
2. Change to the `csound/emscripten` directory.
3. First you will need to build libsndfile. Use the
   `download_and_build_libsndfile_wasm.sh` script. In the emscripten
   folder, type `sh download_and_build_libsndfile_wasm.sh`. This will compile 
   `libsndfile-wasm.a` in `deps/libsndfile-1.0.25`.
2. Run the `build-wasm.sh` script using `sh build-wasm.sh`. This will create a 
   build folder, run cmake from there with the Csound source,
   then compile Csound with Emscripten for WASM. The script will copy
   the required files into a dist-wasm folder for distribution.
3. The WASM build works generally as a drop-in replacement for the
   asm.js version, but uses different files.

### Release Instructions

1. Run `sh build-wasm.sh`
2. Run `sh update_example_libs_from_dist_wasm.sh`
3. Update CS_VERSION in build-wasm.sh if necessary
4. Run `sh release-wasm.sh`

### Test Instructions

1. Change to the `examples-wasm` directory.
2. Run  `python httpd.py` to run a local Web server.
3. Run a WebAssembly enabled browser such as Chrome. Navigate to 
  `http://localhost:port_number` where `port_number` is the port printed when 
  `httpd.py` starts.
4. You should see a "Web Assembly Csound" page and be able to select an example 
   CSD from the list on the lower right.
5. Click on the _Compile_ and _Perform_ buttons to see if you can hear the CSD.
