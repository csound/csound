@echo off
echo In essence, this script locally emulates the AppVeyor build of Csound.
echo The script produces a local build of Csound using Microsoft Visual
echo Studio, and including the VST features that cannot be hosted in GitHub
echo releases. You must adjust the environment variables below to match your
echo locations, and you must call vcvars64.bat before running this script!
set CSOUND_HOME=D:\\msys64\\home\\restore\\csound\\
set PYTHON=C:\Program_Files\Anaconda2\python.exe
set APPVEYOR_BUILD_FOLDER=D:\\msys64\\home\\restore\\csound\\
set VST_SDK_HOME=D:\\msys64\\mingw64\\include\\vstsdk2.4
set VCREDIST_CRT_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CRT
set VCREDIST_CXXAMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CXXAMP
set VCREDIST_OPENMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.OpenMP
set HDF5_HOME=C:\\Program Files\\HDF_Group\\HDF5\\1.8.19
set ABLETON_LINK_HOME="D:\\msys64\\home\\restore\\csound\\msvc\\deps\\link"
cd %APPVEYOR_BUILD_FOLDER%\\msvc
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" /o. /dQtSdkBinDir="C:\\Qt\\Qt5.9.1\\5.9.1\\msvc2015_64\\bin\\" /dVcpkgInstalledBinDir="D:\\msys64\\home\\restore\\vcpkg\\installed\\x64-windows\\bin\\" /dInstallCsoundVst "..\installer\windows\csound6_x64_appveyor.iss"
