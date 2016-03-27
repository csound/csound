# Building with msys2 and Mingw64

The following are instructions for building Csound for 64-bit Windows (x86_64) using msys2 and mingw64. These instructions were used to build the Windows installer for Windows 64 bit CPU architecture on Windows 8.

These instructions come in two parts. The first is for building a "plain" Csound that the build will install into a "dist" directory. This build is simple, and Csound will run in the "dist" directory. The second is for building all features of Csound that are available for Windows 64 bit CPU architecture, then building an Inno Setup installer for them, then running the installer. This build is considerably more complex.

## Plain Build

8. Install the following dependencies, which are not or should not be built with mingw64. The build system should be able to find them without manual configuration.
  * The Java SDK for x64 from [Oracle](http://www.oracle.com/technetwork/java/index.html).
  * Python for x64 from [Python.org](https://www.python.org/).
3. Obtain from [Steinberg](http://www.steinberg.net/en/company/developers.html) the ASIO2 and VST 2.X SDKs and copy them into the mingw64/include directory.
4. Install [Graphviz](http://www.graphviz.org/) and add its bin dir to the PATH in ~/.bash_profile.
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
  * mingw-w64-x86_64-luajit-git
  * mingw-w64-x86_64-eigen3
  * mingw-w64-x86_64-hdf5
  * mingw-w64-x86_64-libwebsockets
5. Open a MinGW-w64 Win64 Shell, which provides a terminal with all tools set up for mingw64 development.
6. If necessary, modify the PATH environment variable in ~/.bash_profile so that executables which compete with MSYS2 cannot be loaded. Also ensure that the environment variable RAWWAVE_PATH is not set, or set to the STK source rawwaves directory. Other things may also need to be set. My ~/.bash_profile contains:
```
export CSOUND_HOME=/home/restore/csound/
export CEF_HOME=D:/cef_binary_3.2556.1368.g535c4fb_windows64
export LUA_PATH=D:\\Dropbox\\silencio\\?.lua;.\\?.lua;..\\?.lua;
export LUA_CPATH=D:\\msys2\\home\\restore\\csound\\mingw64\\csound-mingw64\\?.dll;
export OPCODE6DIR64=/home/restore/csound/mingw64/csound-mingw64
export PATH=/home/restore/csound/mingw64/csound-mingw64:/mingw64/bin:/usr/local/bin:/usr/bin:/bin
export PATH=${PATH}:/c/Program_Files_x86/Graphviz2.38/bin
export PYTHONPATH=/home/restore/csound/mingw64/csound-mingw64
export RAWWAVE_PATH=/home/restore/csound/mingw64/packages/stk/src/stk-4.5.1/rawwaves
```
7. Build and install packages for dependencies not currently in MSYS2's repositories. The formulas are included in the packages folder. Note that the purpose of these packages is simply to get dependencies installed for the Csound build system to find, not to replicate existing packages, so PKGBUILD files may be simplified. Also note that some packages contain patches that may need to be updated when source files are updated. Cd into each directory and use 'makepkg-mingw' to build the package. Use 'pacman -U name-of-package.pkg.tar.xz' to install the package. If there is a 'devel' package in the package directory, install that also. If there are any errors about line endings, simply run dos2unix on the file(s) to change the line endings.
7. Create an mingw64-compatible import library for the MSVS-built Python DLL following [instructions here](http://ascend4.org/Setting_up_a_MinGW-w64_build_environment). Note that currently, gendef comes with the MSYS2 toolchain, and does not need to be built.
8. Skip this step if you are performing the installer build: run ./build.sh in the mingw64 directory. It will run CMake, run make, and then copy the targets into the "dist" directory. For a truly clean build, first delete the csound-mingw64 directory and all of its contents.

# Installer Build

1. Of course, configure your system as for the "Plain Build", above, and perform all steps -- except do not perform the actual build. In addition...
1. Install Microsoft Visual Studio 2013, Community Edition, from [here](https://www.visualstudio.com/en-us/news/vs2013-community-vs.aspx).
2. Install the Qt SDK for 64 bit CPU architecture and MSVS 2013, from [here](http://download.qt.io/official_releases/qt/5.6/5.6.0/qt-opensource-windows-x86-msvc2013_64-5.6.0.exe).
3. Install the Chromium Embedded Framework from [here](https://cefbuilds.com/) and compile the solution using MSVS for 64 bit CPU architecture. Once you have confirmaed that the cefclient program runs, rebuild the wrapper library (libcef_dll_wrapper.lib) using the /MD (release) and /MDd (debug) compiler options, which are required by the Qt SDK.
4. Install node.js for Windows 64 bit CPU architecture from [here](https://nodejs.org/en/). This is used to build csound.node for NW.js.
4. Install NW.js for Windows 64 bit CPU architecture from [here](http://nwjs.io/). The latest version that I could get to work with csound.node is 0.12.3.
9. Set an environment variable XSL_BASE_PATH in ~/.bash_profile e.g. to mingw64/share/xml/docbook/xsl-stylesheets-1.78.1. Clone or update the Csound manual repository from git@github.com:csound/manual.git. Execute "mingw32-make html-dist" in the mingw64 shell to build the Csound Reference Manual.
10. Edit the csound/mingw64/find_csound_dependencies.py script to reflect paths for tools and resources it requires. This may need to be edited if your paths differ from mine.
11. Run ./build-mkg.sh in the mingw64 directory. It may be necessary to edit this script to reflect options or paths it requires. The first time you run this script, it should build Csound and then fail because some targets must be built with MSVS.
12. Run the VS2013 x64 native tools command prompt. Change to the csound/mingw64 directory and run the make_import_library.cmd script. This will create a Microsoft-compatible import library for the mingw64-built Csound DLL. This import library is required to build csound.node and CsoundQt.
13. Clone the CsoundQt [repository](https://github.com/CsoundQt/CsoundQt). Run QtCreator and open the CsoundQt qcs.pro project. Configure config.user.pri if necessary. Disable the shadow build option. Run qmake and rebuild the project. My config.user.pri is:
```
CONFIG *= html5
CONFIG *= perfThread_build

CEF_HOME = D:/cef_binary_3.2556.1368.g535c4fb_windows64
CSOUND_API_INCLUDE_DIR = D:/msys64/home/restore/csound/include
CSOUND_INTERFACES_INCLUDE_DIR = D:/msys64/home/restore/csound/interfaces
CSOUND_LIBRARY_DIR = D:/msys64/home/restore/csound/mingw64
INCLUDEPATH += C:\Program_Files\Mega-Nerd\libsndfile\include
LPTHREAD = D:\msys\local\opt\pthreads-w32-2-9-1-release\Pre-built.2\lib\x64\pthreadVC2.lib
LSNDFILE = C:\Program_Files\Mega-Nerd\libsndfile\lib\libsndfile-1.lib
PTHREAD_INCLUDE_DIR = D:/msys/local/opt/pthreads-w32-2-9-1-release/Pre-built.2/include
```
14. Run the node.js command prompt. Set the CSOUND_HOME environment variable to point to your Csound project root directory. Run 'nw-gyp rebuild --target=0.12.3 --arch=x64". If the script ends with "ok" it has succeeded.
15. Run csound/mingw64/build-mkg.sh again. For a truly clean build, first delete the csound-mingw64 directory and all of its contents. The build script should:
  * Run CMake.
  * Run make.
  * Run doxygen to rebuild the API documentation.
  * Run find_csound_depencencies.py to use ldd to find all actual runtime targets and dependencies of Csound (mingw64 ones, anyway).
  * Run the Inno Setup compiler to build the Csound installer for Windows x64, including CsoundQt and all the CEF, Qt, NW.js, etc. dependencies.
  * Actually install Csound so you can test it! Running Csound from the build directory may work for some features, but will not work for all features.

## Notes

* SWIG_DIR is not parsed correctly by CMake within msys2.  The reason is unknown at this time, and SWIG_DIR was hardcoded in build.sh to the appropriate location.  This is a hack, as the value will have to change if the version of SWIG is updated in the package repository for mingw64.

# NOT YET BUILDING

* Wii opcodes (requires wiiuse).
* Jack and jacko opcodes.
* C Unit tests (cunit library).

