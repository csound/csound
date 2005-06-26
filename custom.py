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
    customCPPPATH.append('/usr/local/include/python2.3')
    customLIBPATH.append('/usr/local/lib/python2.3/config')
elif sys.platform == 'cygwin':
    platform = 'cygwin'
    customCPPPATH.append('c:/utah/usr/Python23/include')
elif sys.platform[:3] == 'win':
    # For the basic build you need MinGW, MSys, and libsndfile.
    # Add them here:
    customLIBPATH.append('c:/utah/usr/mingw/lib')
    customCPPPATH.append('c:/utah/usr/msys/1.0/local/include')
    customLIBPATH.append('c:/utah/usr/msys/1.0/local/lib')
    # If libsndfile is not in a standard location add it here:
    #customCPPPATH.append('C:/utah/usr/libsndfile-1.0.11/src')
    #customLIBPATH.append('C:/utah/usr/libsndfile-1.0.11')
    ################################################################
    # If you want real-time audio you need PortAudio.
    # If it is not in a standard location add it here
    # (of course you must build it first):
    # customCPPPATH.append('C:/projects/portaudio/pa_common')
    # customLIBPATH.append('C:/projects/portaudio/lib')
    ################################################################
    # If you want FLTK widgets or if you want to build CsoundVST,
    # you need FLTK. If it is not in a standard location,
    # add it here (of course you must build it first):
    # customCPPPATH.append('C:/utah/opt/fltk-1.1.6')
    # customLIBPATH.append('C:/utah/opt/fltk-1.1.6/lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customCPPPATH.append('c:/utah/opt/Python23/include')
    customCPPPATH.append('c:/utah/usr/mingw/include')
    customLIBPATH.append('c:/projects/csound5/cygwin_import_libs')
    customSWIGFLAGS.append('-Derrmsg=err_msg')
    ################################################################
    # If you want to build CsoundVST you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/boost')
    ################################################################
    # If you want to build the FluidSynth opcodes
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    #customLIBPATH.append('c:/utah/opt/fluidsynth-1.0.3-win32')
    #customCPPPATH.append('c:/utah/opt/fluidsynth-1.0.3-win32/include')
    ################################################################
    # If you want to build the Java wrapper for CsoundVST
    # you need Java. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/jdk1.5.0/include')
    customCPPPATH.append('c:/utah/opt/jdk1.5.0/include/win32')
    ################################################################
    # If you want to build the PD external csoundapi~
    # you need PD. If it is not in a standard location,
    # add it here (you do NOT need to build it first):
    customCPPPATH.append('c:/utah/opt/pd-0.38-4-devel-2/src')
    customLIBPATH.append('c:/utah/opt/pd-0.38-4-devel-2/bin')
    platform = 'mingw'
else:
    platform = 'unsupported platform'

