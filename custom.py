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
elif sys.platform == 'mingw':
    platform = 'mingw'
    customCPPPATH.append('c:/tools/Python23/include')
    customLIBPATH.append('cygwin_import_libs')
elif sys.platform[:3] == 'win':
    platform = 'windows'
    customCPPPATH.append('c:/tools/Python23/include')
    customLIBPATH.append('cygwin_import_libs')
else:
    platform = 'unsupported platform'

