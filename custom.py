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
elif sys.platform == 'cygwin':
    platform = 'cygwin'
    customCPPPATH.append('c:/tools/Python23/include')
    customLIBPATH.append('cygwin_import_libs')
elif sys.platform[:3] == 'win':
    # For the basic build you need MinGW, MSys, and libsndfile.
    # Add them here:
    customLIBPATH.append('c:/tools/mingw/lib')
    customCPPPATH.append('c:/tools/msys/1.0/local/include')
    customLIBPATH.append('c:/tools/msys/1.0/local/lib')
    # If libsndfile is not in a standard location add it here:
    # customLIBPATH.append('C:/tools/libsndfile-1.0.10')
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
    # customCPPPATH.append('C:/tools/fltk-1.1.5rc1')
    # customLIBPATH.append('C:/tools/fltk-1.1.5rc1/lib')
    ################################################################
    # If you want to build CsoundVST you need Python and
    # a MinGW import library for Python. Add them here:
    customCPPPATH.append('c:/apcc/tools/Python23/include')
    customCPPPATH.append('c:/apcc/tools/mingw/include')
    customLIBPATH.append('c:/projects/csound5/cygwin_import_libs')    
    customSWIGFLAGS.append('-Ic:/apcc/tools/mingw/lib/gcc-lib/mingw32/3.3.1/include')
    ################################################################
    # If you want to build CsoundVST you need boost.
    # If it is not in a standard lcoation add it here
    # (you do NOT need to build it first):
    customCPPPATH.append('c:/tools/boost')
    ################################################################
    # If you want to build the FluidSynth opcodes 
    # you need FluidSynth. If it is not a standard location,
    # add it here (you do NOT need to build it first):
    customLIBPATH.append('c:/tools/fluidsynth')
    customCPPPATH.append('c:/tools/fluidsynth/include')
    ################################################################
    platform = 'mingw'
else:
    platform = 'unsupported platform'


