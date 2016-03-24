# Building with msys2 and Mingw64

The following are instructions for building Csound for 64-bit Windows (x86_64) using msys2 and mingw64. These instructions have been tested on Windows 8.1 and Windows 10.

## Setup

1. Install the x86_64 version of [msys2](http://msys2.github.io/). Follow the pacman setup instructions carefully.
2. Using pacman, install the following additional development tools and libraries:
  * mingw-w64-x86_64-toolchain
  * make
  * autoconf
  * automake
  * libtool
  * flex
  * bison
  * doxygen
  * patch
  * dos2unix
  * subversion
  * mingw-w64-x86_64-cmake
  * mingw-w64-x86_64-docbook-xml
  * mingw-w64-x86_64-docbook-xsl
  * mingw-w64-x86_64-swig
  * mingw-w64-x86_64-libsndfile
  * DO NOT install mingw-w64-x86_64-portaudio. We need to build a custom version of PortAudio for Csound (see (5) below).
  * mingw-w64-x86_64-portmidi
  * mingw-w64-x86_64-fltk
  * mingw-w64-x86_64-fluidsynth
  * mingw-w64-x86_64-boost
  * mingw-w64-x86_64-curl
  * mingw-w64-x86_64-luajit-git
  * mingw-w64-x86_64-eigen3
  * mingw-w64-x86_64-hdf5
  * mingw-w64-x86_64-libwebsockets
3. Obtain from [Steinberg](http://www.steinberg.net/en/company/developers.html) the ASIO2 and VST 2.X SDKs and copy them into the mingw64/include directory.
4. Install [Graphviz](http://www.graphviz.org/) and add its bin dir to the PATH in .
5. Open a MinGW-w64 Win64 Shell, which provides a terminal with all tools set up for mingw64 development.
6. If necessary, modify the PATH environment variable in ~/.bash_profile so that executables which compete with MSYS2 cannot be loaded. Also ensure that the environment variable RAWWAVE_PATH is not set, or set to the STK source rawwaves directory.
7. Build and install packages for dependencies not currently in MSYS2's repositories. The formulas are included in the packages folder. Note that the purpose of these packages is simply to get dependencies installed for the Csound build system to find, not to replicate existing packages, so PKGBUILD files may be simplified. Also note that some packages contain patches that may need to be updated when source files are updated. Cd into each directory and use 'makepkg-mingw' to build the package. Use 'pacman -U name-of-package.pkg.tar.xz' to install the package. If there is a 'devel' package in the package directory, install that also. If there are any errors about line endings, simply run dos2unix on the file(s) to change the line endings.
8. Install the following dependencies, which are not or should not be built with mingw64. The build system should be able to find them without manual configuration.
  * The Java SDK for x64 from [Oracle](http://www.oracle.com/technetwork/java/index.html).
  * Python for x64 from [Python.org](https://www.python.org/). Building the Python interface for 64-bit Python with mingw64 requires some hacking. Information is available at [this site](http://ascend4.org/Setting_up_a_MinGW-w64_build_environment). Note that currently, gendef comes with the MSYS2 toolchain, and does not need to be built. You need to create an import library for the Python DLL.
9. Because their build systems are not currently configured to build static libraries, but rather only shared libraries with import libraries, manually set the following CMake variables:
  * Set MUSICXML_LIBRARY to something like D:/msys64/mingw64/bin/libmusicxml2.dll.
  * Set CURL_LIBRARY to something like D:/msys64/mingw64/bin/libcurl-4.dll.
10. Run ./build.sh in the mingw64 directory.
11. Set an environment variable XSL_BASE_PATH ~/.bash_profile e.g. to mingw64/share/xml/docbook/xsl-stylesheets-1.78.1. Clone or update the Csound manual repository from git@github.com:csound/manual.git. Execute "mingw32-make html-dist" in the mingw64 shell to build the Csound Reference Manual.

## Notes

* SWIG_DIR is not parsed correctly by CMake within msys2.  The reason is unknown at this time, and SWIG_DIR was hardcoded in build.sh to the appropriate location.  This is a hack, as the value will have to change if the version of SWIG is updated in the package repository for mingw64.

# NOT YET BUILDING

* Wii opcodes (requires wiiuse)
* Jack (is this done normally on Windows?)
* C Unit tests (cunit library)

# TO DO

* Update installer.
