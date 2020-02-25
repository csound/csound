#!/bin/bash
echo "Overrides for Csound's CMake build system."
echo
echo "Building Csound..."
mkdir csound-mingw64
cd csound-mingw64
pwd
rm -rf dist
cmake ../.. -G "MSYS Makefiles" \
-DUSE_GETTEXT=0 \
-DBUILD_CSOUNDVST=0 \
-DBUILD_PD_CLASS=0 \
-DBUILD_STATIC_LIBRARY=1 \
-DBUILD_VST4CS_OPCODES=0 \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_INSTALL_PREFIX=dist \
-DCMAKE_VERBOSE_MAKEFILE=0 \
-DMUSICXML_LIBRARY=D:/msys64/mingw64/bin/libmusicxml2.dll \
-DTCL_VERSION=8.5 \
-DUSE_CURL=0 \
-DSWIG_DIR=C:\msys2\mingw64\share\swig\3.0.6
if [ $? -ne 0 ]; then
    echo "Failed to run CMake."
    exit
fi
make -j6 $@
if [ $? -ne 0 ]; then
    echo "Failed to run make."
    exit
fi
make install
if [ $? -ne 0 ]; then
    echo "Failed to run make install."
    exit
fi

