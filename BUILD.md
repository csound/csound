Building Csound from Sources
=========================

The following requirements apply to all platforms:

- CMake: Csound uses the CMake build system (cmake.org).
- C compiler toolchain: Csound is mostly written in the C language, and therefore
it requires a complete installation of C development tools.

Optional component requirements

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


Build Instructions
----

MacOS
----

On MacOS, the following specific build instructions apply. The
software is built from the terminal application. From the
top-level sources directory, the following sequence of commands
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
















