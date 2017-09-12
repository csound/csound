# Building with msys2 and Mingw64

The following are instructions for building Csound for 64-bit Windows (x86_64) using msys2 and mingw64.

These instructions come in two parts. The first is for building a "plain" Csound that the build will install into a "dist" directory. This build is simple, and Csound will run in the "dist" directory. The second is for building all features of Csound that are available for Windows 64 bit CPU architecture, then building an Inno Setup installer for them, then running the installer. This build is considerably more complex.

Note that MSYS2 can also be used to create 32bit binaries. Simply use the MinGW-w64 Win32 Shell instead of the Win64 Shell. See below for details.

If you want all Csound plugins to be built with static linkage to all library dependencies, you must set `STK_LIBRARY` and `PTHREAD_LIBRARY` in your `build.sh` or `build-mkg.sh` to specify the full pathname to the static version of the respective library.

## Plain Build

1. Install the x86_64 version of [msys2](https://msys2.github.io/). Follow the pacman setup instructions carefully.

2. Using pacman, install (pacman -S package_name) the following additional development tools and libraries (not all of these tools are needed in order to build Csound. It depends on what 3rd party tools you require. For example, luajit, boost, fltk, eigen3, hdf5, fluidsynth, libwebsockets, etc. are all optional):
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

3. Open a MinGW-w64 Win64 Shell, which provides a terminal with all tools set up for mingw64 development.

4. If necessary, modify the PATH environment variable in `~/.bash_profile` so that executables which compete with MSYS2 cannot be loaded. Also ensure that the environment variable RAWWAVE_PATH is not set, or set to the STK source rawwaves directory. Other things may also need to be set. My `~/.bash_profile contains`:
    ```sh
    export CSOUND_HOME=/home/restore/csound/
    export CEF_HOME=D:/cef_binary_3.2556.1368.g535c4fb_windows64
    export LUA_PATH=D:\\Dropbox\\silencio\\?.lua;.\\?.lua;..\\?.lua;
    export LUA_CPATH=D:\\msys64\\home\\restore\\csound\\mingw64\\csound-mingw64\\?.dll;
    export OPCODE6DIR64=D:/msys64/home/restore/csound/mingw64/csound-mingw64
    export PATH=/home/restore/csound/mingw64/csound-mingw64:/mingw64/bin:/usr/local/bin:/usr/bin:/bin
    export PATH=${PATH}:/c/Program_Files_x86/Graphviz2.38/bin
    export PATH=${PATH}:/C/Program_Files/Java/jdk1.7.0_75/bin
    export PYTHONPATH=/home/restore/csound/mingw64/csound-mingw64
    export RAWWAVE_PATH=/home/restore/csound/mingw64/packages/stk/src/stk-4.5.1/rawwaves
    ```

5. Build and install packages for dependencies not currently in MSYS2's repositories. The formulas are included in the packages folder. Note that the purpose of these packages is simply to get dependencies installed for the Csound build system to find, not to replicate existing packages, so PKGBUILD files may be simplified. Also note that some packages contain patches that may need to be updated when source files are updated. Cd into each directory and use `makepkg-mingw -f` to build the package. Use `pacman -U name-of-package.pkg.tar.xz` to install the package. If there is a 'devel' package in the package directory, install that also. If there are any errors about line endings, simply run dos2unix on the file(s) to change the line endings. Failing that you can simply run this command from the package directory:
    ```sh
    sed -i 's/^M//' PKGBUILD
    ```

6. If you wish to create an mingw64-compatible import library for the MSVS-built Python DLL, see the following [instructions here](http://ascend4.org/Setting_up_a_MinGW-w64_build_environment). Note that currently, gendef comes with the MSYS2 toolchain, and does not need to be built.

7. Run `./build.sh` in the mingw64 directory. It will run CMake, run make, and then copy the targets into the "dist" directory. For a truly clean build, first delete the `csound-mingw64` directory and all of its contents. (You can skip this step if you are performing the installer build below.)

# Installer Build

1. Of course, configure your system as for the "Plain Build", above, and perform all steps -- except do not perform the actual build. In addition...

2. Install the following dependencies, which are not or should not be built with mingw64. The build system should be able to find them without manual configuration.
  * The Java SDK for x64 from [Oracle](http://www.oracle.com/technetwork/java/index.html).
  * Python for x64 from [Python.org](https://www.python.org/).
  * The [Ableton Link software development kit](https://github.com/Ableton/link).

3. Obtain from [Steinberg](http://www.steinberg.net/en/company/developers.html) the ASIO2 and VST 2.X SDKs and copy them into the mingw64/include directory.

4. Install [Graphviz](http://www.graphviz.org/) and add its bin dir to the PATH in ~/.bash_profile.

5. Install Microsoft Visual Studio 2015, Community Edition Update 2 or later, from [here](https://www.visualstudio.com/en-us/news/vs2013-community-vs.aspx).

6. Install the Qt SDK for 64 bit CPU architecture and MSVS 2015, from [here](http://download.qt.io/development_releases/qt/5.8/5.8.0-beta/qt-opensource-windows-x86-msvc2015_64-5.8.0-beta.exe).

7. Install node.js for Windows 64 bit CPU architecture from [here](https://nodejs.org/en/). This is used to build csound.node for NW.js.

8. Install NW.js for Windows 64 bit CPU architecture from [here](http://nwjs.io/).

9. Set an environment variable `XSL_BASE_PATH` in `~/.bash_profile` e.g. to `mingw64/share/xml/docbook/xsl-stylesheets-1.78.1`. Clone or update the Csound manual repository from git@github.com:csound/manual.git. Execute `python csd2docbook.py -a` to build docbook versions of the example CSDs. Execute `mingw32-make html-dist` in the mingw64 shell to build the Csound Reference Manual.

10. Execute `./build-mkg.sh` in the mingw64 directory. It may be necessary to edit this script to reflect options or paths it requires. The first time you run this script, it should build Csound and then fail because some targets must be built with MSVS.

11. Load the csound/Opcodes/AbletonLinkOpcodes/AbletonLinkOpcodes.sln solution in Visual Studio 2015 and build the release version of the project for x64. Mingw64 will build these opcodes but they don't work, Visual Studio will build a version that does work.

11. Execute the VS2015 x64 native tools command prompt. Change to the csound/mingw64 directory and run the `make_import_library.cmd` script. This will create a Microsoft-compatible import library for the mingw64-built Csound DLL. This import library is required to build csound.node and CsoundQt.

12. Clone the CsoundQt [repository](https://github.com/CsoundQt/CsoundQt). Run QtCreator and open the CsoundQt `qcs.pro` project. Configure `config.user.pri` if necessary. Disable the shadow build option. Run qmake and rebuild the project. My `config.user.pri` is:
    ```qmake
    CONFIG *= perfThread_build
    CONFIG *= rtmidi

    CSOUND_API_INCLUDE_DIR = D:/msys64/home/restore/csound/include
    CSOUND_INTERFACES_INCLUDE_DIR = D:/msys64/home/restore/csound/interfaces
    CSOUND_LIBRARY_DIR = D:/msys64/home/restore/csound/mingw64
    INCLUDEPATH += D:/msys64/home/restore/Mega-Nerd/libsndfile/include
    INCLUDEPATH += D:/msys64/home/restore/rtmidi-2.1.1
    LPTHREAD = D:/msys64/home/restore/pthreads-w32-2-9-1-release/Pre-built.2/lib/x64/pthreadVC2.lib
    LSNDFILE = D:/msys64/home/restore/Mega-Nerd/libsndfile/lib/libsndfile-1.lib
    PTHREAD_INCLUDE_DIR = D:/msys64/home/restore/pthreads-w32-2-9-1-release/Pre-built.2/include
    RTMIDI_DIR = D:/msys64/home/restore/rtmidi-2.1.1
    ```

14. Run the node.js command prompt. Set the `CSOUND_HOME` environment variable to point to your Csound project root directory. Run `nw-gyp rebuild --target=0.22.3 --arch=x64`. If the script ends with "ok" it has succeeded.

15. Run `csound/mingw64/build-mkg.sh` again. For a truly clean build, first delete the `csound-mingw64` directory and all of its contents. The build script should:
  * Run CMake.
  * Run make.
  * Run doxygen to rebuild the API documentation.
  * Run `find_csound_depencencies.py` to use ldd to find all actual runtime targets and dependencies of Csound (mingw64 ones, anyway).
  * Run the Inno Setup compiler to build the Csound installer for Windows x64, including CsoundQt and all the CEF, Qt, NW.js, etc. dependencies.
  * Actually install Csound so you can test it! Running Csound from the build directory may work for some features, but will not work for all features.

16. In the Csound installation's command shell (the Csound icon on the Start menu), run the following pieces to perform a variety of quick tests of some major features.
  * Run `examples/xanadu.csd` with command-line Csound to test basic functionality.
  * Run `examples/python/drone.py` and `TrappedCsd.py` with Python to test the Python interface.
  * Run `examples/java/CSDPlayer.jar` with java to test the Java interface.
  * Run `examples/opcode_demos/lua_scoregen.csd` with LuaJIT to test the Lua opcodes.
  * Run `examples/html/GameOfLife3D.csd` in CsoundQt to test CsoundQt, the HTML5 integration, the signal flow graph opcodes, and the vst4cs or Fluid opcodes.
  * Run examples/html/NW_Csound_Demo.html in NW.js to test csound.node.

## Notes

* SWIG_DIR is not parsed correctly by CMake within msys2.  The reason is unknown at this time, and SWIG_DIR was hardcoded in build.sh to the appropriate location.  This is a hack, as the value will have to change if the version of SWIG is updated in the package repository for mingw64.

# NOT YET BUILDING

* Wii opcodes (requires wiiuse).
* Jack and jacko opcodes.
* C Unit tests (cunit library).
