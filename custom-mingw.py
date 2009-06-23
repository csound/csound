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
	################################################################
	# If you want to build Csound,
	# libsndfile is REQUIRED.
 	################################################################
	# If you want to build Csound,
	# libsndfile is REQUIRED.
	customCPPPATH.append(r'U:\Mega-Nerd\libsndfile\include')
	customLIBPATH.append(r'U:\Mega-Nerd\libsndfile')
	################################################################
	# If you want to build PortAudio,
	# for real-time audio performance.
	customCPPPATH.append(r'U:\portaudio\include')
	customLIBPATH.append(r'U:\portaudio')
	#################################################################
	# If you want to build PortMidi,
	# for real-time MIDI performance.,
	# you need both portmidi and porttime.
	customCPPPATH.append(r'U:\portmidi\pm_common')
	customCPPPATH.append(r'U:\portmidi\pm_win')
	customCPPPATH.append(r'U:\portmidi\porttime')
	customLIBPATH.append(r'U:\portmidi')
	################################################################
	# If you want to build the FLTK widgets, csound5gui, cseditor,
	# CsoundAC, or CsoundVST, FLTK 1.1x is required.
	customCPPPATH.append(r'U:\fltk-mingw')
        # Import libraries MUST come first.
        customLIBPATH.append(r'U:\fltk-mingw\src')
        # Then static libraries.
	customLIBPATH.append(r'U:\fltk-mingw\lib')
	################################################################
	# If you want to build the image opcodes,
	# libpng and libz are required.
	customCPPPATH.append(r'U:\zlib-1.2.3.win32\include')
	customLIBPATH.append(r'U:\zlib-1.2.3.win32\lib')
	customLIBPATH.append(r'U:\zlib-1.2.3.win32\bin')
	customCPPPATH.append(r'U:\libpng-1.2.24')
	customLIBPATH.append(r'U:\libpng-1.2.24\.libs')
	################################################################
	# If you want to build the Python opcodes, the Python interfaces
	# to Csound, or CsoundAC, Python 2.5 is required.
	customCPPPATH.append(r'U:\Python26\include')
	customLIBPATH.append(r'U:\Python26\libs')
	################################################################
	# If you want to build CsoundVST, you need the 
	# Steinberg VST SDK version 2.4.
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
	customCPPPATH.append(r'U:\Lua5.1')
	customLIBPATH.append(r'U:\Lua5.1')
	################################################################
	# If you want to build Tcl\Tk interfaces
	# or Tclcsound, you need Tcl\Tk.
	# Add it here:
	customCPPPATH.append(r'U:\Tcl\include')
	customLIBPATH.append(r'U:\Tcl\lib')
	################################################################
	# If you want to build the FluidSynth opcodes,
	# you need the FluidSynth DLL (not .lib).
	customLIBPATH.append(r'U:\fluidsynth-1.0.9\src\.libs')
	customCPPPATH.append(r'U:\fluidsynth-1.0.9\include')
	# And the dsound library from the Microsoft DirectX SDK.
	customLIBPATH.append(r'U:\dxsdk\Lib\x86')
	################################################################
	# If you want to build the Java wrapper for CsoundVST
	# you need Java. 
	customCPPPATH.append(r'U:\Java\jdk1.6.0_14\include')
	customCPPPATH.append(r'U:\Java\jdk1.6.0_14\include\win32')
	################################################################
	# If you want to build the OSC opcodes,
	# you need liblo.
	customCPPPATH.append(r'U:\liblo-0.26')
	customCPPPATH.append(r'U:\liblo-0.26\lo')
	customCPPPATH.append(r'U:\liblo-0.26\src')
	customLIBPATH.append(r'U:\liblo-0.26')
	# For OSC on Windows, you will also need a Windows pthread library.
	customCPPPATH.append(r'U:\pthreads\Pre-built.2\include')
	customLIBPATH.append(r'U:\pthreads\Pre-built.2\lib')
	################################################################
	# If you want to build the Pure Data external csoundapi~,
	# you need Pure Data. 
	# Add it here (you do NOT need to build it first):
	customCPPPATH.append(r'U:\pure-data\trunk\pd\src')
	customLIBPATH.append(r'U:\pd\bin')
	################################################################
	# If you want to build the linear algebra opcodes, 
	# you need Gmm++ (headers only).
	customCPPPATH.append(r'U:\gmm-3.1\include')
	################################################################
	# If you want to build the Loris opcodes,
	# then copy Loris to csound5\Opcodes\Loris. Loris also
	# requires FFTW, if it is not in a standard location,
	# add it here (you do need to build it first):
	#customCPPPATH.append(r'U:\fftw-3.0.1\api')
	#customLIBPATH.append(r'U:\fftw-3.0.1\.libs')
	################################################################
	# If you want to build MusicXML import and export, 
	# you need the MusicXML library.
        # Add it here (the CodeBlocks build seems to work).
	customCPPPATH.append(r'U:\musicxml-v2\src\elements') 
	customCPPPATH.append(r'U:\musicxml-v2\src\files') 
	customCPPPATH.append(r'U:\musicxml-v2\src\visitors') 
	customCPPPATH.append(r'U:\musicxml-v2\src\lib') 
	customCPPPATH.append(r'U:\musicxml-v2\src\parser') 
	customLIBPATH.append(r'U:\musicxml-v2\win32\codeblocks')
	# print "Adding custom path."
else:
	platform = 'unsupported platform'

