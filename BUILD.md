Building Csound from Sources
=========================

The following requirements apply to all platforms:

- CMake: Csound uses the CMake build system (cmake.org).
- C compiler toolchain: Csound is mostly written in the C language, and therefore
it requires a complete installation of C development tools.
- Flex and bison: the Csound parser requires bison and flex to be
  installed. Recent versions are recommended (e.g. bison 3.8 and flex 2.6)

Optional component requirements:

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


Build Instructions
----

MacOS
----

On MacOS, the following specific build instructions apply. The
software is built from the terminal application. Note that while
MacOS contains bison and flex, Csound requires a newer version
of bison to be installed (see https://www.gnu.org/software/bison/).

From the top-level sources directory, the following sequence of commands
can be used to build the system.  It assumes that libsndfile has been
installed in the system

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

MacOS using Brew
----

Alternatively, on MacOS we may use `brew` (https://brew.sh/) to install all dependencies
and then build Csound.

```
brew install --only-dependencies csound
brew install bison flex asio jack googletest
cmake -B build -DCUSTOM_CMAKE="./platform/osx/custom-osx.cmake
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















