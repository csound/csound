#!/bin/sh

cd build

#C_FLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.4"

MINGW64_INSTALL_ROOT=$HOME/mingw64

export CFLAGS="-I${MINGW64_INSTALL_ROOT}/usr/local/include"
export CPPFLAGS="-I${MINGW64_INSTALL_ROOT}/usr/local/include"
export LDFLAGS="-L${MINGW64_INSTALL_ROOT}/usr/local/lib"
export PKG_CONFIG_PATH="${MINGW64_INSTALL_ROOT}/usr/local/lib/pkg-config"
export PKG_CONFIG_LIBDIR="${MINGW64_INSTALL_ROOT}/usr/local/lib"


# PORTAUDIO
if [ ! -f portaudio/.complete ]; then
cd portaudio
./configure --host=x86_64-w64-mingw32 
make
DESTDIR=$HOME/mingw64 make install
touch .complete
cd ..
fi


# PORTMIDI
#if [ ! -f portmidi-cmake/.complete ];
#then

#rm -rf portmidi-cmake
#mkdir portmidi-cmake
#cd portmidi-cmake
#cmake ../portmidi-svn -DCMAKE_TOOLCHAIN_FILE=../../Toolchain-mingw64.cmake
#make
#DESTDIR=$HOME/mingw64 make install
#touch .complete
#cd ..

#fi



# OGG

if [ ! -f libogg-1.3.0/.complete ]; then

cd libogg-1.3.0
./configure --host=x86_64-w64-mingw32 
make 
DESTDIR=$HOME/mingw64 make install
touch .complete
cd ..
fi



# VORBIS
if [ ! -f libvorbis-1.3.3/.complete ]; then

cd libvorbis-1.3.3
./configure --host=x86_64-w64-mingw32
make 
DESTDIR=$HOME/mingw64 make install
touch .complete
cd ..
fi


# FLAC

if [ ! -f flac-1.2.1/.complete ]; then

cd flac-1.2.1
./configure  --host=x86_64-w64-mingw32 --disable-cpplibs --disable-asm-optimizations --enable-sse --prefix=${MINGW64_INSTALL_ROOT}/usr/local
make 
make install
touch .complete
cd ..
fi


# LIBSNDFILE
if [ ! -f libsndfile-1.0.25/.complete ]; then
cd libsndfile-1.0.25
./configure  --host=x86_64-w64-mingw32 --disable-disable-sqlite
make 
DESTDIR=$HOME/mingw64 make install
touch .complete
cd ..
fi


exit 

# WIIUSE
if [ ! -f wiiuse_v0.13/.complete ]; then
cd wiiuse_v0.13
xcodebuild -project wiiuse.xcodeproj -sdk macosx
sudo cp build/Release/libwiiuse.dylib /usr/local/lib
sudo cp wiiuse_v0.12/src/wiiuse.h /usr/local/include
touch .complete
cd ..
fi

# FLTK 
if [ ! -f fltk-1.3.2/.complete ]; then
cd fltk-1.3.2
./configure  --with-archflags="-arch i386 -arch x86_64" --enable-shared
make 
sudo make install
touch .complete
cd ..
fi

# LIBLO 
if [ ! -f liblo-0.26/.complete ]; then
cd liblo-0.26 
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking 
make 
sudo make install
touch .complete
cd ..
fi

# LIBPNG
if [ ! -f libpng-1.6.6/.complete ]; then
cd libpng-1.6.6
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking 
make 
sudo make install
touch .complete
cd ..
fi
