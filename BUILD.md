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

- `USE_LIBSAMPLERATE=`: controls whether libsamplerate (Secret Rabbit Code) 
is used for high-quality sample rate conversion. When disabled, Csound will
use linear interpolation for sample rate conversion. Default is ON.

- `BUILD_PLUGINS=`: build all external opcodes as plugin libraries.
This option is disabled by default.

- `USE_WASMTIME=`: allow CMake to build native WebAssembly opcode support when
it finds the Wasmtime C API. This option is enabled by default. Set it to
`OFF` to disable the feature even when Wasmtime is installed. Set
`Wasmtime_ROOT` if the Wasmtime package is outside CMake's normal search paths.
See [Native Csound WebAssembly plugins](platform/wasm-wasi/NATIVE_OPCODE_PLUGINS.md).

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
cd platform/ios
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
portaudio19-dev libportmidi-dev libpulse-dev swig \
default-jdk liblo-dev \
bison flex \
gettext libsamplerate0-dev
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

If you wish to build the installer, download the Visual C++ Redistributable installer (pinned to VS 2022 / `vs/17` stable, bundled offline) and then run Inno Setup. The installer will run `vc_redist.x64.exe` silently during Csound installation so loose VCRUNTIME DLLs are no longer shipped (avoids Windows Defender false positives). The redistributable requires admin elevation (`PrivilegesRequired=admin`).

For 64-bit:

```powershell
Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vc_redist.x64.exe -OutFile installer/windows/vc_redist.x64.exe
iscc /o. installer\windows\csound7_x64_github.iss
```

For 32-bit:

```powershell
Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vc_redist.x86.exe -OutFile installer/windows/vc_redist.x86.exe
iscc /o. installer\windows\csound7_x86_github.iss
```

### Windows library names

Csound 7 uses `csound7_64.dll` for double-precision samples and `csound7.dll`
for single-precision samples. The `_64` suffix refers to sample precision,
not CPU architecture. The x64 and ARM64 installers use double precision;
the x86 installer uses single precision.

Link against `csound7_64.lib` or `csound7.lib`, or use the CMake target
`Csound::Csound`. Hosts that load the DLL at runtime must use the new name.
Rebuild Csound 7 hosts and plugins that link against the old DLL name.
Do not alias the new DLL to `csound64.dll` or `csound.dll`: those names
belong to Csound 6 and would restore the conflict.

When updating a registered Csound 7 installation in the same folder, the
installer removes its old DLL and library names. For a manual ZIP update,
remove those old files from the Csound 7 folder yourself. Keep Csound 6
and Csound 7 in separate folders. The command-line program remains
`csound.exe`, so PATH order still chooses which version runs.

### Windows ARM64

The ARM64 package runs natively on Windows on Arm. It includes the command-line
tools, Csound library, plugins and headers. Applications that load the library
and any extra plugins must also target ARM64.

Build on Windows on Arm with Visual Studio 2026, the ARM64 C++ build tools and a
Windows SDK. Install the prerequisites and initialize vcpkg as described above. Use a fresh checkout
or remove an old `build` directory before changing architectures.

```powershell
cmake -B build -S . -G "Visual Studio 18 2026" -A ARM64 -DUSE_VCPKG=1 -DCUSTOM_CMAKE="./platform/windows/Custom-vs.cmake"
cmake --build build --config Release
cmake --install build --prefix build\install --config Release
```

CMake selects `arm64-windows-static` for the dependencies. The CI job uses a
native `windows-11-arm` runner and runs the existing CSD and C++ tests there.
The installer and binaries are available as workflow artifacts and release
assets.

Set the version and build number before packaging a local build. Download the
ARM64 Visual C++ runtime and compile the installer with Inno Setup 6.3 or later.

```powershell
$env:CSOUND_VERSION = "7.0-local"
$env:GITHUB_RUN_NUMBER = "1"
Invoke-WebRequest -Uri https://aka.ms/vc14/vc_redist.arm64.exe -OutFile installer/windows/vc_redist.arm64.exe
iscc /o. installer\windows\csound7_arm64_github.iss
```

The installer accepts only ARM64 Windows and uses `C:\Program Files\Csound7 ARM64`.
It bundles the ARM64 runtime for offline installation. The Windows installers
share `PATH`, `OPCODE7DIR64` and `RAWWAVE_PATH`. The last installation sets
`OPCODE7DIR64` and `RAWWAVE_PATH` for all copies. If you keep an x64 installation
too, use the full path to the desired executable and set `OPCODE7DIR64` and
`RAWWAVE_PATH` to its `plugins64` and `samples` directories.

Uninstalling a copy leaves plugin and sample paths that point elsewhere alone.
If they point to the copy being removed, the uninstaller clears them. Set them
again for any copy you keep.

If you wish to customize the build in any way, you can modify the `Custom-vs.cmake` file in the `platform\windows` directory. For common options, refer to "Useful CMake Options" above.

### Windows audio backends

The x64 and ARM64 installers include PortAudio, WASAPI and Windows MME.
Use `-+rtaudio=wasapi` to select the native WASAPI backend. It uses exclusive
mode when the device accepts Csound's rate, channel count and float format.
Otherwise it tries shared mode with Windows audio engine conversion enabled
through `AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM`.

Shared mode requires the audio engine to accept this conversion request for
playback or capture. If it cannot, Csound reports the HRESULT and stops opening
the stream. It does not fall back to an unconverted device format, which could
change pitch, duration or channel layout. Try `-+rtaudio=pa` to use PortAudio
instead. The Windows ARM64 workflow uses Windows 11.
















