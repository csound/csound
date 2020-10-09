# Building Csound with MSVC on Windows

Csound can be built on Windows using Visual Studio via CMake. Windows currently does not have a native package manager like apt-get for Linux, so it is a bit more complicated to get dependencies. There is a powershell script which will download dependencies and generate the Visual Studio solution. The script is located in "csound/msvc". The "downloadDependencies" script will acquire all 3rd party code using VCPKG, Chocolatey or manually downloading. VCPKG is a package manager of sorts except it only acquires source code and requires the user to locally build each dependency. They can be shared globally on the system rather than per project. Chocolatey is a package manager for (usually large) installable dependencies e.g. QT, SWIG or Python. 

Once the solution file is produced, CMake will automatically rebuild the project if required (i.e. if the CMakeLists are edited).

## Pre-requisite steps

1. MSVS 2019 x64 Community Edition (free)
    * Needs to have C++ tools installed, which isn't the detault
    * <https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15>
    * In the installer, in the "Individual components" section. You should install "Python 2 64 bit" (easy way to install Python if not already on system), "VC++ 2019 toolset", "Windows Universal CRT SDK", "Windows 10 SDK"
2. CMake version 3.18.0 or higher
    * <https://cmake.org/download>
    * Ensure Cmake is available on the Windows path
3. Git latest version
    * <https://git-scm.com/download/win>
    * Ensure Git is available on the Windows path
4. Chocolatey latest
    * <https://chocolatey.org/>
    * Required for large binary dependencies

### Optional steps

Some other dependencies may need to be installed to enable all features within Csound or if building the installer (full) version of Csound. Most of these are system wide tools that a development machine might have installed anyway.

1. Python 2.7
    * Only required if building Python bindings or Opcodes related
    * Python 3 is not currently supported. Must install Python headers and libs.
    * <https://www.python.org/downloads/release/python-2713/>
    * Visual Studio installer has the option of installing Python 2
    * Note: CMake (3.8.0) doesn't seem to find the Python embedded in Anaconda, only the offical version
2. Java JDK 1.8
    * Only required if building Java interface
    * <http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html>
3. Inno Setup Installer 5/6
    * Only required if building the Csound Installer
    * <http://www.jrsoftware.org/isdl>
4. 7zip
    * Required to extract GMM files. Only required if building this feature.

## VCPKG - Windows Package Manager

VCPKG is used as the package manager for Windows. It will download the source for each dependency and build in Debug and Release for the architecture provided (e.g. 32/64 static/dynamic). These packages can be used in VS projects (automatically included) or CMake based projects afterwards. Packages are installed like other command line PMs such as NPM. It contains most popular libraries and even more obscure ones, mostly due to the ease of adding new libraries.

VCPKG can be used system wide. However, there is no versioning of packages in VCPKG. Every package is at the most current version that has been commited into the VCPKG main repository. For popular packages, this is usually the very latest version of that library. For individual projects that require a specific version, VCPKG is usually forked into a custom repo that contains the right versions for a specific project. This may be done for Csound eventually but right now, using the official VCPKG repo as a system wide version is sufficient.

The official instructions are here: <https://github.com/Microsoft/vcpkg>. The location of the folder should be added to the Windows Path so that vcpkg.exe can be invoked on the command line.

## Acquire dependencies and generate solution file

1. Launch a command window (Win key + r, type "cmd" and enter). Navigate to the Csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat"
    * This will download all the required dependencies into the msvc folder ("cache" folder holds the downloaded files, "deps" folder is the extracted files, "staging" folder is for any manually built projects)
    * VCPKG will build and install it's packages in it's own folder elsewhere but it's CMAKE file will find them later
    * The first time downloading the packages with VCPKG can be very slow due to the number of packages and amount of building that occurs.
4. Once this script has finished, the Visual Studio solution file will be produced and located in "Csound\msvc\csound-vs" folder.
5. Open this solution file in Visual Studio and build as normal. Execute "Build Solution" or "Build" the "ALLBUILD" project to build everything.

Alternatively, execute a batch file to do all of the above. `build.bat` should build Csound (but not Csound installer) using MSVS 2019.


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
