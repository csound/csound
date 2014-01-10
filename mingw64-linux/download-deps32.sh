#!/bin/bash

flac_src="http://sourceforge.net/projects/flac/files/flac-src/flac-1.2.1-src/flac-1.2.1.tar.gz"
ogg_src="http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz"
vorbis_src="http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz"
libsndfile_src="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz"
liblo_src="http://sourceforge.net/projects/liblo/files/liblo/0.26/liblo-0.26.tar.gz"
#wiiuse_src="http://sourceforge.net/projects/wiiuse/files/wiiuse/v0.12/wiiuse_v0.12_src.tar.gz"
#wiiuse_src="http://codemist.co.uk/jpff/wiiuse_v0.13a.zip"
#wiiuse="http://sourceforge.net/projects/wiiuse/files/wiiuse/v0.12/wiiuse_v0.12_win.zip"
#portaudio_src="http://www.portaudio.com/archives/pa_stable_v19_20111121.tgz"
portaudio_src="http://portaudio.com/archives/pa_snapshot.tgz"
#portmidi_src="http://sourceforge.net/projects/portmedia/files/portmidi/217/portmidi-src-217.zip"
boost_src="http://sourceforge.net/projects/boost/files/boost/1.52.0/boost_1_52_0.tar.gz"
#fltk_src="http://ftp.easysw.com/pub/fltk/1.3.1/fltk-1.3.1-source.tar.gz"
fltk_src="http://fltk.org/pub/fltk/1.3.2/fltk-1.3.2-source.tar.gz"
#png_src="ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng-1.5.13.tar.gz"
png_src="http://sourceforge.net/projects/libpng/files/libpng16/1.6.7/libpng-1.6.7.tar.gz"
fluidsynth_src="http://sourceforge.net/projects/fluidsynth/files/older%20releases/fluidsynth-1.0.9.tar.gz"
luajit_src="http://luajit.org/download/LuaJIT-2.0.2.tar.gz"

pthreads="ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-9-1-release.zip"
zlib="http://zlib.net/zlib128-dll.zip"
packages=($pthreads $portaudio_src $portmidi_src $flac_src $ogg_src $vorbis_src $libsndfile_src $liblo_src $fltk_src $png_src $boost_src $fluidsynth_src $luajit_src $zlib)


#prepare
mkdir -p cache32
#rm -rf build
mkdir -p build32
mkdir -p mingw64-i686/usr/local/lib
mkdir -p mingw64-i686/usr/local/include

cd cache32
for i in ${packages[*]}; do
  echo $i
  wget -nc $i 
done

cd ../build32
for i in `ls ../cache32 | grep zip`; do
  echo "Unzipping $i"
  unzip ../cache32/$i 
done

for i in `ls ../cache32 | grep -v zip`; do 
  tar xzvf ../cache32/$i
done

cp Pre-built.2/dll/x86/pthreadGC2.dll ../mingw64-i686/usr/local/lib
cp Pre-built.2/include/* ../mingw64-i686/usr/local/include

#cp wiiuse_v0.12_win/wiiuse.dll ../mingw64/usr/local/lib
#cp wiiuse_v0.12_win/wiiuse.lib ../mingw64/usr/local/lib #cp wiiuse_v0.12_win/wiiuse.h ../mingw64/usr/local/include


cp zlib1.dll ../mingw64-i686/usr/local/lib
cp lib/* ../mingw64-i686/usr/local/lib
cp include/* ../mingw64-i686/usr/local/include/

svn co https://svn.code.sf.net/p/portmedia/code/portmidi/trunk portmidi-svn

