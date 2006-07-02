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
elif sys.platform == 'darwin':
    platform = 'darwin'
    customCPPPATH.append('/usr/include/malloc')
    customCPPPATH.append('/opt/local/include/boost-1_32/')
    customCPPPATH.append('/usr/include/python2.3')
    customCXXFLAGS.append('-fabi-version=0')
elif sys.platform[:3] == 'win':
    # For the basic build you need MinGW, MSys, and libsndfile.
    # Add them here:
    customCPPPATH.append('c:/msys/1.0/mingw/include')
    customLIBPATH.append('c:/msys/1.0/mingw/lib')
    # If libsndfile is not in a standard location add it here:
    #customCPPPATH.append('C:/opt/libsndfile-1.0.15/src')
    #customLIBPATH.append('C:/opt/libsndfile-1.0.15/src/.libs')
    ################################################################
    # If you want real-time audio you need PortAudio.
    # If it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append('C:/home/mkg/projects/portaudio/pa_common')
    customLIBPATH.append('C:/home/mkg/projects/portaudio/lib')
    ################################################################
    # If you want PortMidi for real-time midi,
    # if it is not in a standard location add it here
    # (of course you must build it first):
    customCPPPATH.append('C:/opt/portmidi/pm_common')
    customCPPPATH.append('C:/opt/portmidi/pm_win')
    customCPPPATH.append('C:/opt/portmidi/porttime')
    customLIBPATH.append('C:/opt/portmidi/pm_win')
    customLIBPATH.append('C:/opt/portmidi/porttime')
    customLIBPATH.append('C:/opt/portmidi')
    ################################################################
    # If you want FLTK widgets or if you want to build CsoundVST,
    # you need FLTK. If it is not in a standard location,
    # add it here (of course you must build it first):
    #customCPPPATH.append('C:/opt/fltk-1.1.7')
    #customLIBPATH.append('C:/opt/fltk-1.1.7/lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customCPPPATH.append('c:/opt/Python24/include')
    #customLIBPATH.append('c:/WINDOWS/system32')
    ################################################################
    # If you want to build Lua interfaces you need Lua.
    # Add it here:
    customCPPPATH.append('c:/opt/lua-5.1/src')
    customLIBPATH.append('C:/opt/lua-5.1/src')
    ################################################################
    # If you want to build Tcl/Tk interfaces
    # and Tclcsound you need Tcl/Tk.
    # Add it here:
    customCPPPATH.append('c:/opt/Tcl/include')
    customLIBPATH.append('c:/opt/Tcl/bin')
    ################################################################
    # If you want to build CsoundVST you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append('c:/opt/boost/')
    ################################################################
    # If you want to build the FluidSynth opcodes
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    #customLIBPATH.append('c:/opt/fluidsynth-1.0.3-win32')
    #customCPPPATH.append('c:/opt/fluidsynth-1.0.3-win32/include')
    customLIBPATH.append('C:/home/mkg/projects/fluid/fluidsynth/src')
    customCPPPATH.append('C:/home/mkg/projects/fluid/fluidsynth/include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/opt/jdk1.5.0/include')
    customCPPPATH.append('c:/opt/jdk1.5.0/include/win32')
    ################################################################
    # If you want to build the PD external csoundapi~
    # you need PD. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/opt/pd-0.38-4-devel-2/src')
    customLIBPATH.append('c:/opt/pd-0.38-4-devel-2/bin')
    platform = 'mingw'
    ################################################################
    # If you want to build the Loris opcodes,
    # then copy Loris to csound5/Opcodes/Loris. Loris also
    # requires FFTW, if it is not in a standard location,
    # add it here (you do need to build it first):
    customCPPPATH.append('c:/opt/fftw-3.0.1/api')
    customLIBPATH.append('c:/opt/fftw-3.0.1/.libs')
    ################################################################
    # If you want to build the OSC opcodes,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    #customCPPPATH.append('c:/home/mkg/projects/liblo')
    #customLIBPATH.append('c:/home/mkg/projects/liblo/src/.libs')
    # For OSC on Windows you will also need a Windows pthread library,
    # if it is not in a standard location,
    # add it here (you do need to build it first):
    #customCPPPATH.append('c:/opt/pthreads')
    #customLIBPATH.append('c:/opt/pthreads')
    ################################################################
    # print "Adding custom path"
    # customLIBPATH.append("D:/Program Files/Microsoft Visual Studio/vc98/Lib")
    # customLIBPATH.append('msvc')
    # customCPPPATH.append("D:/Program Files/Microsoft Visual Studio/vc98/include")
    # customCPPPATH.append("D:/Program Files/java/jdk1.5.0_05/include")
    # customCPPPATH.append("D:/Program Files/java/jdk1.5.0_05/include/win32")
    # customLIBPATH.append("D:/Program Files/java/jdk1.5.0_05/lib")
    # customCPPPATH.append("msvc/pa_common")
    # customCPPPATH.append("msvc/tcl")
    # customCPPPATH.append("D:/Python23/include")
    # customCPPPATH.append("msvc/tk")
    # customCPPPATH.append("msvc")
    ### If libsndfile is not in a standard location add it here:
    # customCPPPATH.append("./msvc/libsndfile")
    # print customCPPPATH
    # customCCFLAGS.append("-DMSVC")
    # customCCFLAGS.append("-GX")
    # customCCFLAGS.append("-MD")
    # customCCFLAGS.append("-W2")
    # customCCFLAGS.append("-Ob2")
else:
    platform = 'unsupported platform'

