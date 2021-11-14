#!/bin/sh

brew install libsndfile
brew install portaudio
brew install liblo
brew install fltk
brew install portmidi
brew install flac
brew install vorbis-tools
brew install libpng
brew install libogg
brew install fluid-synth
brew install libsamplerate
brew install openssl

# install Csound, and grab libs pre-built libs from there
# mkdir csDownload
# cd csDownload
# curl -L -o Csound6.12.1-MacOS_universal.dmg 'https://github.com/csound/csound/releases/download/6.12.2/Csound6.12.1-MacOS_universal.dmg'
# ls
# hdiutil attach Csound6.12.1-MacOS_universal.dmg
# cp -R /Volumes/Csound6.12.1/ Csound
# hdiutil detach /Volumes/Csound6.12.1/
# cd Csound
# sudo installer -pkg csound6.12.1-OSX-universal.pkg -target /

# cd ../..
# echo "Checking libs folder is valid"
# ls /Library/Frameworks/CsoundLib64.framework/libs
# sudo cp -a /Library/Frameworks/CsoundLib64.framework/libs/.  /usr/local/libs/
# mkdir $SYSTEM_DEFAULTWORKINGDIRECTORY/csoundLibs

echo "Showing usr/lib before copying over files"
ls /usr/local/lib

# sudo cp -a /Library/Frameworks/CsoundLib64.framework/libs/. $SYSTEM_DEFAULTWORKINGDIRECTORY/csoundLibs

