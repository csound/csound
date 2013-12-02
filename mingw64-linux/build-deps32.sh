#!/bin/sh

export MINGW64_INSTALL_ROOT=`pwd`/mingw64-i686
echo $MINGW64_INSTALL_ROOT

cd build32

mkdir -p $MINGW64_INSTALL_ROOT

export CFLAGS="-I${MINGW64_INSTALL_ROOT}/usr/local/include"
export CPPFLAGS="-I${MINGW64_INSTALL_ROOT}/usr/local/include"
export LDFLAGS="-L${MINGW64_INSTALL_ROOT}/usr/local/lib"
export PKG_CONFIG_PATH="${MINGW64_INSTALL_ROOT}/usr/local/lib/pkg-config"
export PKG_CONFIG_LIBDIR="${MINGW64_INSTALL_ROOT}/usr/local/lib"


# PORTAUDIO
if [ ! -f portaudio/.complete ]; then
cd portaudio
./configure --host=i686-w64-mingw32  --prefix=$MINGW64_INSTALL_ROOT/usr/local
make
make install
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
./configure --host=i686-w64-mingw32 --prefix=$MINGW64_INSTALL_ROOT/usr/local
make 
make install
touch .complete
cd ..
fi



# VORBIS
if [ ! -f libvorbis-1.3.3/.complete ]; then

cd libvorbis-1.3.3
./configure --host=i686-w64-mingw32 --prefix=$MINGW64_INSTALL_ROOT/usr/local
make 
make install
touch .complete
cd ..
fi


# FLAC

if [ ! -f flac-1.2.1/.complete ]; then

cd flac-1.2.1
./configure  --host=i686-w64-mingw32 --disable-cpplibs --disable-asm-optimizations --enable-sse --prefix=${MINGW64_INSTALL_ROOT}/usr/local
make 
make install
touch .complete
cd ..
fi


# LIBSNDFILE
if [ ! -f libsndfile-1.0.25/.complete ]; then
cd libsndfile-1.0.25
./configure  --host=i686-w64-mingw32 --disable-disable-sqlite --prefix=$MINGW64_INSTALL_ROOT/usr/local
make 
make install
touch .complete
cd ..
fi

# FLTK 
if [ ! -f fltk-1.3.2/.complete ]; then
cd fltk-1.3.2 
./configure  --host=i686-w64-mingw32 --prefix=$MINGW64_INSTALL_ROOT/usr/local --enable-threads --enable-gl --enable-shared --enable-localjpeg --enable-localzlib --enable-localpng
make || true
make install
# touch .complete
cd ..
fi


# LIBLO 
#if [ ! -f liblo-0.26/.complete ]; then
#cd liblo-0.26 
#./configure  --host=i686-w64-mingw32 --prefix=$MINGW64_INSTALL_ROOT/usr/local
#make install
##touch .complete
#cd ..
#fi

exit

# LIBPNG
if [ ! -f libpng-1.6.7/.complete ]; then
cd libpng-1.6.7
./configure  --host=i686-w64-mingw32 --prefix=$MINGW64_INSTALL_ROOT/usr/local 
make install
##touch .complete
cd ..
fi
