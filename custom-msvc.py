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

# MSVC build
customLIBPATH.append("C:/Program Files/Microsoft Visual Studio/vc98/Lib")
customLIBPATH.append("C:/Python23/Libs")
customLIBPATH.append('msvc')
customCPPPATH.append("C:/Program Files/Microsoft Visual Studio/vc98/include")
customCPPPATH.append("C:/Program Files/java/jdk1.5.0_05/include")
customCPPPATH.append("C:/Program Files/java/jdk1.5.0_05/include/win32")
customLIBPATH.append("C:/Program Files/java/jdk1.5.0_05/lib")
customCPPPATH.append("msvc/pa_common")
customCPPPATH.append("msvc/tcl")
customCPPPATH.append("C:/Python23/include")
customCPPPATH.append("msvc/tk")
customCPPPATH.append("msvc")
### If libsndfile is not in a standard location add it here:
#   customCPPPATH.append("./msvc/libsndfile")
#   print customCPPPATH
customCCFLAGS.append("-DMSVC")
customCCFLAGS.append("-GX")
customCCFLAGS.append("-MD")
customCCFLAGS.append("-W2")
customCCFLAGS.append("-Ob2")
