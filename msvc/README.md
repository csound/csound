# Building Csound with MSVC on Windows
Please note, the following applies not only to the online AppVeyor build of Csound, but also to local builds of Csound using Microsoft Visual Studio (MSVS). The build works either with MSVS 2015, or with MSVS 2017.

Csound contains many dependencies, some of which do not work with the MSVS build yet. This page will document the current build status and will be updated accordingly as the work progresses. 

The goal is to have as much of Csound build in a native Windows manner as possible which will ease maintainence of the Windows port.

## Pre-requisites
1. MSVS 2017=5 x64 Community Edition (free) or greater 
    * Needs to have C++ tools installed, which isn't the detault
    * https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
    * In the installer, in the "Individual components" section. You should install "Python 2 64 bit", "VC++ 2015.3 toolset", "VC++ 2017 toolset", "Windows Universal CRT SDK", "Windows XP Support for C++", "Windows 10 SDK", "Windows 8.1 SDK" depending on the type of build you wish to create.
2. CMake version 3.6.0 or higher
    * https://cmake.org/files/v3.8/cmake-3.8.0-rc4-win64-x64.msi
    * Ensure Cmake is available on the Windows path
3. Git needs to be installed 
    * https://git-scm.com/download/win
    * Ensure Git is available on the Windows path

### Optional
Some other dependencies may need to be installed to enable all features within Csound or if building the installer (full) version of Csound. Most of these are system wide tools that a development machine might have installed anyway.

1. Python 2.7
    * Greater than 2.7 is not currently supported. Must install python headers and libs.
    * https://www.python.org/downloads/release/python-2713/
    * Visual Studio installer has the option of installing Python 2
    * Note: CMake (3.8.0) doesn't seem to find the python embedded in Anaconda, only the offical version
2. Java JDK
    * http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html
3. Inno Setup Installer
    * Only required if building the Csound Installer
    * http://www.jrsoftware.org/isdl.
4. SVN 
	* Currently required to acquire Portmidi. svn must be available on the path.
5. 7zip 
	* Required to extract GMM files. Only required if building this feature.

## VCPKG
The package manager used for Windows is VCPKG. It is still in beta/preview so it is not as integrated into Visual Studio as would be desired. It is advised to install it before downloading dependencies as the folder can grow to a large size (gigabytes). This is due to the source and binaries being housed within this folder. It can also be used system wide rather than just Csound so it is worth setting up.

The official instructions are here: https://github.com/Microsoft/vcpkg. The location of the folder should be added to the Windows Path so that vcpkg.exe can be invoked on the command line.

## Acquire dependencies and generate solution file
1. Launch a command window (Win key + r, type cmd and enter). Navigate to the Csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat" 
    * This should download all the required dependencies into the msvc folder (cache holds the downloaded files, deps is the extracted files, staging is for any manually built projects)
    * VCPKG will install it's packages in a specified folder elsewhere but it's CMAKE file will find them later
    * The first time downloading the packages with VCPKG can be very slow due to the number of packages and amount of building that occurs. It can take a few hours due to the large dependencies such as boost and GTK.
4  After this script has executed, execute the "generateProject.bat" script, which will invoke cmake with the proper settings to generate the solution.
4. Once this script has finished, the Visual Studio solution file should be produced and located in "Csound\msvc\csound-vs" folder.
5. Open this solution file in Visual Studio and build as normal. Just build the "ALLBUILD" project to build everything.

Alternatively, execute a batch file to do all of the above. `build.bat` should build Csound (but not CsoundQt or the Csound installer) using MSVS 2017; `build2015.bat` should build using MSVS 2015.

If you want to build CsoundQt in addition to Csound and then compile the Windows installer, you can adapt the following batch file, which emulates the complete AppVeyor build:

