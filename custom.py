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
    # Example:
    # customCPPPATH.append('c:/tools/Python23/include')
    # customLIBPATH.append('cygwin_import_libs')
    customCPPPATH.append('c:/tools/Python23/include')
    customLIBPATH.append('cygwin_import_libs')
elif sys.platform[:3] == 'win':
    customCPPPATH.append('c:/tools/mingw/include')
    customCPPPATH.append('c:/tools/boost')
    customCPPPATH.append('c:/tools/Python23/include')
    customCPPPATH.append('c:/tools/msys/1.0/local/include')
    customCPPPATH.append('c:/tools/fluidsynth/include')
    customLIBPATH.append('cygwin_import_libs')
    customLIBPATH.append('c:/tools/mingw/lib')
    customLIBPATH.append('c:/tools/msys/1.0/local/lib')
    customLIBPATH.append('c:/tools/fluidsynth')
    platform = 'windows'
else:
    platform = 'unsupported platform'


