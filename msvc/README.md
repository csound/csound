# Building Csound with MSVC on Windows
The following instructions are required to get Csound building and running using Microsoft Visual Studio 2017

Csound contains many dependencies, some of which do not work with the MSVC build yet. This page will document the current build status and will be updated accordingly as the work progresses. 

The goal is to have as much of Csound build in a native Windows manner as possible which will ease maintainence of the Windows port.

## Pre-requisites
1. Visual Studio 2017 x64 Community Edition (free) or greater 
    * Needs to have C++ tools installed, which isn't the detault
    * https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15
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
2. Java JDK
    * http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html

## VCPKG 
The package manager used for Windows is VCPKG. It is still in beta/preview so it is not as integrated into Visual Studio as would be desired. It is advised to install it before downloading dependencies as the folder can grow to a large size (gigabytes). This is due to the source and binaries being housed within this folder. It can also be used system wide rather than just Csound so it is worth setting up.

The official instructions are here: https://github.com/Microsoft/vcpkg. The location of the folder should be added to the Windows Path so that vcpkg.exe can be invoked on the command line.

## Acquire dependencies and generate solution file
1. Launch a command window (Win key + r, type cmd and enter). Navigate to the Csound source folder and to the "msvc" folder.
2. In the command window, execute the "downloadDependencies.bat" 
    * This should download all the required dependencies into the msvc folder (cache holds the downloaded files, deps is the extracted files, staging is for any manually built projects)
    * VCPKG will install it's packages in a specified folder elsewhere but it's CMAKE file will find them later
    * The first time downloading the packages with VCPKG can be very slow due to the number of packages and amount of building that occurs. It can take a few hours due to the large dependencies such as boost and GTK.
3. Once this script has finished, the Visual Studio solution file should be produced and located in "Csound\msvc\csound-vs" folder.
4. Open this solution file in Visual Studio and build as normal. Just build the "ALLBUILD" project to build everything.

## Development workflow
Once this project has been successfully generated as shown above, it does not require much further maintenance. To work on the Csound source, you can update the source directory via git pull and just rebuild the solution file in Visual Studio. There is no need to re-run the CMake command or script. Visual Studio detects changes within the project and will re-run the cmake command internally. 

If after updating the source directory the build fails due to a 3rd party library, re-run the "downloadDependencies.bat" to update to the latest versions. Rebuilding the solution should now work again. This will most likely be a rare event.

CMake GUI can be used on the "csound\msvc\csound-vs" folder to visually enable some features after the project has been initially set up. Make changes via the CMake GUI and hit "Configure" plus "Generate" to produce the solution file. Visual Studio will detect the solution file has changed and ask to be reloaded.

### Building 
In VS, select the build configuration from the top combo box. Either Debug, Release, Release with debug symbols, minimum size release. Also there is the option of x86/x64 (32/64 bit). We only support 64 bit currently however. The default project is the "ALL_BUILD" project, this will compile everything if you select to build it. "Build Solution" achieves the same thing. 

If a change to any of the cmakelists.txt occurs, VS will automatically run cmake and rebuild the solution. You should avoid running cmake externally while VS is open and just let it take care of it.

### Debugging 
The project can be launched for debugging in two ways:
1. First we need to change the default project as this what is attached to by default. The default with cmake is "ALL_BUILD" so when the user hits "Build Solution", it will build this project and indirectly, everything else. Right click the "csound-bin" project and click "Set As Startup Project". You can now select the green play button "Local Windows Debugger" to launch it. 
2. Leave the ALL_BUILD project as the default Startup Project and to launch the debugger, right-click the "csound-bin" project. Go to Debug and start new instance. This way, "Build Solution (shortcut F7)" still works.

You can pass in default arguments to the csound instance by right-clicking on "csound-bin" and going to properties. Go to "Debugging" and then in the command arguments, you can pass in a csd file or any other args to csound. This is useful for debugging a particular issue or walking through the code and seeing how it works.

### Profiling
Using the build configuration "Release" or "Release with debug info" will allow profilling of the project. The profilling tools may not appear if you have the community edition however, professional or enterprise might be required. By going to "Analyze" and "Performance Profiler", you will get a choice of profilling tools (cpu usage, memory etc).

## Build status
The table below indicates the current working state of the MSVC build. Over time this table will be fully filled out with all Windows relevant options running.

