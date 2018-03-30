#!/bin/sh

# change this if installation needs to go elsewhere
export PGK_CONFIG_PATH=/usr/local/lib/pkgconfig
PREFIX=/usr/local
cd build
C_FLAGS="-arch i386 -arch x86_64 -mmacosx-version-min=10.7"

# PORTAUDIO
if [ ! -f portaudio/.complete ]; then
cd portaudio
./configure --enable-mac-universal --prefix=$PREFIX
make
make install
touch .complete
cd ..
fi

# PORTMIDI
if [ ! -f portmidi-cmake/.complete ];
then

rm -rf portmidi-cmake
mkdir portmidi-cmake
cp ../patches/portmidi-CMakeLists.txt portmidi-svn/CmakeLists.txt
cp ../patches/pm_common-CMakeLists.txt portmidi-svn/pm_common/CmakeLists.txt
cp ../patches/pm_dylib-CMakeLists.txt portmidi-svn/pm_dylib/CmakeLists.txt
cd portmidi-cmake
cmake ../portmidi-svn -DCMAKE_INSTALL_PREFIX=$PREFIX
make
make install
touch .complete
cd ..

fi

# OGG
if [ ! -f libogg-1.3.0/.complete ]; then

cd libogg-1.3.0
./configure CFLAGS="${C_FLAGS}" --disable-dependency-tracking --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi

# VORBIS
if [ ! -f libvorbis-1.3.3/.complete ]; then

cd libvorbis-1.3.3
./configure CFLAGS="${C_FLAGS}" --disable-dependency-tracking --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi


# FLAC

if [ ! -f flac-1.3.2-i386/.complete ]; then

cd flac-1.3.2
./configure  --disable-dependency-tracking --disable-cpplibs --disable-asm-optimizations --enable-sse --prefix=$PREFIX
make 
make install

cd ../flac-1.3.2-i386
./configure CC="gcc -m32" --disable-dependency-tracking --disable-cpplibs --disable-asm-optimizations --enable-sse --prefix=$PREFIX 
make

#cp $PREFIX/lib/libFLAC.a libflac-x86_64.a
cp $PREFIX/libFLAC.8.dylib libflac-x86_64.dylib 

#rm $PREFIX/lib/libFLAC.a
rm $PREFIX/lib/libFLAC.8.dylib
#lipo -create src/libFLAC/.libs/libflac.a libflac-x86_64.a -output $PREFIX/lib/libFLAC.a
lipo -create src/libFLAC/.libs/libflac.8.dylib libflac-x86_64.dylib -output $PREFIX/lib/libFLAC.8.dylib

touch .complete
cd ..
fi

# LIBSNDFILE
if [ ! -f libsndfile-1.0.28/.complete ]; then
cd libsndfile-1.0.28
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking  --disable-sqlite --prefix=$PREFIX 
make || true
make install
touch .complete
cd ..
fi

# WIIUSE
if [ ! -f wiiuse_v0.13/.complete ]; then
cd wiiuse_v0.13
xcodebuild -project wiiuse.xcodeproj -sdk macosx
cp build/Release/libwiiuse.dylib $PREFIX/lib
cp wiiuse_v0.12/src/wiiuse.h $PREFIX/include
touch .complete
cd ..
fi

# FLTK 
if [ ! -f fltk-1.3.4/.complete ]; then
cd fltk-1.3.4
./configure  --with-archflags="-arch i386 -arch x86_64" --enable-shared --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi

# LIBLO 
if [ ! -f liblo-0.28/.complete ]; then
cd liblo-0.28 
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi

# LIBPNG
if [ ! -f libpng-1.6.6/.complete ]; then
cd libpng-1.6.6
./configure CFLAGS="$C_FLAGS" --disable-dependency-tracking --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi

# FLUIDSYNTH
if [ ! -f fluidsynth-1.0.9/.complete ]; then
cd fluidsynth-1.0.9
./configure CFLAGS="$C_FLAGS" --prefix=$PREFIX
make 
make install
touch .complete
cd ..
fi
