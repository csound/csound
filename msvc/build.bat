@echo off

mkdir csound-vs 
cd csound-vs 
REM cmake ..\.. -G "Visual Studio 14 2015" -T v140_clang_c2 -DCMAKE_INSTALL_PREFIX=dist -DCUSTOM_CMAKE=..\Custom-vs.cmake -DUSE_GETTEXT=0 -DSWIG_DIR=C:\msys64\mingw64\share\swig\3.0.6  -DUSE_CURL=0 -DBUILD_FLUID_OPCODES=0 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DBUILD_CSOUND_AC_LUA_INTERFACE=0  -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short" -DLIBSNDFILE_LIBRARY="c:\Program Files\Mega-Nerd\libsndfile\lib\libsndfile.lib"  -DPTHREAD_LIBRARY="c:\msys64\mingw64\bin\libwinpthread-1.dll" -DCMAKE_SYSTEM_INCLUDE_PATH="c:\msys64\usr\include"
REM cmake ..\.. -G "Visual Studio 14 2015" -DCMAKE_INSTALL_PREFIX=dist -DCUSTOM_CMAKE=..\Custom-vs.cmake -DUSE_GETTEXT=0 -DSWIG_DIR=C:\msys64\mingw64\share\swig\3.0.6  -DUSE_CURL=0 -DBUILD_FLUID_OPCODES=0 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DBUILD_CSOUND_AC_LUA_INTERFACE=0  -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short" -DLIBSNDFILE_LIBRARY="c:\Program Files\Mega-Nerd\libsndfile\lib\libsndfile.lib"  -DPTHREAD_LIBRARY="c:\msys64\mingw64\bin\libwinpthread-1.dll" -DCMAKE_SYSTEM_INCLUDE_PATH="c:\msys64\usr\include" -DBUILD_PD_CLASS=0
REM cmake ..\.. -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX=dist -DCUSTOM_CMAKE=..\Custom-vs.cmake -DUSE_GETTEXT=0 -DSWIG_DIR=C:\msys64\mingw64\share\swig\3.0.6  -DUSE_CURL=0 -DBUILD_FLUID_OPCODES=0 -DBUILD_LUA_OPCODES=0 -DBUILD_LUA_INTERFACE=0 -DBUILD_CSOUND_AC_LUA_INTERFACE=0  -DHAVE_BIG_ENDIAN=0 -DCMAKE_16BIT_TYPE="unsigned short" -DLIBSNDFILE_LIBRARY="..\deps\lib\libsndfile-1.lib"  -DBUILD_PD_CLASS=0 -DUSE_DOUBLE=1 -DFLEX_EXECUTABLE="..\deps\win_flex_bison\win_flex.exe" -DBISON_EXECUTABLE="..\deps\win_flex_bison\win_bison.exe"
cmake ..\.. -G "Visual Studio 15 2017 Win64" ^
 -DCMAKE_BUILD_TYPE="RELEASE" ^
 -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
 -DCMAKE_INSTALL_PREFIX=dist ^
 -DCUSTOM_CMAKE=..\Custom-vs.cmake ^
 -DHAVE_BIG_ENDIAN=0 ^
 -DCMAKE_16BIT_TYPE="unsigned short" ^
 -DUSE_DOUBLE=1 ^
 -DUSE_GETTEXT=0 ^
 -DUSE_CURL=0 ^
 -DBUILD_FLUID_OPCODES=0 ^
 -DBUILD_LUA_OPCODES=0 ^
 -DBUILD_LUA_INTERFACE=0 ^
 -DBUILD_CSOUND_AC_LUA_INTERFACE=0 ^
 -DBUILD_PD_CLASS=0 ^
 -DLIBSNDFILE_LIBRARY="..\deps\lib\libsndfile-1.lib" ^
 -DSWIG_DIR="C:\msys64\mingw64\share\swig\3.0.6" ^
 -DFLEX_EXECUTABLE="..\deps\win_flex_bison\win_flex.exe" ^
 -DBISON_EXECUTABLE="..\deps\win_flex_bison\win_bison.exe"

cd ..
