#!/bin/sh

set -x

if [ $# == 0 ]; then
  echo "Must give branch name to build from"
  exit
else
  export BRANCH_NAME=$1
  echo "Using branch: $BRANCH_NAME"
fi

if [ $# -gt 1 ]; then
  export CS_VERSION=$2
else
  export CS_VERSION="6"
fi
echo "Version: $CS_VERSION"
export MANUAL_DIR=`pwd`/../../../manual
export PACKAGE_NAME=csound-MacOS_x86_64-${CS_VERSION}.pkg
export DMG_DIR="Csound${CS_VERSION}"
export DMG_NAME="csound-MacOS_x86_64-${CS_VERSION}.dmg"

#export SDK=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/
export TARGET=10.7
export DEPS_BASE=/usr/local
# If arg2 passed in, will cd into that dir and rebuild, otherwise
# will clone from repo and do a fresh build

if [ $# -gt 2 ]; then
	cd $3
	echo "Using directory $3 `pwd`"
        export INSTALLER_DIR=`pwd`/installer
        rm -rf installer 
	rm -rf csound6/build/dist
        mkdir installer
else
	export RELEASE_DIR="`eval date +%Y-%m-%d-%H%M%S`"
        export INSTALLER_DIR=`pwd`/$RELEASE_DIR/installer
	mkdir $RELEASE_DIR 
        mkdir $INSTALLER_DIR
	cd $RELEASE_DIR

	#git clone git://csound.git.sourceforge.net/gitroot/csound/csound5
	git clone -b $BRANCH_NAME file://$PWD/../../.. csound6 --depth=1 
	#cp -R csound5 csound5-f
fi

#BUILD DOUBLES CSOUND
echo "Building Csound (double)..."
cd csound6
cp ../../Custom_10.9.cmake Custom.cmake 

#$DEPS_BASE/bin/scons -j2 &> ../csound5_build_log.txt
mkdir build
cd build
export BUILD_DIR=`pwd`
# i386 is now deprecated, so we're not building it anymore
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=0 -DBUILD_FAUST_OPCODES=1 -DFAUST_LIBRARY=$DEPS_BASE/lib/libfaust.a  -DBUILD_STK_OPCODES=1 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DUSE_GETTEXT=0
# RUN CMAKE TWICE TO GET AROUND ISSUE WITH UNIVERSAL BUILD
#cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="i386;x86_64" -DBUILD_TESTS=0 -DBUILD_FAUST_OPCODES=1 -DFAUST_LIBRARY=$DEPS_BASE/lib/libfaust.a -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET -DCMAKE_OSX_SYSROOT=$SDK -DBUILD_STK_OPCODES=1 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DUSE_GETTEXT=0
make -j6 install

cd ../..

# ASSEMBLE FILES FOR INSTALLER
export CSLIBVERSION=6.0
#export APPS32_DIR=$INSTALLER_DIR/CsoundApps/Package_Contents/usr/local/bin
export APPS64_DIR=$INSTALLER_DIR/CsoundApps64/Package_Contents/usr/local/bin
#export FRAMEWORK32_DIR=$INSTALLER_DIR/CsoundLib/Package_Contents/Library/Frameworks/CsoundLib.framework
export FRAMEWORK64_DIR=$INSTALLER_DIR/CsoundLib64/Package_Contents/Library/Frameworks/CsoundLib64.framework

export SUPPORT_LIBS_DIR=$FRAMEWORK64_DIR/libs

export PYTHON_DIR=Versions/$CSLIBVERSION/Resources/Python/Current
export TCLTK_DIR=Versions/$CSLIBVERSION/Resources/TclTk
export JAVA_DIR=Versions/$CSLIBVERSION/Resources/Java
export SAMPLES_DIR=Versions/$CSLIBVERSION/Resources/samples

export DIST=csound6/build/dist
export BLD=csound6/build

#mkdir -p $APPS32_DIR
mkdir -p $APPS64_DIR
#mkdir -p $FRAMEWORK32_DIR
mkdir -p $FRAMEWORK64_DIR
mkdir -p $SUPPORT_LIBS_DIR


mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$JAVA_DIR
#mkdir -p $FRAMEWORK64_DIR/$SAMPLES_DIR
mkdir -p $FRAMEWORK64_DIR/../Documentation
#mkdir -p $FRAMEWORK64_DIR/Headers

cp -R $DIST/CsoundLib64.framework/ $FRAMEWORK64_DIR
#cp -R csound5-f/CsoundLib.framework/ $FRAMEWORK32_DIR

echo "Copying Python Libs... $PWD"

cp $DIST/../_csnd6.so $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/../csnd6.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp $BLD/../interfaces/ctcsound.py $FRAMEWORK64_DIR/$PYTHON_DIR


echo "preparing framework..."

cp  $DIST/lib/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/$CSLIBVERSION/
cp  $DIST/lib/csnd6.jar $FRAMEWORK64_DIR/$JAVA_DIR
cp  $DIST/lib/lib_jcsound6.jnilib $FRAMEWORK64_DIR/$JAVA_DIR

# fix ID to /Library/Frameworks/CsoundLib64.framework/CsoundLib64
# as CMake introduces an undesirable @rpath
install_name_tool -id CsoundLib64.framework/CsoundLib64 $FRAMEWORK64_DIR/CsoundLib64

echo "copying manual..."

#cp -Rf $MANUAL_DIR/html $FRAMEWORK32_DIR/Resources/
#mv $FRAMEWORK32_DIR/Resources/html $FRAMEWORK32_DIR/Resources/Manual

cp -Rf $MANUAL_DIR/html $FRAMEWORK64_DIR/Resources/
mv $FRAMEWORK64_DIR/Resources/html $FRAMEWORK64_DIR/Resources/Manual

# installer should take care of this now (VL 07.20)
#echo "copying samples..."

#cp csound6/samples/*.dat $FRAMEWORK64_DIR/$SAMPLES_DIR

echo "copying apps..."

cp $DIST/bin/* $APPS64_DIR

echo "copying support libs..."
cp $DEPS_BASE/lib/libfltk.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/liblo.7.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libsamplerate.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libportmidi.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libpng16.16.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libFLAC.8.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libogg.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfluidsynth.1.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libwiiuse.dylib $SUPPORT_LIBS_DIR
 
# chnage IDs
install_name_tool -id libfltk.1.3.dylib $SUPPORT_LIBS_DIR/libfltk.1.3.dylib
install_name_tool -id libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -id libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib
install_name_tool -id liblo.7.dylib $SUPPORT_LIBS_DIR/liblo.7.dylib
install_name_tool -id libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -id libsamplerate.0.dylib $SUPPORT_LIBS_DIR/libsamplerate.0.dylib
install_name_tool -id libportaudio.2.dylib $SUPPORT_LIBS_DIR/libportaudio.2.dylib
install_name_tool -id libportmidi.dylib $SUPPORT_LIBS_DIR/libportmidi.dylib
install_name_tool -id libpng16.16.dylib $SUPPORT_LIBS_DIR/libpng16.16.dylib
install_name_tool -id libFLAC.8.dylib $SUPPORT_LIBS_DIR/libFLAC.8.dylib
install_name_tool -id libvorbisenc.2.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib
install_name_tool -id libvorbis.0.dylib $SUPPORT_LIBS_DIR/libvorbis.0.dylib
install_name_tool -id libogg.0.dylib $SUPPORT_LIBS_DIR/libogg.0.dylib
install_name_tool -id libfluidsynth.1.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.dylib
install_name_tool -id libwiiuse.dylib $SUPPORT_LIBS_DIR/libwiiuse.dylib

# change deps for libsndfile
export OLD_VORBISENC_LIB=$DEPS_BASE/lib/libvorbisenc.2.dylib
export NEW_VORBISENC_LIB=@loader_path/libvorbisenc.2.dylib
install_name_tool -change $OLD_VORBISENC_LIB $NEW_VORBISENC_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib

export OLD_VORBIS_LIB=$DEPS_BASE/lib/libvorbis.0.dylib
export NEW_VORBIS_LIB=@loader_path/libvorbis.0.dylib
install_name_tool -change $OLD_VORBIS_LIB $NEW_VORBIS_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_VORBIS_LIB $NEW_VORBIS_LIB $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

export OLD_OGG_LIB=$DEPS_BASE/lib/libogg.0.dylib
export NEW_OGG_LIB=@loader_path/libogg.0.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libvorbis.0.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

export OLD_FLAC_LIB=$DEPS_BASE/lib/libFLAC.8.dylib
export NEW_FLAC_LIB=@loader_path/libFLAC.8.dylib
install_name_tool -change $OLD_FLAC_LIB $NEW_FLAC_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libFLAC.8.dylib

# change dep for libsamplerate
install_name_tool -change $DEPS_BASE/lib/libsamplerate.0.dylib @loader_path/libsamplerate.0.dylib $SUPPORT_LIBS_DIR/libsamplerate.0.dylib
# and for src_conv (NEEDS CHECKING!)
install_name_tool -change $DEPS_BASE/lib/libsamplerate.0.dylib $SUPPORT_LIBS_DIR/libsamplerate.0.dylib $APPS64_DIR/src_conv
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib $APPS64_DIR/src_conv

install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libpng16.16.dylib @loader_path/libpng16.16.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.dylib

# install name changes for libs under framework, luajit not included here
# will make libsndfile location absolute to avoid linking problems for API clients using flat namespace.
# @loader_path/../../ => /Library/Frameworks/CsoundLib64.framework/
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/CsoundLib64
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib

install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# absolute path in _csnd6.so
install_name_tool -change $BUILD_DIR/libcsnd6.6.0.dylib /Library/Frameworks/CsoundLib64.framework/Versions/6.0/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# absolute path in _csnd6.so
install_name_tool -change $BUILD_DIR/CsoundLib64.framework/Versions/6.0/CsoundLib64 /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64 $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# absolute path in libcsnd6.6.0.dylib
install_name_tool -change CsoundLib64.framework/Versions/6.0/CsoundLib64  /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64  $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib

install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python Python.framework/Versions/2.7/Python $FRAMEWORK64_DIR/Resources/Opcodes64/libpy.dylib

install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Java/lib_jcsound6.jnilib

# Python Library now needs to be re-set to /Library/Frameworks 
install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python /Library/Frameworks/Python.framework/Versions/2.7/Python $FRAMEWORK64_DIR/Resources/Opcodes64/libpy.dylib
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libstdutil.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/../../../../libs/libfltk.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk_images.1.3.dylib @loader_path/../../../../libs/libfltk_images.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk_forms.1.3.dylib @loader_path/../../../../libs/libfltk_forms.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/../../../../libs/libfltk.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk_images.1.3.dylib @loader_path/../../../../libs/libfltk_images.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk_forms.1.3.dylib @loader_path/../../../../libs/libfltk_forms.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change $DEPS_BASE/lib/liblo.7.dylib @loader_path/../../../../libs/liblo.7.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libosc.dylib
install_name_tool -change $DEPS_BASE/lib/libportaudio.2.dylib @loader_path/../../../../libs/libportaudio.2.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/librtpa.dylib
install_name_tool -change $DEPS_BASE/lib/libwiiuse.dylib @loader_path/../../../../libs/libwiiuse.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwiimote.dylib
install_name_tool -change $DEPS_BASE/lib/libpng16.16.dylib @loader_path/../../../../libs/libpng16.16.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libimage.dylib
install_name_tool -change $DEPS_BASE/lib/libpng16.16.dylib @loader_path/../../../../libs/libpng16.16.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change $DEPS_BASE/lib/libfluidsynth.1.dylib @loader_path/../../../../libs/libfluidsynth.1.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libfluidOpcodes.dylib
install_name_tool -change libportmidi.dylib @loader_path/../../../../libs/libportmidi.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libpmidi.dylib

echo "...setting permissions..."

cd installer

sudo chgrp -R admin  CsoundLib64/Package_Contents/Library
sudo chown -R root   CsoundLib64/Package_Contents/Library
sudo chmod -R 775    CsoundLib64/Package_Contents/Library

sudo chgrp -R wheel  CsoundApps64/Package_Contents/usr
sudo chown -R root   CsoundApps64/Package_Contents/usr
sudo chmod -R 755    CsoundApps64/Package_Contents/usr


echo "building packages ..."

pkgbuild --identifier com.csound.csound6Environment.csoundLib64 --root CsoundLib64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib64 CsoundLib64.pkg
pkgbuild --identifier com.csound.csound6Environment.csoundApps64 --root CsoundApps64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps64 CsoundApps64.pkg

echo "building product..."

productbuild --distribution ../../Distribution2.dist --resources ../../PkgResources/en.lproj $PACKAGE_NAME

echo "assembling DMG..."

mkdir "$DMG_DIR" 
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
#cp  ../../../readme.pdf .
#cp  ../../../DmgResources/CsoundQt-0.9.7-MacOs.dmg .
#hdiutil create CsoundQT.dmg -srcfolder ../../../DmgResources/

cd ..
hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR" -fs HFS+ 

echo "... finished."

open $INSTALLER_DIR
