#!/bin/sh

mkdir -p cache 
mkdir -p deps 

cd cache 

wget -nc http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip 
#wget -nc ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-9-1-release.zip
#wget -nc https://sourceforge.net/projects/winflexbison/files/latest/download 
#wget -nc https://dl.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip
wget -nc https://downloads.sourceforge.net/project/winflexbison/win_flex_bison-latest.zip 
wget -nc https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.tar.bz2 

cd ../deps

unzip ../cache/libsndfile-1.0.27-w64.zip
mkdir -p temp
#cd temp
#unzip ../../cache/pthreads-w32-2-9-1-release.zip
#cp Pre-built.2/lib/x64/* ../lib/
#cp Pre-built.2/dll/x64/* ../bin/
#cp Pre-built.2/include/* ../include/
#cd ..
mkdir -p win_flex_bison
cd win_flex_bison
unzip ../../cache/win_flex_bison-latest.zip
