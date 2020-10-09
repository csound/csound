#!/bin/bash
echo "Overrides for Csound's CMake build system, for building Csound for the Windows x64 installer."
echo
echo "Building Csound..."
mkdir csound-mingw64
cd csound-mingw64
pwd
rm -rf dist

# -DFIND_CMAKE_SYSTEM_PATH=//mingw64 \
# --trace-expand \

cmake ../.. -G "MSYS Makefiles" \
-DABLETON_LINK_HOME:PATH="D:\msys64\home\restore\link" \
-DBUILD_ABLETON_LINK_OPCODES:BOOL=0 \
-DBUILD_CSOUNDVST=1 \
-DBUILD_PD_CLASS=0 \
-DBUILD_STATIC_LIBRARY=1 \
-DBUILD_TESTS=0 \
-DBUILD_VST4CS_OPCODES=1 \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_VERBOSE_MAKEFILE=1 \
-DMUSICXML_LIBRARY:FILEPATH=/mingw64/bin/libmusicxml2.dll \
-DPTHREAD_LIBRARY:FILEPATH=/mingw64/x86_64-w64-mingw32/lib/libpthread.a \
-DTCL_VERSION=8.5 \
-DUSE_CURL=0 \
-DUSE_GETTEXT=0 \
-D_WIN32=1 \
-DPYTHON_INCLUDE_DIRS:PATH="C:/Program Files/Anaconda2/include" \
-DPYTHON_LIBRARIES:FILEPATH="C:/Program Files/Anaconda2/libs/python27.a" \
-DSTK_LIBRARY:FILEPATH=/mingw64/lib/libstk.a \
-DLIBSNDFILE_LIBRARY:FILEPATH="C:\Program Files\Mega-Nerd\libsndfile\bin\libsndfile-1.dll" \
-DSNDFILE_H_PATH:PATH="C:\Program Files\Mega-Nerd\libsndfile\include" \
-DBUILD_WEBSOCKET_OPCODE=0 \
-DMS_WIN64=1

if [ $? -ne 0 ]; then
    echo "Failed to run CMake."
    exit
fi
make -j6 $@
if [ $? -ne 0 ]; then
    echo "Failed to run make."
    exit
fi
echo "Compiling csound.node..."
echo "Compiling NSIS list of targets and dependencies..."
C:/Program_Files/Anaconda2/python ../find_csound_dependencies.py
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
./Setup_Csound6_x64_6.09.2beta2-vst.exe /SILENT
if [ $? -ne 0 ]; then
    echo "Failed to install Csound x64."
    exit
fi
echo "Finished building and installing Csound x64."

