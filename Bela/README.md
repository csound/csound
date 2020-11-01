Csound for Bela
=========================================

Csound for Bela is provided in two forms:

1. BelaCsound.cpp: setup(), render(), an cleanup() functions
to be used in Bela C++ projects, requiring linking to the Csound library

2. belacsound: a standalone executable, built from CMake with
-DBUILD_BELA=1

Build instructions
=========================================

To build Csound on the board (as root), just do, from the
top-level sources

```
$ cp Bela/Custom.cmake.bela Custom.cmake
$ mkdir build && cd build
$ cmake .. -DBUILD_BELA=1 -DUSE_DOUBLE=0 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/
$ make && make install
$ sudo ldconfig
```

Csound is installed by default in /usr/local. Other locations can be
selected by setting the CMake variable CMAKE_INSTALL_PREFIX (as done above)

Cross-compiling instructions
=========================================

A file crosscompile-setup.sh is provided to either build the pre-requisites
(xenomai and Bela libs) or at least guide you through what is needed.

1. Setup your cross-compiling toolchain for armhf. On Debian stretch
this means apt-get install arm-linux-gnueabihf-g++ cross-compile-essential-arm
libsndfile1-dev:armhf libasound-dev:armhf and possibly other things.

2. Build and install xenomai for armhf.

3. Build libbela and libbelaextra for armhf.

Once the above preparatory steps are done, you can build Csound for armhf. You
can specify `-DBELA_HOME` to point to your local copy of the Bela repo
(defaults to `~/Bela`)

```
$ cmake -DCMAKE_TOOLCHAIN_FILE=../Bela/crosscompile.cmake \
      -DUSE_DOUBLE=0 -DBUILD_BELA=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/ ..
$ make
$ make install
```

The belacsound program should appear in your binary install directory. Read access
to /root/Bela/lib and /root/Bela/include is needed even if these directories are
empty.

Distribution instructions
=========================================

Ensure you built above with `-DCMAKE_INSTALL_PREFIX=/usr/`, then you can generate a deb package with:

```
cpack -G DEB -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="armhf" -D CPACK_PACKAGE_CONTACT="Your Name <your.name@domain.com>" -D CPACK_STRIP_FILES=true
```

