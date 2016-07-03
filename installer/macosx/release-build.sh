#!/bin/sh

if [ $# == 0 ]; then
  echo "Must give branch name to build from"
  exit
else
  export BRANCH_NAME=$1
  echo "Using branch: $BRANCH_NAME"
fi

export MANUAL_DIR=`pwd`/../../../manual
export CS_VERSION="6.07"
export PACKAGE_NAME=csound${CS_VERSION}-OSX-universal.pkg
export DMG_DIR="Csound ${CS_VERSION}"
export DMG_NAME="csound${CS_VERSION}-OSX-universal.dmg"

export SDK=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/
export TARGET=10.7
export DEPS_BASE=/usr/local
# If arg2 passed in, will cd into that dir and rebuild, otherwise
# will clone from repo and do a fresh build

if [ $# -gt 1 ]; then
	cd $2
	echo "Using directory $2 `pwd`"
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
# RUN CMAKE TWICE TO GET AROUND ISSUE WITH UNIVERSAL BUILD
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=1 -DBUILD_FAUST_OPCODES=1 -DFAUST_LIBRARY=$DEPS_BASE/lib/faust/libfaust.a  -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET -DCMAKE_OSX_SYSROOT=$SDK
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="i386;x86_64" -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=1 -DBUILD_FAUST_OPCODES=1 -DFAUST_LIBRARY=$DEPS_BASE/lib/faust/libfaust.a -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET -DCMAKE_OSX_SYSROOT=$SDK
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
export LUA_DIR=Versions/$CSLIBVERSION/Resources/Luajit
export CSLADSPA_DIR=Versions/$CSLIBVERSION/Resources/csladspa
export SAMPLES_DIR=Versions/$CSLIBVERSION/Resources/samples
export PD_DIR=Versions/$CSLIBVERSION/Resources/PD

export DIST=csound6/build/dist
export BLD=csound6/build

#mkdir -p $APPS32_DIR
mkdir -p $APPS64_DIR
#mkdir -p $FRAMEWORK32_DIR
mkdir -p $FRAMEWORK64_DIR
mkdir -p $SUPPORT_LIBS_DIR


mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$JAVA_DIR
mkdir -p $FRAMEWORK64_DIR/$LUA_DIR
mkdir -p $FRAMEWORK64_DIR/$SAMPLES_DIR
mkdir -p $FRAMEWORK64_DIR/$PD_DIR
mkdir -p $FRAMEWORK64_DIR/../Documentation
#mkdir -p $FRAMEWORK64_DIR/Headers

cp -R $DIST/CsoundLib64.framework/ $FRAMEWORK64_DIR
#cp -R csound5-f/CsoundLib.framework/ $FRAMEWORK32_DIR

echo "Copying Python Libs... $PWD"

cp $DIST/../_csnd6.so $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/../csnd6.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/../CsoundAC.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/../_CsoundAC.so $FRAMEWORK64_DIR/$PYTHON_DIR
cp $BLD/../interfaces/ctcsound.py $FRAMEWORK64_DIR/$PYTHON_DIR
export CSOUND_AC_PYLIB=$FRAMEWORK64_DIR/$PYTHON_DIR/_CsoundAC.so


echo "preparing framework..."

cp  $DIST/lib/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/$CSLIBVERSION/
cp  $DIST/lib/libCsoundAC.6.0.dylib $FRAMEWORK64_DIR/Versions/$CSLIBVERSION/
cp  $DIST/lib/lib_jcsound6.jnilib $FRAMEWORK64_DIR/$JAVA_DIR
cp  $DIST/lib/csnd6.jar $FRAMEWORK64_DIR/$JAVA_DIR
cp  $DIST/lib/luaCsnd6.so $FRAMEWORK64_DIR/$LUA_DIR
cp  $DIST/lib/luaCsoundAC.so $FRAMEWORK64_DIR/$LUA_DIR
cp  $DIST/lib/csound6~.pd_darwin $FRAMEWORK64_DIR/$PD_DIR
cp  csound6/examples/csoundapi_tilde/csound6~-help.pd $FRAMEWORK64_DIR/$PD_DIR/
cp  csound6/examples/csoundapi_tilde/csapi_demo.csd $FRAMEWORK64_DIR/$PD_DIR/
cp  csound6/examples/csoundapi_tilde/demo.orc $FRAMEWORK64_DIR/$PD_DIR/

echo "copying manual..."

#cp -Rf $MANUAL_DIR/html $FRAMEWORK32_DIR/Resources/
#mv $FRAMEWORK32_DIR/Resources/html $FRAMEWORK32_DIR/Resources/Manual

cp -Rf $MANUAL_DIR/html $FRAMEWORK64_DIR/Resources/
mv $FRAMEWORK64_DIR/Resources/html $FRAMEWORK64_DIR/Resources/Manual

echo "copying samples..."

cp csound6/samples/*.dat $FRAMEWORK64_DIR/$SAMPLES_DIR

echo "copying csladspa..."

mkdir -p $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA
mv $FRAMEWORK64_DIR/Resources/Opcodes64/csladspa.dylib $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA/csladspa64.dylib

echo "copying apps..."

cp $DIST/bin/* $APPS64_DIR

echo "copying support libs..."
cp $DEPS_BASE/lib/libfltk.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/liblo.7.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libportmidi.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libpng16.16.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libFLAC.8.2.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libogg.0.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libfluidsynth.1.dylib $SUPPORT_LIBS_DIR
cp $DEPS_BASE/lib/libwiiuse.dylib $SUPPORT_LIBS_DIR
# not sure which opcode etc is dependent on this luajit lib
cp $DEPS_BASE/lib/libluajit-5.1.2.0.2.dylib $SUPPORT_LIBS_DIR 

#cp -L $DEPS_BASE/lib/libmpadec.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libluajit.dylib $SUPLIBS

# chnage IDs
install_name_tool -id libfltk.1.3.dylib $SUPPORT_LIBS_DIR/libfltk.1.3.dylib
install_name_tool -id libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -id libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib
install_name_tool -id liblo.7.dylib $SUPPORT_LIBS_DIR/liblo.7.dylib
install_name_tool -id libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -id libportaudio.2.dylib $SUPPORT_LIBS_DIR/libportaudio.2.dylib
install_name_tool -id libportmidi.dylib $SUPPORT_LIBS_DIR/libportmidi.dylib
install_name_tool -id libpng16.16.dylib $SUPPORT_LIBS_DIR/libpng16.16.dylib
install_name_tool -id libFLAC.8.2.0.dylib $SUPPORT_LIBS_DIR/libFLAC.8.2.0.dylib
install_name_tool -id libvorbisenc.2.dylib $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib
install_name_tool -id libvorbis.0.dylib $SUPPORT_LIBS_DIR/libvorbis.0.dylib
install_name_tool -id libogg.0.dylib $SUPPORT_LIBS_DIR/libogg.0.dylib
install_name_tool -id libfluidsynth.1.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.dylib
install_name_tool -id libwiiuse.dylib $SUPPORT_LIBS_DIR/libwiiuse.dylib
install_name_tool -id libluajit-5.1.2.0.2.dylib $SUPPORT_LIBS_DIR/libluajit-5.1.2.0.2.dylib

# change deps for libsndfile
export OLD_VORBISENC_LIB=/usr/local/lib/libvorbisenc.2.dylib
export NEW_VORBISENC_LIB=@loader_path/libvorbisenc.2.dylib
install_name_tool -change $OLD_VORBISENC_LIB $NEW_VORBISENC_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib

export OLD_VORBIS_LIB=/usr/local/lib/libvorbis.0.dylib
export NEW_VORBIS_LIB=@loader_path/libvorbis.0.dylib
install_name_tool -change $OLD_VORBIS_LIB $NEW_VORBIS_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_VORBIS_LIB $NEW_VORBIS_LIB $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

export OLD_OGG_LIB=/usr/local/lib/libogg.0.dylib
export NEW_OGG_LIB=@loader_path/libogg.0.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libvorbis.0.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

export OLD_FLAC_LIB=/usr/local/lib/libFLAC.8.2.0.dylib
export NEW_FLAC_LIB=@loader_path/libFLAC.8.2.0.dylib
install_name_tool -change $OLD_FLAC_LIB $NEW_FLAC_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib
install_name_tool -change $OLD_OGG_LIB $NEW_OGG_LIB $SUPPORT_LIBS_DIR/libFLAC.8.2.0.dylib

install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libpng16.16.dylib @loader_path/libpng16.16.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change $DEPS_BASE/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.dylib

# install name changes for libs under framework, luajit not included here
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/CsoundLib64
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/libCsoundAC.6.0.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/../../libs/libfltk.1.3.dylib  $FRAMEWORK64_DIR/Versions/6.0/libCsoundAC.6.0.dylib
install_name_tool -change $DEPS_BASE/lib/libfltk_images.1.3.dylib @loader_path/../../libs/libfltk_images.1.3.dylib  $FRAMEWORK64_DIR/Versions/6.0/libCsoundAC.6.0.dylib

install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# absolute path in _csnd6.so
install_name_tool -change libcsnd6.6.0.dylib /Library/Frameworks/CsoundLib64.framework/Versions/6.0/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so
install_name_tool -change libcsnd6.6.0.dylib /Library/Frameworks/CsoundLib64.framewok/Versions/6.0/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so
install_name_tool -change $DEPS_BASE/lib/libfltk.1.3.dylib @loader_path/../../../../../libs/libfltk.1.3.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so
install_name_tool -change $DEPS_BASE/lib/libfltk_images.1.3.dylib @loader_path/../../../../../libs/libfltk_images.1.3.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so

install_name_tool -change $BUILD_DIR/CsoundLib64.framework/Versions/6.0/CsoundLib64 /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64 $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so

# absolute path in _csnd6.so
install_name_tool -change $BUILD_DIR/CsoundLib64.framework/Versions/6.0/CsoundLib64 /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64 $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# absolute path in libcsnd6.6.0.dylib
install_name_tool -change CsoundLib64.framework/Versions/6.0/CsoundLib64  /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64  $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib

#install_name_tool -change $BUILD_DIR/libcsnd6.6.0.dylib libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_CsoundAC.so
#install_name_tool -change $BUILD_DIR/libcsnd6.6.0.dylib libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python Python.framework/Versions/2.7/Python $FRAMEWORK64_DIR/Resources/Opcodes64/libpy.dylib
install_name_tool -change $DEPS_BASE/lib/libsndfile.1.dylib @loader_path/../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Java/lib_jcsound6.jnilib

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
install_name_tool -change /usr/local/lib/libportmidi.dylib @loader_path/../../../../libs/libportmidi.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libpmidi.dylib


install_name_tool -change libCsoundAC.6.0.dylib /usr/local/lib/libCsoundAC.6.0.dylib $CSOUND_AC_PYLIB

install_name_tool -change $DEPS_BASE/lib/libluajit-5.1.2.dylib @loader_path/../../../../libs/libluajit-5.1.2.0.2.dylib  $FRAMEWORK64_DIR/$LUA_DIR/luaCsnd6.so 
install_name_tool -change $DEPS_BASE/lib/libluajit-5.1.2.dylib @loader_path/../../../../libs/libluajit-5.1.2.0.2.dylib  $FRAMEWORK64_DIR/$LUA_DIR/luaCsoundAC.so

echo "...setting permissions..."

cd installer

sudo chgrp -R admin  CsoundLib64/Package_Contents/Library
sudo chown -R root   CsoundLib64/Package_Contents/Library
sudo chmod -R 775    CsoundLib64/Package_Contents/Library

sudo chgrp -R admin  CsoundApps64/Package_Contents/Library
sudo chown -R root   CsoundApps64/Package_Contents/Library
sudo chmod -R 775    CsoundApps64/Package_Contents/Library
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
cp  ../../../readme.pdf .
cp  ../../../DmgResources/CsoundQt-d-py-cs6-0.9.2.1.dmg .
#hdiutil create CsoundQT.dmg -srcfolder ../../../DmgResources/

cd ..
hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR" 

echo "... finished."

open $INSTALLER_DIR
