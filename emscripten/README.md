# Csound for Emscripten and Web Assembly

Authors: Edward Costello, Steven Yi

## Introduction

## Requirements

* [Emscripten SDK/toolchain](https://github.com/kripken/emscripten): install the
  emscripten tools using these
  [instructions](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)
  thoroughly. Make sure all steps are followed to ensure a correct
  installation of the toolchain.

## Build Instructions for asm.js

1. First you will need to build libsndfile.  Use the `download_and_build_libsndfile.sh `script. Be sure to be in the emscripten folder and type `sh ./download_and_build_libsndfile.sh`. This will create a deps folder, download libsndfile 1.0.25, unarchive the tarball, and then compile libsndfile with Emscripten.
2. Run the build.sh script using `sh ./build.sh`.  This will create a build folder, run cmake from there with the Csound source, then compile Csound with Emscripten.  The script will then link the output with libsndfile and generate a libcsound.js script file.  Finally, the script will copy the libcsound.js and src/CsoundObj.js files into a dist folder.

At this point, the two js files in the dist folder are all that are necessary to run Csound in a web browser that supports WebAudio.

### Release Instructions

1. Run `sh ./build.sh`
2. Run `sh ./update_example_libs_from_dist.sh`
3. Update CS_VERSION in build.sh if necessary
4. Run `sh ./release.sh`


## Build Instructions for WASM

1. First you will need to build libsndfile.  Use the
   `download_and_build_libsndfile_wasm.sh` script. In the emscripten
   folder, type `sh ./download_and_build_libsndfile_wasm.sh`. This
   will compile `libsndfile-wasm.a` in `deps/libsndfile-1.0.25`.
2. Run the `build-wasm.sh` script using `sh ./build-wasm.sh`.  This will
   create a build folder, run cmake from there with the Csound source,
   then compile Csound with Emscripten for WASM.  The script will copy
   the required files into a dist-wasm folder for distribution.
3. The WASM build works generally as a drop-in replacement for the
   asm,js version, but uses different files.

### Release Instructions

1. Run `sh ./build-wasm.sh`
2. Run `sh ./update_example_libs_from_dist_wasm.sh`
3. Update CS_VERSION in build.sh if necessary
4. Run `sh ./release-wasm.sh`


