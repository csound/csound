#!/bin/sh

export CS_TILDE_VERSION=1.1.3
export PACKAGE_NAME=csound~_${CS_TILDE_VERSION}.pkg
export DMG_DIR="csound~_${CS_TILDE_VERSION}"
export DMG_NAME="csound~_${CS_TILDE_VERSION}.dmg"
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
	git clone -b $BRANCH_NAME file://$PWD/../../../.. csound6 --depth=1 
	#cp -R csound5 csound5-f
fi


#BUILD DOUBLES CSOUND
echo "Building Csound (double)..."
cd csound6
cp ../../../../../Custom.cmake .

#/usr/local/bin/scons -j2 &> ../csound5_build_log.txt
mkdir build
cd build
# RUN CMAKE TWICE TO GET AROUND ISSUE WITH UNIVERSAL BUILD
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=0
cmake .. -DBUILD_INSTALLER=1 -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="i386;x86_64" -DBUILD_TESTS=0 -DBUILD_CSOUND_AC=0
make -j6 csound~.mxo

cd ../..


# ASSEMBLE FILES FOR INSTALLER
export PACKAGE_CONTENTS="installer/Package_Contents"
export MAX_7_INSTALL_DIR="$PACKAGE_CONTENTS/Users/Shared/Max 7/"
export EXTERNALS_DIR="${MAX_7_INSTALL_DIR}/Library/csound~/externals"
export HELP_DIR="${MAX_7_INSTALL_DIR}/Library/csound~/help"
export EXAMPLES_DIR="${MAX_7_INSTALL_DIR}/Examples"

mkdir -p "$EXTERNALS_DIR"
mkdir -p "$HELP_DIR"
mkdir -p "$EXAMPLES_DIR"
cp -r csound6/build/frontends/max_csound_tilde/csound~.mxo "$EXTERNALS_DIR"
cp ../../help/csound~/* "$HELP_DIR"
cp -r ../../examples "$EXAMPLES_DIR/csound~"


echo "building packages ..."

#pkgbuild --identifier com.csound.csound6Environment.csound~ --root installer/Package_Contents/ --version 1 --scripts ../PkgResources/csound~ csound~_v1.1.0.pkg
pkgbuild --identifier com.csound.csound6Environment.csound~ --root installer/Package_Contents/ --version 1  csound~_v${CS_TILDE_VERSION}.pkg

#echo "building product..."

#productbuild --distribution ../../Distribution.dist --resources ../../PkgResources/en.lproj $PACKAGE_NAME

#echo "assembling DMG..."

#mkdir "$DMG_DIR" 
#cd "$DMG_DIR"
#cp ../$PACKAGE_NAME .
#cp -R ../../../DmgResources/* .
#ln -s /Applications .
#cd ..

#hdiutil create "$DMG_NAME" -srcfolder "$DMG_DIR"

echo "... finished."

open $INSTALLER_DIR
