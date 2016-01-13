# Building with msys2 and Mingw64

The following are instructions for building Csound for 64-bit Windows (x86_64) using msys2 and mingw64. The following was tested on Windows 10.

## Setup 

1. Install the x86_64 version of [msys2](http://msys2.github.io/). Follow the pacman setup instructions carefully.
2. Install additional development tools and libraries using pacman. (TBD)
  * mingw-w64-x86_64-toolchain 
  * flex
  * bison
  * cmake
  * mingw64/mingw-w64-x86_64-swig
  * mingw64/mingw-w64-x86_64-portaudio
  * mingw64/mingw-w64-x86_64-portmidi
  * mingw64/mingw-w64-x86_64-fltk
  * mingw64/mingw-w64-x86_64-fluidsynth
  * mingw64/mingw-w64-x86_64-boost
  * mingw64/mingw-w64-x86_64-curl 
  * mingw64/mingw-w64-x86_64-luajit-git 
  * mingw64/mingw-w64-x86_64-eigen3 
3. Open a MinGW-w64 Win64 Shell.  This will load a terminal with all tools for mingw64 setup.
4. Change directories to this directory and run ./build.sh

## Notes

* SWIG_DIR is not parsed correctly by CMake within msys2.  The reason is unknown at this time, and SWIG_DIR was hardcoded in build.sh to the appropriate location.  This is a hack, as the value will have to change if the version of SWIG is updated in the package repository for mingw64. 

* Building the python interface for 64-bit python requires some hacking to build with mingw64. Information is available at [this site](http://ascend4.org/Setting_up_a_MinGW-w64_build_environment).

# NOT YET BUILDING

* OSC opcodes (requires liblo)
* Lua opcodes (with luajit installed, linker reports -lluajit-5.1 not found, disabled in build.sh for now)
* Lua interface (with luajit installed, linker reports -lluajit-5.1 not found, disabled in build.sh for now)
* Wii opcodes (requires wiiuse)
* STK opcodes (requires STK)
* HDF5 opcodes (hdf5 lib) 
* Websockets opcodes (libwebsockets)
* Jack (is this done normally on Windows?) 
* csoundapi~ for PD (requires pd headers)
* CsoundAC (builds except Lua interface, same linking problems as above)
* C Unit tests (cunit library)
