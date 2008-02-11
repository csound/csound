'''
S C O N S   C U S T O M I Z A T I O N   F I L E
Michael Gogins

In general, the purpose of a customization file for SCons
is to specify build options for third-party dependencies 
that are not installed in 'standard locations'
(e.g. /usr or /usr/local or /opt or /pkg on Linux).

Order is important: place local paths ahead of system paths.

The purpose of this particular customization file is enable the use of 
Microsoft Visual Studio 2005, free edition, to build Csound 
for the Windows installers.

Usage (for example):

scons withMSVC=1 custom=custom-msvc-release.py useDouble=1 etc...

This file assumes that all of Csound's third-party dependencies have
been installed, or built, in "U:\". On my computer, this is a network
mapping to the actual directory.

If you also have installed or built all your open source libraries
in one directory, you also can map that directory to "U:\" and 
use this customization file without modification.
'''
import sys

customCPPPATH = []
customCCFLAGS = []
customCXXFLAGS = []
customLIBS = []
customLIBPATH = []
customSHLINKFLAGS = []
customSWIGFLAGS = []

if sys.platform[:3] == 'win':
    customCXXFLAGS.append('-EHsc')
    customCCFLAGS.append("-DMSVC")
    customCCFLAGS.append("-MD")
    customCCFLAGS.append("-W2")
    #customCCFLAGS.append("-Ob2")
    ################################################################
    # If you want to build Csound,
    # libsndfile is REQUIRED.
    customCPPPATH.append(r'U:\libsndfile-1_0_17')
    customLIBPATH.append(r'U:\libsndfile-1_0_17')
    ################################################################
    # If you want to build PortAudio,
    # for real-time audio performance.
    customCPPPATH.append(r'U:\portaudio\include')
    customLIBPATH.append(r'U:\portaudio\build\msvc\Win32\Debug')
    ################################################################
    # If you want to build PortMidi,
    # for real-time MIDI performance.
    customCPPPATH.append(r'U:\portmidi\pm_common')
    customCPPPATH.append(r'U:\portmidi\pm_win')
    customCPPPATH.append(r'U:\portmidi\porttime')
    customLIBPATH.append(r'U:\portmidi\pm_win')
    customLIBPATH.append(r'U:\portmidi\porttime')
    customLIBPATH.append(r'U:\portmidi\pm_win\Debug')
    ################################################################
    # If you want to build the FLTK widgets, csound5gui, cseditor,
    # CsoundAC, or CsoundVST, FLTK 1.1x is required.
    customCPPPATH.append(r'U:\fltk')
    customLIBPATH.append(r'U:\fltk\lib')
    ################################################################
    # If you want to build the image opcodes,
    # libpng and libz are required.
    customCPPPATH.append(r'U:\zlib')
    customCPPPATH.append(r'U:\libpng-1.2.24')
    customLIBPATH.append(r'U:\libpng-1.2.24\projects\visualc71\Win32_DLL_Debug')
    ################################################################
    # If you want to build the Python opcodes, the Python interfaces
    # to Csound, or CsoundAC, Python 2.5 is required.
    customCPPPATH.append(r'U:\Python25\include')
    customLIBPATH.append(r'U:\Python25\Libs')
    ################################################################
    # If you want to build CsoundVST, you need the 
    # Steinberg VST SDK.
    customCPPPATH.append(r'U:\vstsdk2.4')    
    ################################################################
    # If you want to build CsoundAC, you need the 
    # boost C++ libraries.
    customCPPPATH.append(r'U:\boost')
    ################################################################
    # If you want to build scoregen, you need the include
    # path to VST MIDI plugin SDK header files. 
    customCPPPATH.append(r'U:\VSTModuleArchitectureSDK\pluginterfaces\base')    
    ################################################################
    # If you want to build Lua interfaces, you need Lua.
    customCPPPATH.append(r'U:\\')
    customLIBPATH.append(r'U:\\')
    ################################################################
    # If you want to build Tcl\Tk interfaces
    # or Tclcsound, you need Tcl\Tk.
    # Add it here:
    customCPPPATH.append(r'U:\Tcl\include')
    customLIBPATH.append(r'U:\Tcl\bin')
    ################################################################
    # If you want to build the FluidSynth opcodes,
    # you need FluidSynth. 
    customLIBPATH.append(r'U:\fluidsynth\winbuild\fluidsynth_dll\Debug')
    customCPPPATH.append(r'U:\fluidsynth\include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. 
    customCPPPATH.append(r'U:\jdk1.5.0\include')
    customCPPPATH.append(r'U:\jdk1.5.0\include\win32')
    ################################################################
    # If you want to build the Pure Data external csoundapi~,
    # you need Pure Data. 
    # add it here (you do NOT need to build it first):
    customCPPPATH.append(r'U:\pd-0.38-4-devel-2\src')
    customLIBPATH.append(r'U:\pd-0.38-4-devel-2\bin')
    ################################################################
    # If you want to build the Loris opcodes,
    # then copy Loris to csound5\Opcodes\Loris. Loris also
    # requires FFTW, if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append(r'U:\fftw-3.0.1\api')
    customLIBPATH.append(r'U:\fftw-3.0.1\.libs')
    ################################################################
    # If you want to build the OSC opcodes,
    # you need liblo.
    customCPPPATH.append(r'D:\utah\home\mkg\projects\liblo\lo')
    customCPPPATH.append(r'D:\utah\home\mkg\projects\liblo\src')
    customLIBPATH.append(r'D:\utah\home\mkg\projects\liblo\src\.libs')
    # For OSC on Windows, you will also need a Windows pthread library.
    customCPPPATH.append(r'U:\pthreads\Pre-built.2\include')
    customLIBPATH.append(r'U:\pthreads\Pre-build.2\lib')
    ################################################################
    # print "Adding custom path"
else:
    platform = 'unsupported platform'

