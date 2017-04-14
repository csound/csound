# Building Csound with MSVC on Windows
The following instructions are required to get Csound building and running using Microsoft Visual Studio 2017

Csound contains many dependencies, some of which do not work with the MSVC build yet. This page will document the current build status and will be updated accordingly as the work progresses. 

The goal is to have as much of Csound build in a native Windows manner as possible which will ease maintainence of the Windows port.

# Pre-requisites
1. Visual Studio 2017 x64 Community Edition (free) or greater 
  * Needs to have C++ tools installed, which isn't the detault
  * https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
2. CMake version 3.6.0 or higher
  * https://cmake.org/files/v3.8/cmake-3.8.0-rc4-win64-x64.msi
  * Ensure Cmake is available on the Windows path
3. Git needs to be installed 
  * https://git-scm.com/download/win
  * Ensure Git is available on the Windows path

# VCPKG 
The package manager used for Windows is VCPKG. It is still in beta/preview so it is not as integrated into Visual Studio as would be desired. It is advised to install it before downloading dependencies as the folder can grow to a large size (gigabytes). This is due to the source and binaries being housed within this folder. It can also be used system wide rather than just Csound so it is worth setting up.

The official instructions are here: https://github.com/Microsoft/vcpkg. The location of the folder should be added to the Windows Path so that vcpkg.exe can be invoked on the command line.

# Acquire dependencies and generate solution file
1. Launch a command window (Win key + r, type cmd and enter). Navigate to the Csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat" 
  * This should download all the required dependencies into the msvc folder (cache holds the downloaded files, deps is the extracted files, staging is for any manually built projects)
  * VCPKG will install it's packages in a specified folder elsewhere but it's CMAKE file will find them later
  * The first time downloading the packages with VCPKG can be very slow due to the number of packages and amount of building that occurs. It can take a few hours due to the large dependencies such as boost and GTK.
3. Once this script has finished, the Visual Studio solution file should be produced and located in "Csound\msvc\csound-vs" folder.
4. Open this solution file in Visual Studio and build as normal. Just build the "ALLBUILD" project to build everything.

# Development workflow
Once this project has been generated as above, it does not require much further maintenance. To work on the Csound source, you can update via git pull and just rebuild the solution file as required. There is no need to re-run the CMAKE command or script. Only in the event of the source being updated to use a newer 3rd party library, then you should run the dependency script "downloadDependencies.bat" again as the build will fail otherwise.

## Building 
In VS, select the build configuration from the top combo box. Either Debug, Release, Release with debug symbols, minimum size release. Also there is the option of x86/x64 (32/64 bit). We only support 64 bit currently however. The default project is the "ALL_BUILD" project, this will compile everything if you select to build it. "Build Solution" achieves the same thing. 

If a change to any of the cmakelists.txt occurs, VS will automatically run cmake and rebuild the solution. You should avoid running cmake externally while VS is open and just let it take care of it.

## Debugging 
The project can be launched for debugging in two ways:
1. First we need to change the default project as this what is attached to by default. The default with cmake is "ALL_BUILD" so when the user hits "Build Solution", it will build this project and indirectly, everything else. Right click the "csound-bin" project and click "Set As Startup Project". You can now select the green play button "Local Windows Debugger" to launch it. 
2. Leave the ALL_BUILD project as the default Startup Project and to launch the debugger, right-click the "csound-bin" project. Go to Debug and start new instance. This way, "Build Solution (shortcut F7)" still works.

You can pass in default arguments to the csound instance by right-clicking on "csound-bin" and going to properties. Go to "Debugging" and then in the command arguments, you can pass in a csd file or any other args to csound. This is useful for debugging a particular issue or walking through the code and seeing how it works.

## Profiling
Using the build configuration "Release" or "Release with debug info" will allow profilling of the project. The profilling tools may not appear if you have the community edition however, professional or enterprise might be required. By going to "Analyze" and "Performance Profiler", you will get a choice of profilling tools (cpu usage, memory etc).