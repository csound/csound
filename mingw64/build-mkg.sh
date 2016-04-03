#!/bin/bash
echo "Overrides for Csound's CMake build system, for building Csound for the Windows x64 installer."
echo
echo "Building Csound..."
mkdir csound-mingw64
cd csound-mingw64
pwd
rm -rf dist
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
-D_WIN32=1 \
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
echo "Compiling NSIS list of targets and dependencies..."
../find_csound_dependencies.py
echo "Compiling Doxygen API documentation..."
if [ $? -ne 0 ]; then
    echo "Failed to identify Csound targets and dependencies."
    exit
fi
cd ../../doc
pwd
doxygen Doxyfile
if [ $? -ne 0 ]; then
    echo "Failed to create Csound API documentation."
    exit
fi
doxygen Doxyfile-CsoundAC
if [ $? -ne 0 ]; then
    echo "Failed to create CsoundAC API documentation."
    exit
fi
echo "Compiling Inno Setup installer..."
cd ../installer/windows
pwd
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" csound6_x64.iss
if [ $? -ne 0 ]; then
    echo "Failed to compile Inno Setup installer."
    exit
fi
echo "Uninstalling Csound x64..."
"C:\Program Files\Csound6_x64\unins000.exe" /SILENT
if [ $? -ne 0 ]; then
    echo "Failed to uninstall Csound x64, but continuing."
fi
echo "Installing Csound x64..."
./Setup_Csound6_x64_6.07.0.exe /SILENT
if [ $? -ne 0 ]; then
    echo "Failed to install Csound x64."
    exit
fi
echo "Finished building and installing Csound x64."