|CMake Switch/feature               | Dependenecy       | Working? |
|-----------------------------------|-------------------|----------|
|BUILD_CHUA_OPCODES                 |                   |          |
|BUILD_CSBEATS                      |                   |          |
|BUILD_CSOUNDVST                    |                   |          |
|BUILD_CSOUND_AC                    |                   |          |
|BUILD_CSOUND_AC_LUA_INTERFACE      |                   |          |
|BUILD_CSOUND_AC_PYTHON_INTERFACE   |                   |          |
|BUILD_CXX_INTERFACE                |                   | Working |
|BUILD_DSSI_OPCODES                 | Ladspa            | Disable? |
|BUILD_FAUST_OPCODES                |                   |          |
|BUILD_FLUID_OPCODES                |                   |          |
|BUILD_IMAGE_OPCODES                |                   | Working |
|BUILD_INSTALLER                    |                   |          |
|BUILD_JACK_OPCODES                 |                   | Disabled |
|BUILD_JAVA_INTERFACE               |                   | Working |
|BUILD_LINEAR_ALGEBRA_OPCODES       | Eigen             | Enabled/Not being found |
|BUILD_LUA_INTERFACE                |                   |          |
|BUILD_LUA_OPCODES                  |                   |          |
|BUILD_MULTI_CORE                   |                   |          |
|BUILD_OSC_OPCODES                  | Liblo             | Enabled/Needs work |
|BUILD_P5GLOVE_OPCODES              |                   |          |
|BUILD_PYTHON_INTERFACE             |                   |          |
|BUILD_PYTHON_OPCODES               |                   |          |
|BUILD_RELEASE                      |                   | Working |
|BUILD_SERIAL_OPCODES               |                   |          |
|BUILD_STATIC_LIBRARY               |                   | Working |
|BUILD_STK_OPCODES                  |                   |          |
|BUILD_TESTS                        |                   | Enabled/Needs work |
|BUILD_UTILITIES                    |                   | Working |
|BUILD_VIRTUAL_KEYBOARD             | FLTK              | Enabled/Needs work |
|BUILD_WIIMOTE_OPCODES              |                   |          |
|BUILD_WINDSOUND                    |                   |          |
|USE_ALSA                           |                   | Disabled |
|USE_ATOMIC_BUILTIN                 |                   | Enable/Not working |
|USE_AUDIOUNIT                      |                   | Disabled |
|USE_COMPILER_OPTIMIZATIONS         |                   | Working |
|USE_COREMIDI                       |                   | Disabled |
|USE_DOUBLE                         | -                 | Working |
|USE_FLTK                           |                   | Enabled/Not being found |
|USE_GETTEXT                        | VCPKG             | Enabled/Not being found |
|USE_IPMIDI                         |                   |          |
|USE_JACK                           |                   | Disabled |
|USE_LIB64                          |                   | Working |
|USE_LRINT                          |                   | Disabled |
|USE_OPEN_MP                        |                   |          |
|USE_PORTAUDIO                      |                   | Working |
|USE_PORTMIDI                       | Manual build      | Enabled/Needs work |
|USE_PULSEAUDIO		                  |                   | Disabled |
|USE_SYSTEM_PORTSMF                 |                   |          |

## Work in progress / work to do
1. Python integration into build. Most likely user will need to install it via msi
2. PortMIDI/SMF manual build script needs to be created
3. FLTK not being fully found. Everything is found except fluid_executable
4. Atomic builtins not being found, test program isn't working as expected but should
5. Liblo (OSC) difficult build, needs premake on windows. Need to either prebuild or come up with build script
6. SWIG, binaries to be downloaded manually and extracted
7. STK, source downloaded manually and built
8. LUAJIT, source downloaded manually and built using VS console
9. PureData, source download and extract
10. GetText, is in vcpkg but not being detected by cmake. (missing:  GETTEXT_MSGMERGE_EXECUTABLE GETTEXT_MSGFMT_EXECUTABLE)
11. Eigen, is in vcpkg but include path isn't being found
12. FluidSynth, hard to build. Is there a windows binary download?
13. Explicitly disable non-windows orientated features in cmake. Jack/Ladspa/ALSA/coreaudiomidi etc
14. WII opcodes, download source and build
15. P5Glove, no idea about this
16. Faust opcodes, need to investigate
17. HDF5, need to investigate
18. Websockets, need to investigate
19. Csound AC, 
20. Csound~, needs max sdk
21. Unit testing for build tests. CUnit needs SVN checkout and build. Maybe switch to another up to date framework? Google test?
22. Doxygen for documentation
23. Installer needs work? Not tried
