'''
Modify this file, by platform, to handle nonstandard options for third-party 
dependencies. If you do modify this file, you should make it read-only 
(or otherwise protect it) so that CVS will not overwrite it.
'''
import sys

customCPPPATH = []
customCCFLAGS = []
customCXXFLAGS = []
customLIBS = []
customLIBPATH = []
customSHLINKFLAGS = []

if sys.platform[:5] == 'linux':
    platform = 'linux'
elif sys.platform == 'cygwin':
    platform = 'cygwin'
    customCPPPATH.append('c:/tools/Python23/include')
    customLIBPATH.append('cygwin_import_libs')
elif sys.platform[:3] == 'win':
    #for the basic build use: (modify paths to your system)
    #the location of the mingw compiler:
    customCPPPATH.append('c:/tools/Python23/include')
    customCPPPATH.append('c:/tools/msys/1.0/local/include')
    #customCPPPATH.append('C:/tools/libsndfile-1.0.10pre8/src')
    # Order is important here. Keep local paths in front of system paths.
    customLIBPATH.append('c:/projects/csound5/windows_dlls')
    customLIBPATH.append('c:/projects/csound5/cygwin_import_libs')
    customLIBPATH.append('c:/tools/mingw/lib')
    customLIBPATH.append('c:/tools/msys/1.0/local/lib')
    #inside the windows_dlls directory should be libsndfile.dll
    #if you don't have it or have built it yourself use: (and change accordingly)
    #customLIBPATH.append('C:/tools/libsndfile-1.0.10pre8')
    ################################################################
    #if you want to build with PORTAUDIO include: (you must to build portaudio first)
    customCPPPATH.append('C:/projects/portaudio/pa_common')
    customLIBPATH.append('C:/projects/portaudio/lib')
    ################################################################
    #if you want to build with FLTK include: (you must to build portaudio first)
    customCPPPATH.append('C:/tools/fltk-1.1.5rc1')
    customLIBPATH.append('C:/tools/fltk-1.1.5rc1/lib')
    ################################################################
    #if you want to build CsoundVST include:
    customCPPPATH.append('c:/tools/boost')
    ################################################################
    #if you want to build FLUIDSYNTH include:
    customLIBPATH.append('c:/tools/fluidsynth')
    customCPPPATH.append('c:/tools/fluidsynth/include')
    ################################################################

    #customLIBPATH.append('cygwin_import_libs')
    platform = 'mingw'
else:
    platform = 'unsupported platform'


