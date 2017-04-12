# Building Csound with MSVC on Windows
The following instructions are required to get Csound building and running using Microsoft Visual Studio 2017

Csound contains many dependencies, some of which do not work with the MSVC build yet. This page will document the current build status and will be updated accordingly as the work progresses. 

The goal is to have as much of Csound build in a native Windows manner as possible which will ease maintainence of the Windows port.

# Pre-requisites
1. Visual Studio 2017 x64 Community Edition (free) or greater 
 * Needs to have C++ tools installed, which isn't the detault
 * https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
2. CMake version 3.8.0 or higher
 * https://cmake.org/files/v3.8/cmake-3.8.0-rc4-win64-x64.msi
 * Ensure Cmake is available on the Windows path
3. Git needs to be installed and available on the path
 * https://git-scm.com/download/win

# VCPKG 
The package manager used for Windows is VCPKG. It is still in beta/preview so it is not as integrated into Visual Studio as would be desired. It is advised to install it before downloading dependencies as the folder can grow to a large size (gigabytes). This is due to the source and binaries being housed within this folder. It can also be used system wide rather than just Csound so it is worth setting up.

The official instructions are here: https://github.com/Microsoft/vcpkg
The location of the folder should be added to the Windows Path so that vcpkg.exe can be invoked on the command line.

# Acquire dependencies and generate solution file
1. Launch a command window (Win key + r, type cmd and enter). Navigate to the Csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat" 
 * This should download all the required dependencies into the msvc folder (cache holds the downloaded files, deps is the extracted files)
 * VCPKG will install it's packages in a specified folder elsewhere but it's CMAKE file will find them later
 * The first time downloading the packages with VCPKG can be very slow due to the number and amount of building that occurs. It can take a few hours due to large dependencies such as boost and GTK.
3. Once this script has finished, the Visual Studio solution file should be produced and located in "Csound\msvc\csound-vs" folder.
4. Open this solution file in Visual Studio and build as normal.

# Development workflow
Once this project has been generated, it does not require much further maintenance. You can update the csound source via git pull and simply build the solution file as normal. 
If a build failure occurs after updating the git repo, due to a 3rd party library, re-execute the "downloadDependencies.bat" as shown earlier to get the latest versions.