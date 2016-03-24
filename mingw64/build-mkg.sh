#!/bin/bash
echo "Overrides for Csound's CMake build system, for building Csound for the Windows x64 installer."
echo "Building Csound..."
mkdir csound-mingw64
cd csound-mingw64
pwd
cmake ../.. -G "MSYS Makefiles" \
-DUSE_GETTEXT=0 \
-DBUILD_CSOUNDVST=1 \
-DBUILD_PD_CLASS=0 \
-DBUILD_STATIC_LIBRARY=0 \
-DBUILD_TESTS=0 \
-DBUILD_VST4CS_OPCODES=1 \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_VERBOSE_MAKEFILE=1 \
-DMUSICXML_LIBRARY=D:/msys64/mingw64/bin/libmusicxml2.dll \
-DNEED_PORTTIME=0 \
-DTCL_VERSION=8.5 \
-DUSE_CURL=0 \
-DUSE_OPEN_MP=0 \
-DSWIG_DIR=C:\msys2\mingw64\share\swig\3.0.6
make -j6 $@
if [ $retval -ne 0 ]; then
    exit
fi
echo "Compiling NSIS list of targets and dependencies."
../find_csound_dependencies.py
echo "Compiling Doxygen API documentation..."
cd ../../doc
pwd
doxygen Doxyfile
doxygen Doxyfile-CsoundAC
echo "Compiling Inno Setup installer..."
cd ../installer/windows
pwd
C:/Program_Files_x86/Inno\ Setup\ 5/iscc.exe csound6_x64.iss
if [ $retval -ne 0 ]; then
    exit
fi
