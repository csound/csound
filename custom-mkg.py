'''
This file is to customize Csound SConstruct for using 
Microsoft Visual Studio 2005, free edition, to build Csound
for the Windows installers.

Order is important: place local paths ahead of system paths.
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
    # If libsndfile is not in a standard location add it here:
    customCPPPATH.append(r'D:\utah\opt\libsndfile-1_0_17')
    customLIBPATH.append(r'D:\utah\opt\libsndfile-1_0_17')
    ################################################################
    # If you want real-time audio you need PortAudio.
    # If it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append(r'D:\utah\home\mkg\projects\portaudio\include')
    customLIBPATH.append(r'D:\utah\home\mkg\projects\portaudio\build\msvc\Win32\Release')
    ################################################################
    # If you want PortMidi for real-time midi,
    # if it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append(r'D:\utah\opt\portmidi\pm_common')
    customCPPPATH.append(r'D:\utah\opt\portmidi\pm_win')
    customCPPPATH.append(r'D:\utah\opt\portmidi\porttime')
    customLIBPATH.append(r'D:\utah\opt\portmidi\pm_win')
    customLIBPATH.append(r'D:\utah\opt\portmidi\porttime')
    customLIBPATH.append(r'D:\utah\opt\portmidi')
    ################################################################
    # If you want FLTK widgets or if you want to build CsoundVST,
    # you need FLTK. If it is not in a standard location,
    # add it here (of course you must build it first):
    customCPPPATH.append(r'D:\utah\msys\1.0\local\include')
    customLIBPATH.append(r'D:\utah\msys\1.0\local\lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customLIBPATH.append(r'D:\utah\msys\1.0\local\lib')
    customCPPPATH.append(r'D:\utah\opt\Python25\include')
    customLIBPATH.append(r'D:\utah\opt\Python25\Libs')
    ################################################################
    # If you want to build vst4cs or CsoundVST you need the include
    # path to the VST SDK header files. Add them there:
    customCPPPATH.append(r'D:\utah\home\mkg\projects\csoundd\frontends\CsoundVST\vstsdk2.4')    
    ################################################################
    # If you want to build CsoundVST or CsoundAC you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append(r'D:\utah\opt\boost')
    ################################################################
    # If you want to build scoregen you need the include
    # path you need the VST MIDI plugin SDK header files. Copy
    # the whole VST MIDI plugin SDK into the CsoundVST directory:
    customCPPPATH.append(r'D:\dutah\home\mkg\projects\csoundd\frontends\CsoundVST\VSTModuleArchitectureSDK\pluginterfaces\base')    
    ################################################################
    # If you want to build Lua interfaces you need Lua.
    # Add it here:
    customCPPPATH.append(r'D:\utah\opt\lua-5.1.1\src')
    customLIBPATH.append(r'D:\utah\opt\lua-5.1.1\src')
    ################################################################
    # If you want to build Tcl\Tk interfaces
    # and Tclcsound you need Tcl\Tk.
    # Add it here:
    customCPPPATH.append(r'D:\utah\opt\Tcl\include')
    customLIBPATH.append(r'D:\utah\opt\Tcl\bin')
    ################################################################
    # If you want to build the FluidSynth opcodes
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    #customLIBPATH.append(r'D:\utah\opt\fluidsynth-1.0.3-win32')
    #customCPPPATH.append(r'D:\utah\opt\fluidsynth-1.0.3-win32\include')
    customLIBPATH.append(r'D:\utah\home\mkg\projects\fluidsynth\winbuild')
    customCPPPATH.append(r'D:\utah\home\mkg\projects\fluidsynth\include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append(r'D:\utah\opt\jdk1.5.0\include')
    customCPPPATH.append(r'D:\utah\opt\jdk1.5.0\include\win32')
    ################################################################
    # If you want to build the PD external csoundapi~
    # you need PD. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append(r'D:\utah\opt\pd-0.38-4-devel-2\src')
    customLIBPATH.append(r'D:\utah\opt\pd-0.38-4-devel-2\bin')
    ################################################################
    # If you want to build the Loris opcodes,
    # then copy Loris to csound5\Opcodes\Loris. Loris also
    # requires FFTW, if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append(r'D:\utah\opt\fftw-3.0.1\api')
    customLIBPATH.append(r'D:\utah\opt\fftw-3.0.1\.libs')
    ################################################################
    # If you want to build the OSC opcodes,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append(r'D:\utah\home\mkg\projects\liblo\lo')
    customCPPPATH.append(r'D:\utah\home\mkg\projects\liblo\src')
    customLIBPATH.append(r'D:\utah\home\mkg\projects\liblo\src\.libs')
    # For OSC on Windows you will also need a Windows pthread library,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append(r'D:\utah\opt\pthreads\Pre-built.2\include')
    customLIBPATH.append(r'D:\utah\opt\pthreads\Pre-build.2\lib')
    ################################################################
    # print "Adding custom path"
else:
    platform = 'unsupported platform'

