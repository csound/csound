#!/bin/sh

cd build

C_FLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.4"

# PORTAUDIO
if [ ! -f portaudio/.complete ]; then
cd portaudio
./configure --enable-mac-universal 
make
sudo make install
touch .complete
cd ..
fi

# PORTMIDI
if [ ! -f portmidi-cmake/.complete ];
then

rm -rf portmidi-cmake
mkdir portmidi-cmake
cp ../patches/portmidi-CMakeLists.txt portmidi-svn/CmakeLists.txt
cd portmidi-cmake
cmake ../portmidi-svn
make
sudo make install
touch .complete
cd ..

fi

# OGG

if [ ! -f libogg-1.3.0/.complete ]; then

cd libogg-1.3.0
./configure CFLAGS="${C_FLAGS}" --disable-dependency-tracking 
make 
sudo make install
touch .complete
cd ..
fi

# VORBIS
if [ ! -f libvorbis-1.3.3/.complete ]; then

cd libvorbis-1.3.3
./configure CFLAGS="${C_FLAGS}" --disable-dependency-tracking 
make 
sudo make install
touch .complete
cd ..
fi


# FLAC

if [ ! -f flac-1.2.1-i386/.complete ]; then

cd flac-1.2.1
./configure  --disable-dependency-tracking --disable-cpplibs --disable-asm-optimizations --enable-sse
make 
sudo make install

cd ../flac-1.2.1-i386
./configure CC="gcc -m32" --disable-dependency-tracking --disable-cpplibs --disable-asm-optimizations --enable-sse
make

cp /usr/local/lib/libFLAC.a libflac-x86_64.a
cp /usr/local/lib/libFLAC.8.2.0.dylib libflac-x86_64.dylib 

sudo rm /usr/local/lib/libFLAC.a
sudo rm /usr/local/lib/libFLAC.8.2.0.dylib
sudo lipo -create src/libFLAC/.libs/libflac.a libflac-x86_64.a -output /usr/local/lib/libFLAC.a
sudo lipo -create src/libFLAC/.libs/libflac.8.2.0.dylib libflac-x86_64.dylib -output /usr/local/lib/libFLAC.8.2.0.dylib

touch .complete
cd ..
fi

# LIBSNDFILE
if [ ! -f libsndfile-1.0.25/.complete ]; then
cd libsndfile-1.0.25
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking  --disable-sqlite
make || true
sudo make install
touch .complete
cd ..
fi

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
