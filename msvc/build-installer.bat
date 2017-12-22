@echo off
echo Must call vcvars64.bat first!
cls
set BUILD_DEPENDENCIES=0
set BUILD_CSOUND=0
set BUILD_CSOUND_NODE=0
set BUILD_CSOUNDQT=0
set BUILD_INSTALLER=1

set PATH=C:\Qt\Qt5.9.1\5.9.1\msvc2015_64\bin;%PATH%
set ABLETON_LINK_HOME=D:\\msys64\\home\\restore\\csound\\msvc\\deps\\link\\
set CSOUND_HOME=D:\\msys64\\home\\restore\\csound\\
set PYTHON=C:\Program_Files\Anaconda2\python.exe
set APPVEYOR_BUILD_FOLDER=D:\\msys64\\home\\restore\\csound\\
set CSOUND_VERSION=6.10.0-rc2
set VCREDIST_CRT_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CRT
set VCREDIST_CXXAMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CXXAMP
set VCREDIST_OPENMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.OpenMP
set HDF5_HOME=C:\\Program Files\\HDF_Group\\HDF5\\1.8.19
set VST_SDK2_HOME=D:/msys64/home/restore/csound/msvc/deps/VST_SDK/VST2_SDK

if %BUILD_DEPENDENCIES%==1 (
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140"
)

if %BUILD_CSOUND%==1 (
powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140" -vstSdkHome "%VST_SDK2_HOME%"
cmake --build csound-vs --config RelWithDebInfo
)

if %BUILD_CSOUNDQT%==1 (
call build-csoundqt.bat
)

if %BUILD_CSOUND_NODE%==1 (
cd %APPVEYOR_BUILD_FOLDER%\\frontends\\nwjs
call C:\Program_Files\nodejs\nodevars.bat
call nw-gyp rebuild --target=0.23.5 --arch=x64 --msvs_version=2015
)

if %BUILD_INSTALLER%==1 (
cd %APPVEYOR_BUILD_FOLDER%\\msvc
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" /o. /dQtSdkBinDir="C:\\Qt\\Qt5.9.1\\5.9.1\\msvc2015_64\\bin\\" /dVcpkgInstalledBinDir="D:\\msys64\\home\\restore\\vcpkg\\installed\\x64-windows\\bin\\" /dInstallCsoundVst "..\installer\windows\csound6_x64_appveyor.iss"
)
