#!/bin/sh

export MANUAL_DIR=`pwd`/../../../manual
export CS_VERSION="6.03.2"
export PACKAGE_NAME=csound${CS_VERSION}-OSX10.9-universal.pkg
export DMG_DIR="Csound ${CS_VERSION}"
export DMG_NAME="csound${CS_VERSION}-OSX10.9-universal.dmg"
# If arg2 passed in, will cd into that dir and rebuild, otherwise
# will clone from repo and do a fresh build

if [ $# == 0 ]; then
  echo "Must give branch name to build from"
  exit
else
  export BRANCH_NAME=$1
  echo "Using branch: $BRANCH_NAME"
fi

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

#/usr/local/bin/scons -j2 &> ../csound5_build_log.txt
mkdir build
cd build
# RUN CMAKE TWICE TO GET AROUND ISSUE WITH UNIVERSAL BUILD
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=1 -DBUILD_FAUST_OPCODES=1
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="i386;x86_64" -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=1 -DBUILD_FAUST_OPCODES=1
make -j6 install

# BUILD FLOAT CSOUND5
#echo "Building Csound (float)..."

#cd ../csound5-f
#cp ../../custom.10.7.py custom.py 

#/usr/local/bin/scons -j2 &> ../csound5-f_build_log.txt

cd ../..

# ASSEMBLE FILES FOR INSTALLER
export CSLIBVERSION=6.0
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local/lib
#export APPS32_DIR=$INSTALLER_DIR/CsoundApps/Package_Contents/usr/local/bin
export APPS64_DIR=$INSTALLER_DIR/CsoundApps64/Package_Contents/usr/local/bin
#export FRAMEWORK32_DIR=$INSTALLER_DIR/CsoundLib/Package_Contents/Library/Frameworks/CsoundLib.framework
export FRAMEWORK64_DIR=$INSTALLER_DIR/CsoundLib64/Package_Contents/Library/Frameworks/CsoundLib64.framework

export PYTHON_DIR=Versions/$CSLIBVERSION/Resources/Python/Current
export TCLTK_DIR=Versions/$CSLIBVERSION/Resources/TclTk
export JAVA_DIR=Versions/$CSLIBVERSION/Resources/Java
export LUA_DIR=Versions/$CSLIBVERSION/Resources/Luajit
export CSLADSPA_DIR=Versions/$CSLIBVERSION/Resources/csladspa
export SAMPLES_DIR=Versions/$CSLIBVERSION/Resources/samples
export PD_DIR=Versions/$CSLIBVERSION/Resources/PD

export DIST=csound6/build/dist

mkdir -p $SUPPORT_LIBS_DIR
#mkdir -p $APPS32_DIR
mkdir -p $APPS64_DIR
#mkdir -p $FRAMEWORK32_DIR
mkdir -p $FRAMEWORK64_DIR

#mkdir -p $FRAMEWORK32_DIR/$PYTHON_DIR
#mkdir -p $FRAMEWORK32_DIR/$TCLTK_DIR
#mkdir -p $FRAMEWORK32_DIR/$PYTHON_DIR
#mkdir -p $FRAMEWORK32_DIR/$JAVA_DIR
#mkdir -p $FRAMEWORK32_DIR/$SAMPLES_DIR
#mkdir -p $FRAMEWORK32_DIR/$PD_DIR
#mkdir -p $FRAMEWORK32_DIR/../Documentation
#mkdir -p $FRAMEWORK32_DIR/Headers

mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$TCLTK_DIR
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

#cp csound5-f/_csnd.so $FRAMEWORK32_DIR/$PYTHON_DIR
#cp csound5-f/csnd.py $FRAMEWORK32_DIR/$PYTHON_DIR
#cp csound5-f/CsoundAC.py $FRAMEWORK32_DIR/$PYTHON_DIR
#cp csound5-f/_CsoundAC.so $FRAMEWORK32_DIR/$PYTHON_DIR

cp $DIST/lib/_csnd6.so $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/lib/csnd6.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/lib/CsoundAC.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp $DIST/lib/_CsoundAC.so $FRAMEWORK64_DIR/$PYTHON_DIR
export CSOUND_AC_PYLIB=$FRAMEWORK64_DIR/$PYTHON_DIR/_CsoundAC.so


echo "preparing framework..."

#cp  csound5-f/lib_csnd.dylib $FRAMEWORK32_DIR/Versions/$CSLIBVERSION/
#cp  csound5-f/csladspa.so  $FRAMEWORK32_DIR/$CSLADSPA_DIR
#cp  csound5-f/lib_jcsound.jnilib $FRAMEWORK32_DIR/$JAVA_DIR
#cp  csound5-f/csnd.jar $FRAMEWORK32_DIR/$JAVA_DIR
#cp  csound5-f/csoundapi~.pd_darwin $FRAMEWORK32_DIR/$PD_DIR
#cp  csound5-f/examples/csoundapi_tilde/csoundapi-osx.pd $FRAMEWORK32_DIR/$PD_DIR/csoundapi.pd
#cp  csound5-f/examples/csoundapi_tilde/csapi_demo.csd $FRAMEWORK32_DIR/../Documentation/

#mv  $DIST/CsoundLib64.framework/Resources/Opcodes64/csladspa.dylib  $FRAMEWORK64_DIR/$CSLADSPA_DIR
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


#echo "copying headers..."

#cp csound5-f/interfaces/*.hpp $FRAMEWORK32_DIR/Headers
#cp csound5/interfaces/*.hpp $FRAMEWORK64_DIR/Headers


echo "copying samples..."

#cp csound5-f/samples/*.dat $FRAMEWORK32_DIR/$SAMPLES_DIR
cp csound6/samples/*.dat $FRAMEWORK64_DIR/$SAMPLES_DIR


echo "copying csladspa..."

#mkdir -p $APPS32_DIR/../../../Library/Audio/Plug-Ins/LADSPA
#cp csound5-f/csladspa.so $APPS32_DIR/../../../Library/Audio/Plug-Ins/LADSPA/csladspa.so


mkdir -p $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA
mv $FRAMEWORK64_DIR/Resources/Opcodes64/csladspa.dylib $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA/csladspa64.dylib


echo "copying apps..."

cp $DIST/bin/* $APPS64_DIR

#cp csound  $APPS64_DIR
#cp dnoise   $APPS64_DIR
#cp het_export $APPS64_DIR     
#cp lpanal  $APPS64_DIR        
#cp scale  $APPS64_DIR         
#cp tabdes $APPS64_DIR
#cp cs   $APPS64_DIR           
#cp cstclsh $APPS64_DIR        
#cp envext $APPS64_DIR          
#cp het_import $APPS64_DIR     
#cp lpc_export $APPS64_DIR     
#cp mixer  $APPS64_DIR         
#cp scsort $APPS64_DIR
#cp cswish  $APPS64_DIR        
#cp extract $APPS64_DIR         
#cp hetro $APPS64_DIR          
#cp lpc_import $APPS64_DIR     
#cp pvanal  $APPS64_DIR        
#cp sndinfo $APPS64_DIR
#cp csb64enc $APPS64_DIR        
#cp cvanal  $APPS64_DIR        
#cp extractor $APPS64_DIR       
#cp linseg    $APPS64_DIR      
#cp makecsd   $APPS64_DIR      
#cp pvlook   $APPS64_DIR       
#cp srconv $APPS64_DIR
#cp atsa  $APPS64_DIR
#cp csbeats  $APPS64_DIR

#cd ../csound5

#cp csound  $APPS64_DIR/csound64

#cd ../..



echo "copying support libs..."

#cp -L /usr/local/lib/libjpeg.8.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libfltk.1.3.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libglib-2.0.0.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libgthread-2.0.0.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/liblo.7.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libportmidi.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libpng15.15.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libFLAC.8.2.0.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libfluidsynth.1.5.2.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libwiiuse.dylib $SUPPORT_LIBS_DIR


cp /usr/local/lib/libfltk.1.3.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libfltk_forms.1.3.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/liblo.7.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libportmidi.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libpng16.16.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libFLAC.8.2.0.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libfluidsynth.1.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libwiiuse.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libluajit-5.1.2.0.2.dylib $SUPPORT_LIBS_DIR 

#cp -L /usr/local/lib/libmpadec.dylib $SUPPORT_LIBS_DIR
#cp -L /usr/local/lib/libluajit.dylib $SUPLIBS


export OLD_FLAC_LIB=/usr/local/lib/libFLAC.8.dylib
export NEW_FLAC_LIB=/usr/local/lib/libFLAC.8.2.0.dylib
install_name_tool -change $OLD_FLAC_LIB $NEW_FLAC_LIB $SUPPORT_LIBS_DIR/libsndfile.1.dylib

install_name_tool -change libCsoundAC.6.0.dylib /usr/local/lib/libCsoundAC.6.0.dylib $CSOUND_AC_PYLIB

#for file in $SUPPORT_LIBS_DIR/*
#do
#  install_name_tool -id /usr/local/lib/`basename $file` $file
#done

#install_name_tool -id /usr/local/lib/libfluidsynth.1.5.2.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.5.2.dylib
#install_name_tool -id /usr/local/lib/libwiiuse.dylib $SUPPORT_LIBS_DIR/libwiiuse.dylib

# for Fluidsynth
#mkdir -p $SUPPORT_LIBS_OPT_DIR/fluid-synth/
#cp -R /usr/local/opt/fluid-synth/lib $SUPPORT_LIBS_OPT_DIR/fluid-synth/

#mkdir -p $SUPPORT_LIBS_OPT_DIR/glib/
#cp -R /usr/local/opt/glib/lib $SUPPORT_LIBS_OPT_DIR/glib/

#mkdir -p $SUPPORT_LIBS_OPT_DIR/gettext/lib
#cp -L /usr/local/opt/gettext/lib/libintl.8.dylib $SUPPORT_LIBS_OPT_DIR/gettext/lib

#mkdir -p $SUPPORT_LIBS_OPT_DIR/fltk/
#cp -R /usr/local/opt/fltk/lib $SUPPORT_LIBS_OPT_DIR/fltk/

# not sure this is necessary...
#export OLD_FLUID_LIB=/usr/local/opt/fluid-synth/lib/libfluidsynth.1.5.2.dylib
#export NEW_FLUID_LIB=/usr/local/lib/libfluidsynth.1.5.2.dylib 
#install_name_tool -change $OLD_FLUID_LIB $NEW_FLUID_LIB $FRAMEWORK64_DIR/Versions/6.0/Resources/Opcodes64/libfluidOpcodes.dylib

#export OLD_GLIB_LIB=/usr/local/Cellar/glib/2.36.2/lib/libglib-2.0.0.dylib
#export NEW_GLIB_LIB=/usr/local/lib/libglib-2.0.0.dylib
#install_name_tool -change $OLD_GLIB_LIB $NEW_GLIB_LIB $SUPPORT_LIBS_DIR/libgthread-2.0.0.dylib

#export OLD_VORBIS_LIB=/usr/local/Cellar/libvorbis/1.3.3/lib/libvorbis.0.dylib
#export NEW_VORBIS_LIB=/usr/local/lib/libvorbis.0.dylib
#install_name_tool -change $OLD_VORBIS_LIB $NEW_VORBIS_LIB $SUPPORT_LIBS_DIR/libvorbisenc.2.dylib

#export OLD_FLTK_LIB=/usr/local/Cellar/fltk/1.3.2/lib/libfltk.1.3.dylib 
#export NEW_FLTK_LIB=/usr/local/lib/libfltk.1.3.dylib 
#install_name_tool -change $OLD_FLTK_LIB $NEW_FLTK_LIB $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
#install_name_tool -change $OLD_FLTK_LIB $NEW_FLTK_LIB $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib

echo "...setting permissions..."

cd installer

#chgrp -r admin  csoundlib/package_contents/library
#chown -r root   csoundlib/package_contents/library
#chmod -r 775    csoundlib/package_contents/library
##chgrp -r wheel  csoundlib/package_contents/system
##chown -r root   csoundlib/package_contents/system
##chmod -r 755    csoundlib/package_contents/system
#
#chgrp -r admin  csoundapps/package_contents/library
#chown -r root   csoundapps/package_contents/library
#chmod -r 775    csoundapps/package_contents/library
#chgrp -r wheel  csoundapps/package_contents/usr
#chown -r root   csoundapps/package_contents/usr
#chmod -R 755    CsoundApps/Package_Contents/usr

chgrp -R admin  CsoundLib64/Package_Contents/Library
chown -R root   CsoundLib64/Package_Contents/Library
chmod -R 775    CsoundLib64/Package_Contents/Library
#chgrp -R wheel  CsoundLib64/Package_Contents/System
#chown -R root   CsoundLib64/Package_Contents/System
#chmod -R 755    CsoundLib64/Package_Contents/System

chgrp -R admin  CsoundApps64/Package_Contents/Library
chown -R root   CsoundApps64/Package_Contents/Library
chmod -R 775    CsoundApps64/Package_Contents/Library
chgrp -R wheel  CsoundApps64/Package_Contents/usr
chown -R root   CsoundApps64/Package_Contents/usr
chmod -R 755    CsoundApps64/Package_Contents/usr


chgrp -R wheel  SupportLibs/Package_Contents/usr
chown -R root   SupportLibs/Package_Contents/usr
chmod -R 755    SupportLibs/Package_Contents/usr

echo "building packages ..."

pkgbuild --identifier com.csound.csound6Environment.csoundLib64 --root CsoundLib64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib64 CsoundLib64.pkg
#pkgbuild --identifier com.csound.csound6Environment.csoundLib --root CsoundLib/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib CsoundLib.pkg
#pkgbuild --identifier com.csound.csound6Environment.csoundApps --root CsoundApps/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps CsoundApps.pkg
pkgbuild --identifier com.csound.csound6Environment.csoundApps64 --root CsoundApps64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps64 CsoundApps64.pkg
pkgbuild --identifier com.csound.csound6Environment.supportLibs --root SupportLibs/Package_Contents/ --version 1 --scripts ../../PkgResources/SupportLibs SupportLibs.pkg


echo "building product..."

productbuild --distribution ../../Distribution.dist --resources ../../PkgResources/en.lproj $PACKAGE_NAME

echo "assembling DMG..."

mkdir "$DMG_DIR" 
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
cp -R ../../../DmgResources/* .
ln -s /Applications .
cd ..

hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

echo "... finished."

open $INSTALLER_DIR
