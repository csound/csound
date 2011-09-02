'''
S C O N S   C U S T O M I Z A T I O N   F I L E
F O R   M i n G W / M S y s 

Michael Gogins

In general, the purpose of a customization file for SCons
is to specify build options for third-party dependencies 
that are not installed in 'standard locations'
(e.g. /usr or /usr/local or /opt or /pkg on Linux).

Order is important: place local paths ahead of system paths.

The purpose of this particular customization file is enable the use of 
MinGW in the MSys shell to build Csound 
for the Windows installers.

Usage (for example):

scons custom=custom-mingw.py useDouble=1 etc...

This file assumes that all of Csound's third-party dependencies have
been installed, or built, in "C:\utah\opt\". 
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
    ################################################################
    # If you want to build Csound,
    # libsndfile is REQUIRED.
    ################################################################
    # If you want to build Csound,
    # libsndfile is REQUIRED.
    customCPPPATH.append(r'C:\utah\opt\Mega-Nerd\libsndfile\include')
    customLIBPATH.append(r'C:\utah\opt\Mega-Nerd\libsndfile')
    ################################################################
    # If you want to build PortAudio,
    # for real-time audio performance.
    customCPPPATH.append(r'C:\utah\opt\portaudio\include')
    customLIBPATH.append(r'C:\utah\opt\portaudio')
    #################################################################
    # If you want to build PortMidi,
    # for real-time MIDI performance.,
    # you need both portmidi and porttime.
    customCPPPATH.append(r'C:\utah\opt\portmidi\pm_common')
    customCPPPATH.append(r'C:\utah\opt\portmidi\pm_win')
    customCPPPATH.append(r'C:\utah\opt\portmidi\porttime')
    customLIBPATH.append(r'C:\utah\opt\portmidi')
    ################################################################
    # If you want to build the FLTK widgets, csound5gui, cseditor,
    # CsoundAC, or CsoundVST, FLTK 1.1x is required.
    customCPPPATH.append(r'C:\utah\opt\fltk-1.1.10')
    customLIBPATH.append(r'C:\utah\opt\fltk-1.1.10\src')
    customLIBPATH.append(r'C:\utah\opt\fltk-1.1.10\lib')
    customLIBPATH.append(r'C:\utah\opt\fltk-1.1.10\bin')
    ################################################################
    # If you want to build the image opcodes,
    # libpng and libz are required.
    customCPPPATH.append(r'C:\utah\msys\1.0\local\include')
    customLIBPATH.append(r'C:\utah\msys\1.0\local\lib')
    customLIBPATH.append(r'C:\utah\msys\1.0\local\bin')
    ################################################################
    # If you want to build the Python opcodes, the Python interfaces
    # to Csound, or CsoundAC, Python 2.5 is required.
    customCPPPATH.append(r'C:\utah\opt\Python27\include')
    customLIBPATH.append(r'C:\utah\opt\Python27\libs')
    ################################################################
    # If you want to build CsoundVST, you need the 
    # Steinberg VST SDK version 2.4.
    customCPPPATH.append(r'C:\utah\opt\Steinberg\vstsdk2.4')    
    customCPPPATH.append(r'C:\utah\opt\Steinberg\vstsdk2.4\public.sdk\source\vst2.x')    
    customCPPPATH.append(r'C:\utah\opt\Steinberg\vstsdk2.4\pluginterfaces\vst2.x')    
    ################################################################
    # If you want to build CsoundAC, you need the 
    # boost C++ libraries.
    customCPPPATH.append(r'C:\utah\opt\boost_1_47_0')
    ################################################################
    # If you want to build scoregen, you need the include
    # path to VST MIDI plugin SDK header files. 
    customCPPPATH.append(r'C:\utah\opt\VSTModuleArchitectureSDK\pluginterfaces\base')    
    ################################################################
    # If you want to build Lua interfaces, you need Lua.
    customCPPPATH.append(r'C:\utah\opt\luajit-2.0\src')
    customLIBPATH.append(r'C:\utah\opt\luajit-2.0\src')
    ################################################################
    # If you want to build Tcl\Tk interfaces
    # or Tclcsound, you need Tcl\Tk.
    # Add it here:
    customCPPPATH.append(r'C:\utah\opt\Tcl\include')
    customLIBPATH.append(r'C:\utah\opt\Tcl\lib')
    customLIBPATH.append(r'C:\utah\opt\Tcl\bin')
    ################################################################
    # If you want to build the FluidSynth opcodes,
    # you need the FluidSynth DLL (not .lib).
    customLIBPATH.append(r'C:\utah\opt\fluidsynth\src')
    customCPPPATH.append(r'C:\utah\opt\fluidsynth\include')
    # And the dsound library from the Microsoft DirectX SDK.
    customLIBPATH.append(r'C:\utah\opt\directx-sdk-2010-06\Lib\x86')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. 
    customCPPPATH.append(r'C:\utah\opt\Java\jdk1.6.0_21\include')
    customCPPPATH.append(r'C:\utah\opt\Java\jdk1.6.0_21\include\win32')
    ################################################################
    # If you want to build the OSC opcodes,
    # you need liblo.
    customCPPPATH.append(r'C:\utah\opt\liblo')
    customCPPPATH.append(r'C:\utah\opt\liblo\lo')
    customCPPPATH.append(r'C:\utah\opt\liblo\src')
    customLIBPATH.append(r'C:\utah\opt\liblo')
    ################################################################
    # If you want to build the Pure Data external csoundapi~,
    # you need Pure Data. 
    # Add it here (you do NOT need to build it first):
    customCPPPATH.append(r'C:\utah\opt\pure-data\pd\src')
    # But this should installed from the pre-built version:
    customLIBPATH.append(r'C:\utah\opt\pd\bin')
    ################################################################
    # If you want to build the linear algebra opcodes, 
    # you need Gmm++ (headers only).
    customCPPPATH.append(r'C:\utah\opt\gmm-4.1\include')
    ################################################################
    # If you want to build the Loris opcodes,
    # then copy Loris to csound5\Opcodes\Loris. Loris also
    # requires FFTW, if it is not in a standard location,
    # add it here (you do need to build it first):
    #customCPPPATH.append(r'C:\utah\opt\fftw-3.0.1\api')
    #customLIBPATH.append(r'C:\utah\opt\fftw-3.0.1\.libs')
    ################################################################
    # If you want to build MusicXML import and export, 
    # you need the MusicXML library.
        # Add it here (the CodeBlocks build seems to work).
    customCPPPATH.append(r'C:\utah\opt\musicxml-v2\src\elements') 
    customCPPPATH.append(r'C:\utah\opt\musicxml-v2\src\files') 
    customCPPPATH.append(r'C:\utah\opt\musicxml-v2\src\visitors') 
    customCPPPATH.append(r'C:\utah\opt\musicxml-v2\src\lib') 
    customCPPPATH.append(r'C:\utah\opt\musicxml-v2\src\parser') 
    customLIBPATH.append(r'C:\utah\opt\musicxml-v2\win32\codeblocks')
    # print "Adding custom path."
else:
    platform = 'unsupported platform'

