#!/bin/sh

export MANUAL_DIR=`pwd`/../../../manual
export PACKAGE_NAME=csound5.16.1-OSX10.7-universal.pkg
export DMG_DIR="Csound 5.16.1"
export DMG_NAME="csound5.16.1-OSX10.7-universal.dmg"
# If arg2 passed in, will cd into that dir and rebuild, otherwise
# will clone from repo and do a fresh build

if [ $# -gt 0 ]; then
	cd $1
	echo "Using directory $1 `pwd`"
        export INSTALLER_DIR=`pwd`/installer
        rm -rf installer 
        mkdir installer
else
	export RELEASE_DIR="`eval date +%Y-%m-%d-%H%M%S`"
        export INSTALLER_DIR=`pwd`/$RELEASE_DIR/installer
	mkdir $RELEASE_DIR 
        mkdir $INSTALLER_DIR
	cd $RELEASE_DIR

	#git clone git://csound.git.sourceforge.net/gitroot/csound/csound5
	git clone ../../.. csound5 
	cp -R csound5 csound5-f
fi

#BUILD DOUBLES CSOUND
echo "Building Csound (double)..."
cd csound5
cp ../../custom.10.7.py custom.py 

export SCONSFLAGS="CC=gcc-4.2 CXX=g++-4.2 gcc4opt=universalX86 useFLTK=1 includeMP3=1 noDebug=1 buildInterfaces=1 buildPythonOpcodes=1 buildRelease=1 buildTclcsound=1 buildPDClass=1 useOSC=1 buildNewParser=1 dynamicCsoundLibrary=1 buildVirtual=1 useJack=1 buildCsoundAC=1 useGettext=0 NewParserDebug=0 buildPythonWrapper=1 buildMultiCore=1 buildJavaWrapper=1 buildLuaOpcodes=0 useDouble=1"
/usr/local/bin/scons -j2 &> ../csound5_build_log.txt

# BUILD FLOAT CSOUND5
echo "Building Csound (float)..."

cd ../csound5-f
cp ../../custom.10.7.py custom.py 

export SCONSFLAGS="CC=gcc-4.2 CXX=g++-4.2 gcc4opt=universalX86 useFLTK=1 includeMP3=1 noDebug=1 buildInterfaces=1 buildPythonOpcodes=1 buildRelease=1 buildTclcsound=1 buildPDClass=1 useOSC=1 buildNewParser=1 dynamicCsoundLibrary=1 buildVirtual=1 useJack=1 buildCsoundAC=1 useGettext=0 NewParserDebug=0 buildPythonWrapper=1 buildMultiCore=1 buildJavaWrapper=1 buildLuaOpcodes=0"
/usr/local/bin/scons -j2 &> ../csound5-f_build_log.txt

cd ..

# ASSEMBLE FILES FOR INSTALLER
export CSLIBVERSION=5.2
export SUPPORT_LIBS_DIR=$INSTALLER_DIR/SupportLibs/Package_Contents/usr/local/lib
export APPS32_DIR=$INSTALLER_DIR/CsoundApps/Package_Contents/usr/local/bin
export APPS64_DIR=$INSTALLER_DIR/CsoundApps64/Package_Contents/usr/local/bin
export FRAMEWORK32_DIR=$INSTALLER_DIR/CsoundLib/Package_Contents/Library/Frameworks/CsoundLib.framework
export FRAMEWORK64_DIR=$INSTALLER_DIR/CsoundLib64/Package_Contents/Library/Frameworks/CsoundLib64.framework

export PYTHON_DIR=Versions/$CSLIBVERSION/Resources/Python/Current
export TCLTK_DIR=Versions/$CSLIBVERSION/Resources/TclTk
export JAVA_DIR=Versions/$CSLIBVERSION/Resources/Java
export CSLADSPA_DIR=Versions/$CSLIBVERSION/Resources/csladspa
export SAMPLES_DIR=Versions/$CSLIBVERSION/Resources/samples
export PD_DIR=Versions/$CSLIBVERSION/Resources/PD

mkdir -p $SUPPORT_LIBS_DIR
mkdir -p $APPS32_DIR
mkdir -p $APPS64_DIR
mkdir -p $FRAMEWORK32_DIR
mkdir -p $FRAMEWORK64_DIR

mkdir -p $FRAMEWORK32_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK32_DIR/$TCLTK_DIR
mkdir -p $FRAMEWORK32_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK32_DIR/$JAVA_DIR
mkdir -p $FRAMEWORK32_DIR/$SAMPLES_DIR
mkdir -p $FRAMEWORK32_DIR/$PD_DIR
mkdir -p $FRAMEWORK32_DIR/../Documentation
#mkdir -p $FRAMEWORK32_DIR/Headers

mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$TCLTK_DIR
mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$JAVA_DIR
mkdir -p $FRAMEWORK64_DIR/$SAMPLES_DIR
mkdir -p $FRAMEWORK64_DIR/$PD_DIR
mkdir -p $FRAMEWORK64_DIR/../Documentation
#mkdir -p $FRAMEWORK64_DIR/Headers

cp -R csound5/CsoundLib64.framework/ $FRAMEWORK64_DIR
cp -R csound5-f/CsoundLib.framework/ $FRAMEWORK32_DIR



echo "Copying Python Libs..."

cp csound5-f/_csnd.so $FRAMEWORK32_DIR/$PYTHON_DIR
cp csound5-f/csnd.py $FRAMEWORK32_DIR/$PYTHON_DIR
cp csound5-f/CsoundAC.py $FRAMEWORK32_DIR/$PYTHON_DIR
cp csound5-f/_CsoundAC.so $FRAMEWORK32_DIR/$PYTHON_DIR

cp csound5/_csnd.so $FRAMEWORK64_DIR/$PYTHON_DIR
cp csound5/csnd.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp csound5/CsoundAC.py $FRAMEWORK64_DIR/$PYTHON_DIR
cp csound5/_CsoundAC.so $FRAMEWORK64_DIR/$PYTHON_DIR



echo "preparing framework..."

cp  csound5-f/lib_csnd.dylib $FRAMEWORK32_DIR/Versions/$CSLIBVERSION/
cp  csound5-f/tclcsound.dylib $FRAMEWORK32_DIR/$TCLTK_DIR
cp  csound5-f/csladspa.so  $FRAMEWORK32_DIR/$CSLADSPA_DIR
cp  csound5-f/lib_jcsound.jnilib $FRAMEWORK32_DIR/$JAVA_DIR
cp  csound5-f/csnd.jar $FRAMEWORK32_DIR/$JAVA_DIR
cp  csound5-f/csoundapi~.pd_darwin $FRAMEWORK32_DIR/$PD_DIR
cp  csound5-f/examples/csoundapi_tilde/csoundapi-osx.pd $FRAMEWORK32_DIR/$PD_DIR/csoundapi.pd
cp  csound5-f/examples/csoundapi_tilde/csapi_demo.csd $FRAMEWORK32_DIR/../Documentation/

cp  csound5/lib_csnd.dylib $FRAMEWORK64_DIR/Versions/$CSLIBVERSION/
cp  csound5/tclcsound.dylib $FRAMEWORK64_DIR/$TCLTK_DIR
cp  csound5/csladspa.so  $FRAMEWORK64_DIR/$CSLADSPA_DIR
cp  csound5/lib_jcsound.jnilib $FRAMEWORK64_DIR/$JAVA_DIR
cp  csound5/csnd.jar $FRAMEWORK64_DIR/$JAVA_DIR
cp  csound5/csoundapi~.pd_darwin $FRAMEWORK64_DIR/$PD_DIR
cp  csound5/examples/csoundapi_tilde/csoundapi-osx.pd $FRAMEWORK64_DIR/$PD_DIR/csoundapi.pd
cp  csound5/examples/csoundapi_tilde/csapi_demo.csd $FRAMEWORK64_DIR/../Documentation/


echo "copying manual..."

cp -Rf $MANUAL_DIR/html $FRAMEWORK32_DIR/Resources/
mv $FRAMEWORK32_DIR/Resources/html $FRAMEWORK32_DIR/Resources/Manual

cp -Rf $MANUAL_DIR/html $FRAMEWORK64_DIR/Resources/
mv $FRAMEWORK64_DIR/Resources/html $FRAMEWORK64_DIR/Resources/Manual


echo "copying headers..."

cp csound5-f/interfaces/*.hpp $FRAMEWORK32_DIR/Headers
cp csound5/interfaces/*.hpp $FRAMEWORK64_DIR/Headers


echo "copying samples..."

cp csound5-f/samples/*.dat $FRAMEWORK32_DIR/$SAMPLES_DIR
cp csound5/samples/*.dat $FRAMEWORK64_DIR/$SAMPLES_DIR


echo "copying csladspa..."

mkdir -p $APPS32_DIR/../../../Library/Audio/Plug-Ins/LADSPA
cp csound5-f/csladspa.so $APPS32_DIR/../../../Library/Audio/Plug-Ins/LADSPA/csladspa.so


mkdir -p $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA
cp csound5/csladspa.so $APPS64_DIR/../../../Library/Audio/Plug-Ins/LADSPA/csladspa64.so


echo "copying apps..."

cd csound5-f

cp csound  $APPS32_DIR
cp dnoise   $APPS32_DIR
cp het_export $APPS32_DIR     
cp lpanal  $APPS32_DIR        
cp scale  $APPS32_DIR         
cp tabdes $APPS32_DIR
cp cs   $APPS32_DIR           
cp cstclsh $APPS32_DIR        
cp envext $APPS32_DIR          
cp het_import $APPS32_DIR     
cp lpc_export $APPS32_DIR     
cp mixer  $APPS32_DIR         
cp scsort $APPS32_DIR
cp cswish  $APPS32_DIR        
cp extract $APPS32_DIR         
cp hetro $APPS32_DIR          
cp lpc_import $APPS32_DIR     
cp pvanal  $APPS32_DIR        
cp sndinfo $APPS32_DIR
cp csb64enc $APPS32_DIR        
cp cvanal  $APPS32_DIR        
cp extractor $APPS32_DIR       
cp linseg    $APPS32_DIR      
cp makecsd   $APPS32_DIR      
cp pvlook   $APPS32_DIR       
cp srconv $APPS32_DIR
cp atsa  $APPS32_DIR

cd ../csound5

cp csound  $APPS64_DIR/csound64

cd ..



echo "copying support libs..."

cp /usr/local/lib/libfltk.1.3.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libfltk_images.1.3.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libfluidsynth.1.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/liblo.7.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libportaudio.2.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libportmidi.dylib $SUPPORT_LIBS_DIR
#cp /usr/local/lib/libpng12.0.dylib $SUPLIBS
#cp /usr/local/lib/libmpadec.dylib $SUPPORT_LIBS_DIR
#cp /usr/local/lib/libluajit.dylib $SUPLIBS
cp /usr/local/lib/libFLAC.8.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libvorbisenc.2.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libvorbis.0.dylib $SUPPORT_LIBS_DIR
cp /usr/local/lib/libogg.0.dylib $SUPPORT_LIBS_DIR

echo "...setting permissions..."

cd installer

chgrp -R admin  CsoundLib/Package_Contents/Library
chown -R root   CsoundLib/Package_Contents/Library
chmod -R 775    CsoundLib/Package_Contents/Library
#chgrp -R wheel  CsoundLib/Package_Contents/System
#chown -R root   CsoundLib/Package_Contents/System
#chmod -R 755    CsoundLib/Package_Contents/System

chgrp -R admin  CsoundApps/Package_Contents/Library
chown -R root   CsoundApps/Package_Contents/Library
chmod -R 775    CsoundApps/Package_Contents/Library
chgrp -R wheel  CsoundApps/Package_Contents/usr
chown -R root   CsoundApps/Package_Contents/usr
chmod -R 755    CsoundApps/Package_Contents/usr

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

pkgbuild --identifier com.csound.csound5Environment.csoundLib64 --root CsoundLib64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib64 CsoundLib64.pkg
pkgbuild --identifier com.csound.csound5Environment.csoundLib --root CsoundLib/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib CsoundLib.pkg
pkgbuild --identifier com.csound.csound5Environment.csoundApps --root CsoundApps/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps CsoundApps.pkg
pkgbuild --identifier com.csound.csound5Environment.csoundApps64 --root CsoundApps64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps64 CsoundApps64.pkg
pkgbuild --identifier com.csound.csound5Environment.supportLibs --root SupportLibs/Package_Contents/ --version 1 --scripts ../../PkgResources/SupportLibs SupportLibs.pkg


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