if [ $# == 0 ]; then
  echo "Must give branch name to build from"
  exit
else
  export BRANCH_NAME=$1
  echo "Using branch: $BRANCH_NAME"
fi

export CS_VERSION="6"

echo "Version: $CS_VERSION"
export MANUAL_DIR=`pwd`/../../../manual
export PACKAGE_NAME=csound${CS_VERSION}-MacOS_x86_64.pkg
export DMG_DIR="Csound${CS_VERSION}"
export DMG_NAME="csound${CS_VERSION}-MacOS_x86_64.dmg"

export SDK=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk/
export TARGET=10.10
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
	git clone -b $BRANCH_NAME https://github.com/csound/csound.git --depth=1 
	#cp -R csound5 csound5-f
    pwd
    echo "Csound should be cloned here"
    ls
fi
echo "Displaying current folder contents"
ls

#BUILD DOUBLES CSOUND
echo "Building Csound (double)..."

cd csound
ls


#$DEPS_BASE/bin/scons -j2 &> ../csound5_build_log.txt
mkdir build
cd build
export BUILD_DIR=`pwd`

echo "Building Csound (double)..."

cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DUSE_CURL=0 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=0 -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET -DCMAKE_OSX_SYSROOT=$SDK -DBUILD_STK_OPCODES=1 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DUSE_GETTEXT=0
make -j6 install

ls .

zip -r CsoundOSXBinaries.zip .
cp -rf CsoundOSXBinaries.zip $BUILD_ARTIFACTSTAGINGDIRECTORY/CsoundOSXBinaries.zip

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

export DIST=csound/build/dist
export BLD=csound/build

mkdir -p $APPS32_DIR
mkdir -p $APPS64_DIR
mkdir -p $FRAMEWORK32_DIR
mkdir -p $FRAMEWORK64_DIR
mkdir -p $SUPPORT_LIBS_DIR


mkdir -p $FRAMEWORK64_DIR/$PYTHON_DIR
mkdir -p $FRAMEWORK64_DIR/$JAVA_DIR
mkdir -p $FRAMEWORK64_DIR/$SAMPLES_DIR
mkdir -p $FRAMEWORK64_DIR/../Documentation
mkdir -p $FRAMEWORK64_DIR/Headers/


cp -rf include/* $FRAMEWORK64_DIR/Headers/

cp -R $DIST/CsoundLib64.framework/ $FRAMEWORK64_DIR
#cp -R csound5-f/CsoundLib.framework/ $FRAMEWORK32_DIR

# echo "Copying Python Libs... $PWD"

# cp $DIST/../_csnd6.so $FRAMEWORK64_DIR/$PYTHON_DIR
# cp $DIST/../csnd6.py $FRAMEWORK64_DIR/$PYTHON_DIR
# cp $BLD/../interfaces/ctcsound.py $FRAMEWORK64_DIR/$PYTHON_DIR


# echo "preparing framework..."

# cp  $DIST/lib/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/$CSLIBVERSION/
# cp  $DIST/lib/csnd6.jar $FRAMEWORK64_DIR/$JAVA_DIR
# cp  $DIST/lib/lib_jcsound.jnilib $FRAMEWORK64_DIR/$JAVA_DIR

# echo "copying manual..."

# #cp -Rf $MANUAL_DIR/html $FRAMEWORK32_DIR/Resources/
# #mv $FRAMEWORK32_DIR/Resources/html $FRAMEWORK32_DIR/Resources/Manual

# cp -Rf $MANUAL_DIR/html $FRAMEWORK64_DIR/Resources/
# mv $FRAMEWORK64_DIR/Resources/html $FRAMEWORK64_DIR/Resources/Manual

# echo "copying samples..."

# cp csound/samples/*.dat $FRAMEWORK64_DIR/$SAMPLES_DIR

# echo "copying apps..."

cp $DIST/bin/* $APPS64_DIR


# echo "copying support libs..."
# cp -a $SYSTEM_DEFAULTWORKINGDIRECTORY/csoundLibs/. $SUPPORT_LIBS_DIR
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
 
# # chnage IDs
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

# # change deps for libsndfile
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

# # change dep for libsamplerate
install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsamplerate.0.dylib
# # and for src_conv (NEEDS CHECKING!)
install_name_tool -change $DEPS_BASE/lib/libsamplerate.0.dylib $SUPPORT_LIBS_DIR/libsamplerate.0.dylib $APPS64_DIR/src_conv
install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib $SUPPORT_LIBS_DIR/libsndfile.1.dylib $APPS64_DIR/src_conv

install_name_tool -change /usr/local/opt/fltk/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk.1.3.dylib @loader_path/libfltk.1.3.dylib  $SUPPORT_LIBS_DIR/libfltk_forms.1.3.dylib
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @loader_path/libpng16.16.dylib  $SUPPORT_LIBS_DIR/libfltk_images.1.3.dylib
install_name_tool -change  /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/libportaudio.2.dylib $SUPPORT_LIBS_DIR/libfluidsynth.1.dylib

# # install name changes for libs under framework, luajit not included here
# # will make libsndfile location absolute to avoid linking problems for API clients using flat namespace.
# # @loader_path/../../ => /Library/Frameworks/CsoundLib64.framework/
install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib /Library/Frameworks/CsoundLib64.framework/libs/libsndfile.1.dylib $FRAMEWORK64_DIR/CsoundLib64
install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib /Library/Frameworks/CsoundLib64.framework/libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib

install_name_tool -change /usr/local/opt/libsndfile.1.dylib @loader_path/../../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# # absolute path in _csnd6.so
install_name_tool -change $BUILD_DIR/libcsnd6.6.0.dylib /Library/Frameworks/CsoundLib64.framework/Versions/6.0/libcsnd6.6.0.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# # absolute path in _csnd6.so
install_name_tool -change $BUILD_DIR/CsoundLib64.framework/Versions/6.0/CsoundLib64 /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64 $FRAMEWORK64_DIR/Versions/6.0/Resources/Python/Current/_csnd6.so

# # absolute path in libcsnd6.6.0.dylib
install_name_tool -change CsoundLib64.framework/Versions/6.0/CsoundLib64  /Library/Frameworks/CsoundLib64.framework/Versions/6.0/CsoundLib64  $FRAMEWORK64_DIR/Versions/6.0/libcsnd6.6.0.dylib

install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python Python.framework/Versions/2.7/Python $FRAMEWORK64_DIR/Resources/Opcodes64/libpy.dylib

install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Versions/6.0/Resources/Java/lib_jcsound.jnilib

# # Python Library now needs to be re-set to /Library/Frameworks 

echo "O you tool ==============="
otool -L CsoundLib64.framework/Versions/6.0/CsoundLib64

echo "------------"
# install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python /Library/Frameworks/Python.framework/Versions/2.7/Python $FRAMEWORK64_DIR/Resources/Opcodes64/libpy.dylib
install_name_tool -change /usr/local/opt/libsndfile/lib/libsndfile.1.dylib @loader_path/../../../../libs/libsndfile.1.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libstdutil.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk.1.3.dylib @loader_path/../../../../libs/libfltk.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk_images.1.3.dylib @loader_path/../../../../libs/libfltk_images.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk_forms.1.3.dylib @loader_path/../../../../libs/libfltk_forms.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk.1.3.dylib @loader_path/../../../../libs/libfltk.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk_images.1.3.dylib @loader_path/../../../../libs/libfltk_images.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change /usr/local/opt/fltk/lib/libfltk_forms.1.3.dylib @loader_path/../../../../libs/libfltk_forms.1.3.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libvirtual.dylib
install_name_tool -change /usr/local/opt/liblo/lib/liblo.7.dylib @loader_path/../../../../libs/liblo.7.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libosc.dylib
install_name_tool -change /usr/local/opt/portaudio/lib/libportaudio.2.dylib @loader_path/../../../../libs/libportaudio.2.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/librtpa.dylib
install_name_tool -change $DEPS_BASE/lib/libwiiuse.dylib @loader_path/../../../../libs/libwiiuse.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwiimote.dylib
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @loader_path/../../../../libs/libpng16.16.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libimage.dylib
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @loader_path/../../../../libs/libpng16.16.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libwidgets.dylib
install_name_tool -change /usr/local/opt/fluid-synth/lib/libfluidsynth.1.dylib @loader_path/../../../../libs/libfluidsynth.1.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libfluidOpcodes.dylib
install_name_tool -change /usr/local/opt/portmidi/lib/libportmidi.dylib @loader_path/../../../../libs/libportmidi.dylib $FRAMEWORK64_DIR/Resources/Opcodes64/libpmidi.dylib

echo "...setting permissions..."

cd installer

sudo chgrp -R admin  CsoundLib64/Package_Contents/Library
sudo chown -R root   CsoundLib64/Package_Contents/Library
sudo chmod -R 775    CsoundLib64/Package_Contents/Library

sudo chgrp -R wheel  CsoundApps64/Package_Contents/usr
sudo chown -R root   CsoundApps64/Package_Contents/usr
sudo chmod -R 755    CsoundApps64/Package_Contents/usr


echo "building packages ..."

pkgbuild --identifier com.csound.csoundEnvironment.csoundLib64 --root CsoundLib64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundLib64 CsoundLib64.pkg
pkgbuild --identifier com.csound.csoundEnvironment.csoundApps64 --root CsoundApps64/Package_Contents/ --version 1 --scripts ../../PkgResources/CsoundApps64 CsoundApps64.pkg

echo "building product..."

productbuild --distribution ../../Distribution2.dist --resources ../../PkgResources/en.lproj $PACKAGE_NAME

echo "assembling DMG..."

mkdir "$DMG_DIR" 
cd "$DMG_DIR"
cp ../$PACKAGE_NAME .
#cp  ../../../readme.pdf .
# cp  ../../../DmgResources/CsoundQt-0.9.6-MacOs.dmg .
#hdiutil create CsoundQT.dmg -srcfolder ../../../DmgResources/

cd ..
hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR" -fs HFS+ 

cp -rf $PACKAGE_NAME $BUILD_ARTIFACTSTAGINGDIRECTORY/$PACKAGE_NAME

echo "... finished."
ls
# open $INSTALLER_DIR
