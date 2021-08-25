# Building Csound with MSVC on Windows

Visual Studio via CMake is the supported build platform for Windows with Csound. MSYS or Cygwin may no longer work and are not actively maintained. System dependencies for Windows can be installed with [Chocolately](https://chocolatey.org/) or [Scoop](https://scoop.sh/). [VCPKG](https://github.com/microsoft/vcpkg) is used as the 3rd party library dependency manager.

Once the solution file is produced, CMake will automatically rebuild the project if required (i.e. if the CMakeLists are edited).

## Pre-requisite steps

1. MSVS 2019 x64 Community Edition (free)
    - Needs to have C++ tools installed, which isn't the detault
    - <https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15>
    - In the installer, in the "Individual components" section. You should install "Python 2 64 bit" (easy way to install Python if not already on system), "VC++ 2019 toolset", "Windows Universal CRT SDK", "Windows 10 SDK"
2. CMake version 3.18.0 or higher
    - <https://cmake.org/download>
    - Ensure Cmake is available on the Windows path
3. Git latest version
    - <https://git-scm.com/download/win>
    - Ensure Git is available on the Windows path
4. Chocolatey latest
    - <https://chocolatey.org/>
    - Required for system level dependencies

## Optional steps

Some other dependencies may need to be installed to enable all features within Csound or if building the installer (full) version of Csound. Most of these are system wide tools that a development machine might have installed anyway.

1. Python 2.7
    - Only required if building Python bindings or Opcodes related
    - Python 3 is not currently supported. Must install Python headers and libs.
    - <https://www.python.org/downloads/release/python-2713/>
    - Visual Studio installer has the option of installing Python 2
    - Note: CMake (3.8.0) doesn't seem to find the Python embedded in Anaconda, only the offical version
2. Java JDK 1.8
    - Only required if building Java interface
    - <http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html>
3. Inno Setup Installer 6
    - Only required if building the Csound Installer
    - <http://www.jrsoftware.org/isdl>
4. 7zip
    - Required to extract GMM files. Only required if building this feature.

## Acquire dependencies and generate solution file

After doing the pre-requiste steps, open a powershell:
\*Note: Powershell should be opened with admin/elevated rights for Chocolatey to install dependencies correctly. You will get a warning otherwise.

1. Install system level dependencies
    1. choco install -y swig winflexbison
2. Ensure VCPKG submodule is pulled down
    1. git submodule update --init --recursive
    2. If cloning: git clone X --recursive
3. Configure CMake and install VCPKG packages
    1. cmake -B build -S . -DUSE_VCPKG=1
4. (Optionally) Building via command line
    1. cmake --build build --config Release|Debug

The project output will be under "csound/build". The solution can be opened in Visual Studio now. You can also open the root folder in either Visual Studio or VSCode and both editors will detect CMake and work accordingly.

## Development workflow

Once this project has been successfully generated as shown above, it does not require much further maintenance. To work on the Csound source, you can update the source directory via git pull and Visual Studio will detect any further CMake changes and rebuild the solution accordingly. There is no need to re-run the CMake command explicitly unless you make changes to the VCPKG manifest.

CMake GUI can be used on the "csound\build" folder to visually enable some features after the project has been initially set up. Make changes via the CMake GUI and hit "Configure" plus "Generate" to produce the solution file. Visual Studio will detect the solution file has changed and ask to be reloaded.

### Building

In VS, select the build configuration from the top combo box. Either Debug, Release, Release with debug symbols, minimum size release. Also there is the option of x86/x64 (32/64 bit). We only support 64 bit currently however. The default project is the "ALL_BUILD" project, this will compile everything if selected. Hitting "Build Solution" achieves the same thing.

### Debugging

The project can be launched for debugging in two ways:

1. First we need to change the default project as this what is attached to by default. The default with cmake is "ALL_BUILD" so when the user hits "Build Solution", it will build this project and indirectly, everything else. Right click the "csound-bin" project and click "Set As Startup Project". You can now select the green play button "Local Windows Debugger" to launch it.
2. Leave the ALL_BUILD project as the default Startup Project and to launch the debugger, right-click the "csound-bin" project. Go to Debug and start new instance. This way, "Build Solution (shortcut F7)" still works.

You can pass in default arguments to the csound instance by right-clicking on "csound-bin" and going to properties. Go to "Debugging" and then in the command arguments, you can pass in a csd file or any other args to csound. This is useful for debugging a particular issue or walking through the code and seeing how it works.

### Profiling

Using the build configuration "Release" or "Release with debug info" will allow profilling of the project. By going to "Analyze" and "Performance Profiler", you will get a choice of profilling tools (cpu usage, memory etc).
