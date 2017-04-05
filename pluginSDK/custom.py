'''
Modify this file, by platform, to handle nonstandard options for third-party
dependencies. If you do modify this file, you should make it read-only
(or otherwise protect it) so that CVS will not overwrite it.

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

if sys.platform[:5] == 'linux':
    platform = 'linux'
    customCPPPATH.append('/usr/local/include/csound')
elif sys.platform == 'darwin':
    platform = 'darwin'
    customCPPPATH.append('/Library/Frameworks/CsoundLib.framework/Headers')
    customCXXFLAGS.append('-fabi-version=0')
elif sys.platform[:3] == 'win':
    # For the basic build you need MinGW, MSys, and libsndfile.
    # Add them here:
    customCPPPATH.append('c:/msys/1.0/local/include')
    customLIBPATH.append('c:/msys/1.0/local/lib')
    # If libsndfile is not in a standard location add it here:
    customCPPPATH.append('D:/utah/opt/libsndfile-1_0_17')
    customLIBPATH.append('D:/utah/opt/libsndfile-1_0_17')
    ################################################################
    # If you want real-time audio you need PortAudio.
    # If it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append('D:/utah/opt/portaudio_varga/include')
    customLIBPATH.append('D:/utah/opt/lazzarini')
    ################################################################
    # If you want PortMidi for real-time midi,
    # if it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append('D:/utah/opt/portmidi/pm_common')
    customCPPPATH.append('D:/utah/opt/portmidi/pm_win')
    customCPPPATH.append('D:/utah/opt/portmidi/porttime')
    customLIBPATH.append('D:/utah/opt/portmidi/pm_win')
    customLIBPATH.append('D:/utah/opt/portmidi/porttime')
    customLIBPATH.append('D:/utah/opt/portmidi')
    ################################################################
    # If you want FLTK widgets or if you want to build CsoundVST,
    # you need FLTK. If it is not in a standard location,
    # add it here (of course you must build it first):
    customCPPPATH.append('D:/utah/opt/fltk-1.1.7')
    customLIBPATH.append('D:/utah/opt/csound6/bin')
    #customLIBPATH.append('D:/utah/opt/fltk-1.1.7/lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customCPPPATH.append('D:/utah/opt/Python24/include')
    customLIBPATH.append('D:/utah/msys/1.0/local/lib')
    customCPPPATH.append('C:/Python24/include')
    customLIBPATH.append('C:/Python24/Libs')
    ################################################################
    # If you want to build Lua interfaces you need Lua.
    # Add it here:
    customCPPPATH.append('D:/utah/opt/lua-5.1.1/src')
    customLIBPATH.append('D:/utah/opt/lua-5.1.1/src')
    ################################################################
    # If you want to build Tcl/Tk interfaces
    # and Tclcsound you need Tcl/Tk.
    # Add it here:
    customCPPPATH.append('D:/utah/opt/Tcl/include')
    customLIBPATH.append('D:/utah/opt/Tcl/bin')
    ################################################################
    # If you want to build CsoundVST you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append('D:/utah/opt/boost/')
    ################################################################
    # If you want to build the FluidSynth opcodes
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    #customLIBPATH.append('D:/utah/opt/fluidsynth-1.0.3-win32')
    #customCPPPATH.append('D:/utah/opt/fluidsynth-1.0.3-win32/include')
    customLIBPATH.append('D:/utah/home/mkg/projects/fluid/fluidsynth/src')
    customCPPPATH.append('D:/utah/home/mkg/projects/fluid/fluidsynth/include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('D:/utah/opt/jdk1.5.0/include')
    customCPPPATH.append('D:/utah/opt/jdk1.5.0/include/win32')
    ################################################################
    # If you want to build the PD external csoundapi~
    # you need PD. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('D:/utah/opt/pd-0.38-4-devel-2/src')
    customLIBPATH.append('D:/utah/opt/pd-0.38-4-devel-2/bin')
    ################################################################
    # If you want to build the Loris opcodes,
    # then copy Loris to csound6/Opcodes/Loris. Loris also
    # requires FFTW, if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append('D:/utah/opt/fftw-3.0.1/api')
    customLIBPATH.append('D:/utah/opt/fftw-3.0.1/.libs')
    ################################################################
    # If you want to build the OSC opcodes,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    #customCPPPATH.append('D:/utah/home/mkg/projects/liblo')
    #customLIBPATH.append('D:/utah/home/mkg/projects/liblo/src/.libs')
    # For OSC on Windows you will also need a Windows pthread library,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    #customCPPPATH.append('D:/utah/opt/pthreads')
    #customLIBPATH.append('D:/utah/opt/pthreads')
    ################################################################
    # print "Adding custom path"
else:
    platform = 'unsupported platform'

