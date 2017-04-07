# Building Csound with MSVC on Windows
The following instructions are required to get Csound building and running using Microsoft Visual Studio 2017

Csound contains many dependencies, some of which do not work with the MSVC build yet. This page will document the current build status and will be updated accordingly as the work progresses. 

The goal is to have as much of Csound build in a native Windows manner as possible which will ease maintainence of the Windows port.

# Pre-requisites
1. Visual Studio 2017 x64 Community Edition or greater (free)
 * Needs to have C++ tools intstalled, which isn't the detault
 * https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
2. CMake version 3.8.0 (needs to support VS2017 so the latest version is recommended)
 * https://cmake.org/files/v3.8/cmake-3.8.0-rc4-win64-x64.msi
 * Ensure cmake is available on the Windows path by opening a command prompt and launching cmake

# Acquire dependencies and generate solution file
1. Launch a command window (Win key + r, type cmd and enter). Navigate to the csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat" 
 * This should download all the required dependencies into the msvc folder (cache holds the downloaded files, deps is the extracted files)
 * VCPKG will install it's packages in a specified folder elsewhere but it's CMAKE file will find them
 * This can take a long time the first time as VCPKG will download all sources and build debug and release versions. Especially for boost and GTK
3. Once the dependencies are downloaded, in the same CMD window, execute the "build.bat" to produce the solution file
4. Open this solution file in Visual Studio and build as normal.

# Development workflow
Once this project has been generated, it does not require much further maintenance. You can update the csound source via git pull and simply re-build the solution file.
If a build failure occurs after updating the git repo, due to a 3rd party library, re-execute the "downloadDependencies.bat" as shown earlier to get the latest versions.