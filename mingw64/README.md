# Building with msys2 and Mingw64

The following are instructions for building Csound for 64-bit Windows (x86_64) using msys2 and mingw64. These instructions have been tested on Windows 8.1 and Windows 10.

## Setup

1. Install the x86_64 version of [msys2](http://msys2.github.io/). Follow the pacman setup instructions carefully.
2. Install the following additional development tools and libraries using pacman:
  * mingw-w64-x86_64-toolchain
  * make
  * autoconf
  * automake
  * libtool
  * flex
  * bison
  * patch
  * dos2unix
  * subversion
  * mingw-w64-x86_64-cmake
  * mingw-w64-x86_64-swig
  * mingw-w64-x86_64-libsndfile
  * mingw-w64-x86_64-portaudio
  * mingw-w64-x86_64-portmidi
  * mingw-w64-x86_64-fltk
  * mingw-w64-x86_64-fluidsynth
  * mingw-w64-x86_64-boost
  * mingw-w64-x86_64-curl
  * mingw-w64-x86_64-luajit-git
  * mingw-w64-x86_64-eigen3
  * mingw-w64-x86_64-hdf5
  * mingw-w64-x86_64-libwebsockets
3. Open a MinGW-w64 Win64 Shell, which provides a terminal with all tools set up for mingw64 development.
4. If necessary, modify the PATH environment variable in ~/.bash_profile so that executables which compete with MSYS2 cannot be loaded. Also ensure that the environment variable RAWWAVE_PATH is not set,
5. Build and install packages for dependencies not currently in MSYS2's repositories. The formulas are included in the packages folder. Note that purpose of these packages is simply to get dependencies installed for the Csound build system to find, not to replicate existing packages, so PKGBUILD files may be simplified. Also note that some packages contain patches that may need to be updated when source files are updated. Cd into each directory and use 'makepkg-mingw' to build the package. Use 'pacman -U name-of-package.pkg.tar.xz' to install the package. If there are any errors about line endings, simply run dos2unix on the file(s) to change the line endings.
6. Install the Java SDK for x64 from Oracle and Python 2.7 for x64 from Python.org. Update the CMake build to find the relevant tools, headers, and libraries.
7. Obtain from Steinberg the ASIO2 and VST 2.X SDKs and copy them into the mingw64/include directory. Update the CMake build to find the relevant sources.
7. Run ./build.sh in the mingw64 directory.

## Notes

* SWIG_DIR is not parsed correctly by CMake within msys2.  The reason is unknown at this time, and SWIG_DIR was hardcoded in build.sh to the appropriate location.  This is a hack, as the value will have to change if the version of SWIG is updated in the package repository for mingw64.

* Building the python interface for 64-bit python requires some hacking to build with mingw64. Information is available at [this site](http://ascend4.org/Setting_up_a_MinGW-w64_build_environment). Note that currently, gendef comes with the MSYS2 toolchain, and does not need to be built. You need to create an import library for the Python DLL.

# NOT YET BUILDING

* Lua opcodes (with luajit installed, linker reports -lluajit-5.1 not found, disabled in build.sh for now)
* Lua interface (with luajit installed, linker reports -lluajit-5.1 not found, disabled in build.sh for now)
* Wii opcodes (requires wiiuse)
* STK opcodes (requires STK)
* Jack (is this done normally on Windows?)
* csoundapi~ for PD (requires pd headers)
* CsoundAC (builds except Lua interface, same linking problems as above)
* C Unit tests (cunit library)

# TO DO

* Target for csound.node x64.
* Install x64 Python from Python.org and make import library.
* Install x64 Java SDK.
* Packages for:
  -- PortAudio with ASIO
  -- Jack?
* Update installer.
