Building Csound from Sources
=========================

The following requirements apply to all platforms:

- CMake: Csound uses the CMake build system (cmake.org).
- C compiler toolchain: Csound is mostly written in the C language, and therefore
it requires a complete installation of C development tools.
- Flex and bison: the Csound parser requires bison and flex to be
  installed. Recent versions are recommended (e.g. bison 3.8 and flex 2.6)

Optional component requirements:

- Git: A distributed version control system to let you clone and acquire
the Csound sources. Although you can download the source as a zip from GitHub,
having git installed makes it easier to interact with the Csound source. It
also makes it easier to sync with the very latest source.

- C++ compiler toolchain: a few components are written in C++. For
these to be built, a complete installation of C++ development tools is
necessary.

- Libsndfile: for soundfile IO, Csound uses libsndfile
(https://libsndfile.github.io/libsndfile/).
For this functionality to be present, this library needs to be
installed in the system. NB: this requirement can be disabled in the
CMake build with the `USE_LIBSNDFILE=0` option (see below)


Useful CMake options
--------------

The following options may be useful to configure the build according
to the local platform conditions.

- `CMAKE_INSTALL_PREFIX=`: this allows the installation to be placed
in a custom location (defaults to `/usr/local`). The option takes the
top-level installation directory.

- `USE_LIBSNDFILE=`: in systems where libsndfile is not present, this
option can disable this requirement for the build.

- `BUILD_PLUGINS=`: build all external opcodes as plugin libraries.
This option is disabled by default.

-`DCUSTOM_CMAKE=`: with this option you can specify a custom.cmake
file containing build options and CMake variables to control the build.

A full list of CMake build options can be found by running the following
command from the CMake build directory:

```
cmake -LAH
```

Build Instructions
----

*Steps for building Csound across a large range of target platforms can be found in the [csound_builds.yml](https://github.com/csound/csound/blob/develop/.github/workflows/csound_builds.yml) script, which is part of Csound's CI build system.*


MacOS with dependencies installed
----

On MacOS, the following specific build instructions apply. The
software is built from the terminal application. Note that while
MacOS contains bison and flex, Csound requires a newer version
of bison to be installed (see https://www.gnu.org/software/bison/).

From the top-level sources directory, the following sequence of commands
can be used to build the system.  It assumes that libsndfile and other
dependencies have been installed in the system

```
mkdir build
cd build
cmake ..
make -j 8
```

To install the software, super-user access is normally required as the
default installation location includes directories under `/usr/local`

```
sudo make install
```

The CsoundLib64 (or CsoundLib) framework is installed in the local
user Library/Frameworks ($HOME/Library/Frameworks).

See "Useful CMake Options" above for common options to customise the
build.

MacOS Vanilla
--------

On MacOS, with no dependencies installed and no homebrew, the following commands
apply. From the top-level sources directory,

```
git clone https://github.com/libsndfile/libsndfile
cd libsndfile
cmake -B build -DENABLE_EXTERNAL_LIBS=0 -DENABLE_MPEG=0 -DCMAKE_INSTALL_PREFIX=../sndfile_install
cmake --build build
cmake --build build --target install
cd ..
cmake -B build -DCMAKE_PREFIX_PATH="$PWD/sndfile_install" -DCMAKE_INSTALL_PREFIX="$PWD/csound_install" -DCS_FRAMEWORK_DEST="$PWD/csound_install"
cmake --build build --config Release
cmake --build build --target install
```

This will install in the `csound_install` subdirectory of the sources
tree. This installation location can be changed if desired by
replacing it in the relevant command.

Csound will be fully functional, with CoreAudio and CoreMIDI
support, and libsndfile (statically linked) and all its command-line
programs. Note that 


MacOS using Brew
----

Alternatively, on MacOS we may use `brew` (https://brew.sh/) to install all dependencies
and then build Csound.

```
brew install --only-dependencies csound
brew install bison flex jack googletest
cmake -B build -DCUSTOM_CMAKE="./platform/osx/custom-osx.cmake"
cmake --build build --config Release
```

To install the software, super-user access is normally required as the
default installation location includes directories under `/usr/local`

```
sudo make install
```

The CsoundLib64 (or CsoundLib) framework is installed in the local
user Library/Frameworks ($HOME/Library/Frameworks).

See "Useful CMake Options" above for common options to customise the
build.

iOS
----

iOS requires libsndfile to be built for the usual targets. A script
is provided for this. It also requires a newer version of bison to
be installed (`brew install bison` is an alternative to building from
upstream sources).

```
cd iOS
chmod +x build_libsndfile.sh
./build_libsndfile.sh
build.sh
release.sh
```

The release script creates a zip file containing the iOS build.

Ubuntu Linux
------

On Ubuntu Linux, we first make sure all required packages are
installed, then we run the normal cmake build.


```
sudo apt-get update
sudo apt-get install cmake libsndfile1-dev libasound2-dev libjack-dev \
portaudio19-dev libportmidi-dev libpulse-dev swig liblua5.1-0-dev \
default-jdk libfltk1.1-dev libfluidsynth-dev liblo-dev fluid \
ladspa-sdk libpng-dev dssi-dev libstk0-dev libgmm++-dev bison flex \
libportsmf-dev libeigen3-dev libcunit1-dev gettext libsamplerate0-dev
mkdir build
cd build
cmake ..
make
```
To install the software, super-user access is normally required as the
default installation location includes directories under `/usr/local`

```
sudo make install
```

See "Useful CMake Options" above for common options to customise the
build.

Windows (Visual Studio / vcpkg)
----

You will need a recent version of Visual Studio to build Csound. The current default version is Visual Studio 2022. Additionally, you will need CMake, Flex, and Bison. The Chocolatey package manager for Windows is the simplest way to install these prerequisites. If you wish to build the installer, you will also need InnoSetup. For cloning the Csound source from the GitHub repository, you'll need git, although you can also download the latest source as a zip file.

To install the required packages, open Windows PowerShell as an administrator and run the following commands (note that InnoSetup and git are optional):

*Note: All the following commands should be run in Windows PowerShell*

```powershell
choco install -y cmake winflexbison3 innosetup git
```

To download the Csound source with git, use the following command:

```powershell
git clone https://github.com/csound/csound.git
```

The vcpkg package manager is used to build and manage any extra dependencies that Csound needs, such as libsndfile, portaudio, and portmidi. The `vcpkg` submodule must be initialized and updated before building. From the top-level source directory, run these commands:

```powershell
git submodule init
git submodule update
.\vcpkg\bootstrap-vcpkg.bat
```

Next, run the following CMake commands to generate the Visual Studio projects and build Csound. If you have Python 3 installed, you can remove the `-DINSTALL_PYTHON_INTERFACE=OFF` option from the first command:

```powershell
cmake -B build -S . -DUSE_VCPKG=1 -DCUSTOM_CMAKE="./platform/windows/Custom-vs.cmake" -DINSTALL_PYTHON_INTERFACE=OFF
cmake --build build --config Release
```

If you wish to build the installer, use the following commands. The first four set the location of the MS C/C++ runtime libraries that need to be packaged with Csound. Note that the `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build` path might need updating depending on your installed version of Visual Studio:

```powershell
$Env:RedistVersion = Get-Content "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\Microsoft.VCRedistVersion.default.txt"
$Env:VCREDIST_CRT_DIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\${Env:RedistVersion}\x64\Microsoft.VC143.CRT"
$Env:VCREDIST_CXXAMP_DIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\${Env:RedistVersion}\x64\Microsoft.VC143.CXXAMP"
$Env:VCREDIST_OPENMP_DIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\${Env:RedistVersion}\x64\Microsoft.VC143.OpenMP"
iscc /o. installer\windows\csound7_x64_github.iss
```

If you wish to customize the build in any way, you can modify the `Custom-vs.cmake` file in the `platform\windows` directory. For common options, refer to "Useful CMake Options" above.

















