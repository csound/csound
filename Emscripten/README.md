# WebAudio Csound

Authors: Edward Costello, Steven Yi, Victor Lazzarini, Henri Manson, 

## Introduction

These are the sources for WebAudio Csound, based on Web Assembly (WASM)
and the Web Audio API. 

## Requirements

* [Emscripten SDK/toolchain](https://github.com/kripken/emscripten): install 
  the Emscripten tools using these \
  [instructions](https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html)
  thoroughly. Make sure all steps are followed to ensure a correct
  installation of the toolchain. 

## Build Instructions 

1. First you will need to build libsndfile. Use the 
   `download_and_build_libsndfile.sh` script. In the emscripten folder, run 
   `sh ./download_and_build_libsndfile.sh`. This will create a deps folder, 
   download libsndfile 1.0.25, unarchive the tarball, and then compile libsndfile 
   with Emscripten.
2. Run the build.sh script using `sh ./build.sh`. This will create a build 
   folder; run cmake from there with the Csound source, then compile Csound 
   with Emscripten, producing the WASM binaries and their JS interfaces.

At this point, the two .js files in the dist folder are all that are necessary to run Csound in a 
Web browser that supports WebAudio.

### Release Instructions

1. Run `sh build.sh`
2. Run `sh update_example_libs_from_dist.sh`
3. Update CS_VERSION in build.sh if necessary
4. Run `sh release.sh`



