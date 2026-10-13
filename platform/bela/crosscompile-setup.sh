#!/bin/sh
# cross-compile set up for belacsound
# assumes you have the armhf compilers and build essentials
# also libc-dev:armhf, linux-libc-dev:armhf and libstdc++-dev:armhf
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
git clone https://git.xenomai.org/xenomai-3.git
cd xenomai-3
scripts/bootstrap
mkdir build
cd build
../configure --with-core=cobalt --enable-pshared --host=arm-linux-gnueabihf --build=arm CFLAGS="-march=armv7-a -mfpu=vfp3" --enable-dlopen-libs
# this step installs xenomai in /usr/xenomai
sudo make install
# build pasm
cd ../..
git clone git@github.com:beagleboard/am335x_pru_package.git
cd am335x_pru_package
make CC=$CC
# this step installs pasm in /usr/local
sudo make install
# build NE10
cd ..
git clone https://github.com/projectNe10/Ne10
cd Ne10
mkdir build
cd build
cmake -DGNULINUX_PLATFORM=ON ..
make
# build seasocks, needs libz1g:armhf
cd ../..
git clone https://github.com/mattgodbolt/seasocks.git
cd seasocks
mkdir build
cd build
cmake ..
make
# this step installs libseasocks in /usr/local
sudo make install
# build Bela
cd ../..
git clone https://github.com/BelaPlatform/Bela
cd Bela
cp ../Ne10/build/modules/libNE10.a ./lib/.
mkdir -p /build/core
# apply patch to remove offending LDFLAG
patch Makefile bela-makefile.patch
make CC=$CC CXX=$CXX CPPFLAGS="-I/usr/local/include" lib 
# now the Bela libs are in the Bela/lib directory