```
@echo off
echo Must call vcvars64.bat first!
set CSOUND_HOME=D:\\msys64\\home\\restore\\csound\\
set PYTHON=C:\Program_Files\Anaconda2\python.exe
set APPVEYOR_BUILD_FOLDER=D:\\msys64\\home\\restore\\csound\\
set VST_SDK_HOME=D:\\msys64\\mingw64\\include\\vstsdk2.4
set VCREDIST_CRT_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CRT
set VCREDIST_CXXAMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.CXXAMP
set VCREDIST_OPENMP_DIR=%VCINSTALLDIR%\\Redist\\x64\\Microsoft.VC140.OpenMP
set HDF5_HOME=C:\\Program Files\\HDF_Group\\HDF5\\1.8.19
powershell -ExecutionPolicy ByPass -File downloadDependencies.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140_xp"
rem powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140_xp"
powershell -ExecutionPolicy ByPass -File generateProject.ps1 -vsGenerator "Visual Studio 14 2015 Win64" -vsToolset "v140_xp" -vstSdkHome "D:\\msys64\\mingw64\\include\\vstsdk2.4"
cmake --build csound-vs --config Release
call build_csoundqt.bat
cd %APPVEYOR_BUILD_FOLDER%\\frontends\\nwjs
call C:\Program_Files\nodejs\nodevars.bat
call nw-gyp rebuild --target=0.23.5 --arch=x64 --msvs_version=2015
cd %APPVEYOR_BUILD_FOLDER%\\msvc
rem "C:\Program Files (x86)\Inno Setup 5\iscc.exe" /o. /dQtSdkBinDir="C:\\Qt\\Qt5.9.1\\5.9.1\\msvc2015_64\\bin\\" /dVcpkgInstalledBinDir="D:\\msys64\\home\\restore\\vcpkg\\installed\\x64-windows\\bin\\"  "..\installer\windows\csound6_x64_appveyor.iss"
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" /o. /dQtSdkBinDir="C:\\Qt\\Qt5.9.1\\5.9.1\\msvc2015_64\\bin\\" /dVcpkgInstalledBinDir="D:\\msys64\\home\\restore\\vcpkg\\installed\\x64-windows\\bin\\" /dInstallCsoundVst "..\installer\windows\csound6_x64_appveyor.iss"
```

## Development workflow
Once this project has been successfully generated as shown above, it does not require much further maintenance. To work on the Csound source, you can update the source directory via git pull and just rebuild the solution file in Visual Studio. There is no need to re-run the CMake command or script. Visual Studio detects changes within the project and will re-run the cmake command internally. 

If after updating the source directory the build fails due to a 3rd party library, re-run the "downloadDependencies.bat" to update to the latest versions. Rebuilding the solution should now work again. This will most likely be a rare event.

CMake GUI can be used on the "csound\msvc\csound-vs" folder to visually enable some features after the project has been initially set up. Make changes via the CMake GUI and hit "Configure" plus "Generate" to produce the solution file. Visual Studio will detect the solution file has changed and ask to be reloaded.

### Building 
In VS, select the build configuration from the top combo box. Either Debug, Release, Release with debug symbols, minimum size release. Also there is the option of x86/x64 (32/64 bit). We only support 64 bit currently however. The default project is the "ALL_BUILD" project, this will compile everything if you select to build it. "Build Solution" achieves the same thing. 

If a change to any of the cmakelists.txt occurs, VS will automatically run cmake and rebuild the solution. You should avoid running cmake externally to generate the project while VS is open and just let VS take care of it.

Optionally from the command line you can build by running "cmake --build . --config Release" in the csound-vs directory.

### Debugging 
The project can be launched for debugging in two ways:
1. First we need to change the default project as this what is attached to by default. The default with cmake is "ALL_BUILD" so when the user hits "Build Solution", it will build this project and indirectly, everything else. Right click the "csound-bin" project and click "Set As Startup Project". You can now select the green play button "Local Windows Debugger" to launch it. 
2. Leave the ALL_BUILD project as the default Startup Project and to launch the debugger, right-click the "csound-bin" project. Go to Debug and start new instance. This way, "Build Solution (shortcut F7)" still works.

You can pass in default arguments to the csound instance by right-clicking on "csound-bin" and going to properties. Go to "Debugging" and then in the command arguments, you can pass in a csd file or any other args to csound. This is useful for debugging a particular issue or walking through the code and seeing how it works.

### Profiling
Using the build configuration "Release" or "Release with debug info" will allow profilling of the project. By going to "Analyze" and "Performance Profiler", you will get a choice of profilling tools (cpu usage, memory etc).

## Work in progress / work to do
 - [Partial] Atomic builtins not being found, test program isn't working as expected but should
   * Fixed the cmake test but csound source assumes linux based atomics, needs more work
 - PureData, source download and extract
 - Faust opcodes, need to investigate
 - HDF5, need to investigate
 - Websockets, need to investigate
 - Csound~, needs max sdk
 - Unit testing for build tests. CUnit needs SVN checkout and build. Maybe switch to another up to date framework? Google test?
 - Doxygen for documentation
 - Installer, currently being re-written
