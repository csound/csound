
# vim:syntax=python

print '''
C S O U N D 5

SCons build file for Csound 5:
API library, plugin opcodes, utilities, and front ends.

By Michael Gogins <gogins at pipeline dot com>

For custom options, run 'scons -h'.
For default options, run 'scons -H'.
If headers or libraries are not found, edit 'custom.py'.
For Linux, run in the standard shell
    with standard Python and just run 'scons'.
For MinGW, run in the MSys shell
    and use www.python.org WIN32 Python to run scons.
'''

import time
import glob
import os
import os.path
import sys
import string
import zipfile
import shutil
import copy

#############################################################################
#
#   UTILITY FUNCTIONS
#############################################################################

zipDependencies = []
pluginLibraries = []
executables = []
headers = Split('''
    H/cfgvar.h H/cscore.h H/csdl.h H/csound.h H/csound.hpp H/csoundCore.h
    H/cwindow.h H/msg_attr.h H/OpcodeBase.hpp H/pstream.h H/pvfileio.h
    H/soundio.h H/sysdep.h H/text.h H/version.h
    interfaces/CsoundFile.hpp interfaces/CppSound.hpp interfaces/filebuilding.h
''')
libs = []
pythonModules = []

def today():
    return time.strftime("%Y-%m-%d", time.localtime())

# Detect OPERATING SYSTEM platform.

def getPlatform():
    if sys.platform[:5] == 'linux':
        return 'linux'
    elif sys.platform[:3] == 'win':
        return 'win32'
    elif sys.platform[:6] == 'darwin':
        return 'darwin'
    else:
        return 'unsupported'

#############################################################################
#
#   DEFINE CONFIGURATION
#
#############################################################################

print "System platform is '" + getPlatform() + "'."

# Create options that can be set from the command line.

commandOptions = Options()
commandOptions.Add('CC')
commandOptions.Add('CXX')
commandOptions.Add('LINK')
commandOptions.Add('LINKFLAGS')
commandOptions.Add('useDouble',
    'Set to 1 to use double-precision floating point for audio samples.',
    '0')
commandOptions.Add('usePortAudio',
    'Set to 1 to use PortAudio for real-time audio input and output.',
    '1')
commandOptions.Add('usePortMIDI',
    'Build PortMidi plugin for real time MIDI input and output.',
    '1')
commandOptions.Add('useALSA',
    'Set to 1 to use ALSA for real-time audio and MIDI input and output.',
    '1')
commandOptions.Add('useJack',
    'Set to 1 if you compiled PortAudio to use Jack; also builds Jack plugin.',
    '1')
commandOptions.Add('useFLTK',
    'Set to 1 to use FLTK for graphs and widget opcodes.',
    '1')
commandOptions.Add('noFLTKThreads',
    'Set to 1 to disable use of a separate thread for FLTK widgets.',
    '1')
commandOptions.Add('pythonVersion',
    'Set to the Python version to be used.',
    '%d.%d' % (int(sys.hexversion) >> 24, (int(sys.hexversion) >> 16) & 255))
commandOptions.Add('buildCsoundVST',
    'Set to 1 to build CsoundVST (needs FLTK, boost, Python, SWIG).',
    '0')
commandOptions.Add('buildCsound5GUI',
    'Build FLTK GUI frontend (requires FLTK 1.1.7 or later).',
    '0')
commandOptions.Add('generateTags',
    'Set to 1 to generate TAGS',
    '0')
commandOptions.Add('generatePdf',
    'Set to 1 to generate PDF documentation',
    '0')
commandOptions.Add('generateXmg',
    'Set to 1 to generate string database',
    '1')
commandOptions.Add('generateZip',
    'Set to 1 to generate zip archive',
    '0')
commandOptions.Add('buildLoris',
    'Set to 1 to build the Loris Python extension and opcodes',
    '1')
commandOptions.Add('useOSC',
    'Set to 1 if you want OSC support',
    '0')
if getPlatform() != 'win32':
    commandOptions.Add('useUDP',
        'Set to 0 if you do not want UDP support',
        '1')
else:
    commandOptions.Add('useUDP',
        'Set to 1 if you want UDP support',
        '0')
commandOptions.Add('buildPythonOpcodes',
    'Set to 1 to build Python opcodes',
    '0')
commandOptions.Add('prefix',
    'Base directory for installs. Defaults to /usr/local.',
    '/usr/local')
commandOptions.Add('buildRelease',
    'Set to 1 to build for release (implies noDebug).',
    '0')
commandOptions.Add('noDebug',
    'Build without debugging information.',
    '0')
commandOptions.Add('gcc3opt',
    'Enable gcc 3.3.x or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
commandOptions.Add('gcc4opt',
    'Enable gcc 4.0 or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
commandOptions.Add('useLrint',
    'Use lrint() and lrintf() for converting floating point values to integers.',
    '0')
commandOptions.Add('useGprof',
    'Build with profiling information (-pg).',
    '0')
commandOptions.Add('Word64',
    'Build for 64bit computer',
    '0')
if getPlatform() == 'win32':
    commandOptions.Add('dynamicCsoundLibrary',
        'Set to 0 to build static Csound library instead of csound.dll',
        '1')
else:
    commandOptions.Add('dynamicCsoundLibrary',
        'Build dynamic Csound library instead of libcsound.a',
        '0')
commandOptions.Add('buildStkOpcodes',
    "Build opcodes encapsulating Perry Cook's Synthesis Toolkit in C++ instruments and effects",
    '0')
commandOptions.Add('install',
    'Enables the Install targets',
    '1')
commandOptions.Add('buildPDClass',
    "build csoundapi~ PD class (needs m_pd.h in the standard places)",
    '0')
commandOptions.Add('useCoreAudio',
    "Set to 1 to use CoreAudio for real-time audio input and output.",
    '1')
commandOptions.Add('useAltivec',
    "On OSX use the gcc AltiVec optmisation flags",
    '0')
commandOptions.Add('buildDSSI',
    "Build DSSI/LADSPA host opcodes",
    '1')
commandOptions.Add('buildUtilities',
    "Build stand-alone executables for utilities that can also be used with -U",
    '1')
commandOptions.Add('buildTclcsound',
    "Build Tclcsound frontend (cstclsh, cswish and tclcsound dynamic module). Requires Tcl/Tk headers and libs",
    '0')
commandOptions.Add('buildWinsound',
    "Build Winsound frontend. Requires FLTK headers and libs",
    '0')
commandOptions.Add('buildVirtual',
    "Build Virtual MIDI keyboard. Requires FLTK 1.1.7 headers and libs",
    '0')
commandOptions.Add('buildInterfaces',
    "Build interface library for Python, JAVA, Lua, C++, and other languages.",
    '0')
commandOptions.Add('buildJavaWrapper',
    'Set to 1 to build Java wrapper for the interface library.',
    '0')
commandOptions.Add('buildOSXGUI',
    'On OSX, set to 1 to build the basic GUI frontend',
    '0')
commandOptions.Add('buildCSEditor',
    'Set to 1 to build the Csound syntax highlighting text editor. Requires FLTK headers and libs',
    '0')
commandOptions.Add('withMSVC',
    'On Windows, set to 1 to build with Microsoft Visual C++, or set to 0 to build with MinGW',
    '0')
commandOptions.Add('buildNewParser',
	'Enable building new parser (requires Flex/Bison)',
	'0')

# Define the common part of the build environment.
# This section also sets up customized options for third-party libraries, which
# should take priority over default options.

commonEnvironment = Environment(ENV = {'PATH' : os.environ['PATH']})
commandOptions.Update(commonEnvironment)

Help(commandOptions.GenerateHelpText(commonEnvironment))

def withMSVC():
    if getPlatform() == 'win32':
	if commonEnvironment['withMSVC'] == '1':
            return 1
	return 0
    else:
        return 0

def isNT():
    if getPlatform() == 'win32' and os.environ['SYSTEMROOT'].find('WINDOWS') != -1:
        return

# Detect the BUILD platform,
# and update the environment with options from a custom Python script.

if getPlatform() == 'win32':
    if withMSVC():
        optionsFilename = 'custom-msvc.py'
    else:
        optionsFilename = 'custom.py'
else:
    optionsFilename = 'custom.py'
print "Using options from '%s.'" % optionsFilename

fileOptions = Options(optionsFilename)
fileOptions.Add('customCPPPATH', 'List of custom CPPPATH variables')
fileOptions.Add('customCCFLAGS')
fileOptions.Add('customCXXFLAGS')
fileOptions.Add('customLIBS')
fileOptions.Add('customLIBPATH')
fileOptions.Add('customSHLINKFLAGS')
fileOptions.Add('customSWIGFLAGS')
fileOptions.Update(commonEnvironment)

# If the user selects MinGW as the build platform,
# force the use of MinGW tools even if SCons prefers MSVC.

if getPlatform() == 'win32' and not withMSVC():
    Tool('mingw')(commonEnvironment)

customCPPPATH = commonEnvironment['customCPPPATH']
commonEnvironment.Prepend(CPPPATH = customCPPPATH)
customCCFLAGS = commonEnvironment['customCCFLAGS']
commonEnvironment.Prepend(CCFLAGS = customCCFLAGS)
customCXXFLAGS = commonEnvironment['customCXXFLAGS']
commonEnvironment.Prepend(CXXFLAGS = customCXXFLAGS)
customLIBS = commonEnvironment['customLIBS']
commonEnvironment.Prepend(LIBS = customLIBS)
customLIBPATH = commonEnvironment['customLIBPATH']
commonEnvironment.Prepend(LIBPATH = customLIBPATH)
customSHLINKFLAGS = commonEnvironment['customSHLINKFLAGS']
commonEnvironment.Prepend(SHLINKFLAGS = customSHLINKFLAGS)
customSWIGFLAGS = commonEnvironment['customSWIGFLAGS']
commonEnvironment.Prepend(SWIGFLAGS = customSWIGFLAGS)

# Define options for different platforms.
if getPlatform() != 'win32':
 print "Build platform is '" + getPlatform() + "'."
else:
 if not withMSVC():
  print "Build platform is 'mingw/msys'"
 else:
  print "Build platform is 'msvc'"

print "SCons tools on this platform: ", commonEnvironment['TOOLS']

commonEnvironment.Prepend(CPPPATH = ['.', './H'])
if commonEnvironment['useLrint'] != '0':
  commonEnvironment.Prepend(CCFLAGS = ['-DUSE_LRINT'])
if commonEnvironment['gcc3opt'] != '0' or commonEnvironment['gcc4opt'] != '0':
  if commonEnvironment['gcc4opt'] != '0':
    commonEnvironment.Prepend(CCFLAGS = ['-ftree-vectorize'])
    cpuType = commonEnvironment['gcc4opt']
  else:
    cpuType = commonEnvironment['gcc3opt']
  commonEnvironment.Prepend(CCFLAGS = ['-fomit-frame-pointer', '-ffast-math'])
  if getPlatform() == 'darwin':
    flags = '-O3 -mcpu=%s -mtune=%s'%(cpuType, cpuType)
  else:
    flags = '-O3 -march=%s'%(cpuType)
  commonEnvironment.Prepend(CCFLAGS = Split(flags))
elif commonEnvironment['buildRelease'] != '0':
  if not withMSVC():
    commonEnvironment.Prepend(CCFLAGS = Split('''
      -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
    '''))
elif commonEnvironment['noDebug'] == '0':
  if not withMSVC():
    commonEnvironment.Prepend(CCFLAGS = ['-g', '-O2'])
else:
  commonEnvironment.Prepend(CCFLAGS = ['-O2'])
if commonEnvironment['useGprof'] == '1':
  commonEnvironment.Append(CCFLAGS = ['-pg'])
  commonEnvironment.Append(LINKFLAGS = ['-pg'])
if not withMSVC():
  commonEnvironment.Prepend(CXXFLAGS = ['-fexceptions'])
commonEnvironment.Prepend(LIBPATH = ['.', '#.'])
if commonEnvironment['buildRelease'] == '0':
    commonEnvironment.Prepend(CPPFLAGS = ['-DBETA'])
if commonEnvironment['Word64'] == '1':
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib64'])
    commonEnvironment.Append(CCFLAGS = ['-fPIC'])
else:
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib'])

if commonEnvironment['useDouble'] == '0':
    print 'CONFIGURATION DECISION: Using single-precision floating point for audio samples.'
else:
    print 'CONFIGURATION DECISION: Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = ['-DUSE_DOUBLE'])

# Define different build environments for different types of targets.

if not withMSVC():
    commonEnvironment.Prepend(CCFLAGS = "-Wall")

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CPPPATH = '/usr/X11R6/include')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(LINKFLAGS = ['-Wl,-Bdynamic'])
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    if commonEnvironment['useAltivec'] == '1':
        print 'CONFIGURATION DECISION using Altivec optmisation'
        commonEnvironment.Append(CCFLAGS = "-faltivec")
elif getPlatform() == 'win32':
    commonEnvironment.Append(CCFLAGS = "-D_WIN32")
    commonEnvironment.Append(CCFLAGS = "-DWIN32")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(CCFLAGS = "-DOS_IS_WIN32")
    if not withMSVC():
        commonEnvironment.Append(CPPPATH = '/usr/local/include')
        commonEnvironment.Append(CPPPATH = '/usr/include')
        commonEnvironment.Append(CCFLAGS = "-mthreads")
    else:
        commonEnvironment.Append(CCFLAGS = "-DMSVC")

if getPlatform() == 'linux':
    path1 = '/usr/include/python%s' % commonEnvironment['pythonVersion']
    path2 = '/usr/local/include/python%s' % commonEnvironment['pythonVersion']
    pythonIncludePath = [path1, path2]
    path1 = '/usr/include/tcl8.4'
    path2 = '/usr/include/tk8.4'
    tclIncludePath = [path1, path2]
    pythonLinkFlags = []
    if commonEnvironment['Word64'] == '1':
        tmp = '/usr/lib64/python%s/config' % commonEnvironment['pythonVersion']
        pythonLibraryPath = ['/usr/local/lib64', '/usr/lib64', tmp]
    else:
        tmp = '/usr/lib/python%s/config' % commonEnvironment['pythonVersion']
        pythonLibraryPath = ['/usr/local/lib', '/usr/lib', tmp]
    pythonLibs = ['python%s' % commonEnvironment['pythonVersion']]
elif getPlatform() == 'darwin':
    pyBasePath = '/System/Library/Frameworks/Python.Framework'
    pythonIncludePath = ['%s/Headers' % pyBasePath]
    pythonLinkFlags = ['-framework', 'python']
    path1 = '%s/Versions/Current/lib' % pyBasePath
    path2 = '%s/python%s/config' % (path1, commonEnvironment['pythonVersion'])
    pythonLibraryPath = [path1, path2]
    pythonLibs = []
    tclIncludePath = []
elif getPlatform() == 'win32':
    pythonIncludePath = []
    pythonLinkFlags = []
    pythonLibraryPath = []
    tclIncludePath = []
    pythonLibs = ['python%s' % commonEnvironment['pythonVersion'].replace('.', '')]

# Check for prerequisites.
# We check only for headers; checking for libs may fail
# even if they are present, due to secondary dependencies.

libsndfileTests = [
    'int foobar(void)\n{\nreturn (int) SF_FORMAT_SD2;\n}',
    'int foobar(SF_INSTRUMENT *p)\n{\nreturn (int) p->loop_count;\n}',
    'int foobar(void)\n{\nreturn (int) SFC_GET_SIGNAL_MAX;\n}'
]

def CheckSndFile1011(context):
    context.Message('Checking for libsndfile version 1.0.11 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[0] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

def CheckSndFile1013(context):
    context.Message('Checking for libsndfile version 1.0.13 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[1] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

def CheckSndFile1016(context):
    context.Message('Checking for libsndfile version 1.0.16 or later... ')
    testProgram = '\n#include <sndfile.h>\n\n' + libsndfileTests[2] + '\n\n'
    result = context.TryCompile(testProgram, '.c')
    context.Result(result)
    return result

configure = commonEnvironment.Configure(custom_tests = {
    'CheckSndFile1011' : CheckSndFile1011,
    'CheckSndFile1013' : CheckSndFile1013,
    'CheckSndFile1016' : CheckSndFile1016
})

if not configure.CheckHeader("stdio.h", language = "C"):
    print " *** Failed to compile a simple test program. The compiler is"
    print " *** possibly not set up correctly, or is used with invalid flags."
    print " *** Check config.log to find out more about the error."
    Exit(-1)
if not configure.CheckHeader("sndfile.h", language = "C"):
    print "The sndfile library is required to build Csound 5."
    Exit(-1)
portaudioFound = configure.CheckHeader("portaudio.h", language = "C")
portmidiFound = configure.CheckHeader("portmidi.h", language = "C")
fltkFound = configure.CheckHeader("FL/Fl.H", language = "C++")
if fltkFound:
    fltk117Found = configure.CheckHeader("FL/Fl_Spinner.H", language = "C++")
else:
    fltk117Found = 0
boostFound = configure.CheckHeader("boost/any.hpp", language = "C++")
alsaFound = configure.CheckHeader("alsa/asoundlib.h", language = "C")
jackFound = configure.CheckHeader("jack/jack.h", language = "C")
oscFound = configure.CheckHeader("lo/lo.h", language = "C")
stkFound = configure.CheckHeader("Opcodes/stk/include/Stk.h", language = "C++")
pdhfound = configure.CheckHeader("m_pd.h", language = "C")
tclhfound = configure.CheckHeader("tcl.h", language = "C")
if not tclhfound:
     for i in tclIncludePath:
        tmp = '%s/tcl.h' % i
        tclhfound = tclhfound or configure.CheckHeader(tmp, language = "C")
luaFound = configure.CheckHeader("lua.h", language = "C")
swigFound = 'swig' in commonEnvironment['TOOLS']
print 'Checking for SWIG... %s' % (['no', 'yes'][int(swigFound)])
pythonFound = configure.CheckHeader("Python.h", language = "C")
if not pythonFound:
    for i in pythonIncludePath:
        tmp = '%s/Python.h' % i
        pythonFound = pythonFound or configure.CheckHeader(tmp, language = "C")
if getPlatform() == 'darwin':
    tmp = "/System/Library/Frameworks/JavaVM.Framework/Headers/jni.h"
    javaFound = configure.CheckHeader(tmp, language = "C++")
else:
    javaFound = configure.CheckHeader("jni.h", language = "C++")
if getPlatform() == 'linux' and not javaFound:
    if commonEnvironment['buildInterfaces'] != '0':
        if commonEnvironment['buildJavaWrapper'] != '0':
            baseDir = '/usr/lib'
            if commonEnvironment['Word64'] == '1':
                baseDir += '64'
            for i in ['java', 'jvm/java', 'jvm/java-1.5.0']:
                javaIncludePath = '%s/%s/include' % (baseDir, i)
                tmp = '%s/linux/jni_md.h' % javaIncludePath
                if configure.CheckHeader(tmp, language = "C++"):
                    javaFound = 1
                    break
    if javaFound:
        commonEnvironment.Append(CPPPATH = [javaIncludePath])
        commonEnvironment.Append(CPPPATH = [javaIncludePath + '/linux'])

if getPlatform() == 'win32':
    commonEnvironment['ENV']['PATH'] = os.environ['PATH']
    commonEnvironment['SYSTEMROOT'] = os.environ['SYSTEMROOT']

if (commonEnvironment['useFLTK'] == '1' and fltkFound):
   commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_FLTK'])

# Define macros that configure and config.h used to define.
headerMacroCheck = [
    ['io.h',        '-DHAVE_IO_H'       ],
    ['fcntl.h',     '-DHAVE_FCNTL_H'    ],
    ['unistd.h',    '-DHAVE_UNISTD_H'   ],
    ['stdint.h',    '-DHAVE_STDINT_H'   ],
    ['sys/time.h',  '-DHAVE_SYS_TIME_H' ],
    ['sys/types.h', '-DHAVE_SYS_TYPES_H'],
    ['termios.h',   '-DHAVE_TERMIOS_H'  ]]

for h in headerMacroCheck:
    if configure.CheckHeader(h[0], language = "C"):
        commonEnvironment.Append(CPPFLAGS = [h[1]])

if getPlatform() == 'win32':
    if configure.CheckHeader("winsock.h", language = "C"):
        commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')
elif configure.CheckHeader("sys/socket.h", language = "C"):
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_SOCKETS')

if getPlatform() == 'darwin':
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_DIRENT_H')
elif configure.CheckHeader("dirent.h", language = "C"):
    commonEnvironment.Append(CPPFLAGS = '-DHAVE_DIRENT_H')

if configure.CheckSndFile1016():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1016'])
elif configure.CheckSndFile1013():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1013'])
elif configure.CheckSndFile1011():
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1011'])
else:
    commonEnvironment.Prepend(CPPFLAGS = ['-DHAVE_LIBSNDFILE=1000'])

# Package contents.

zipfilename = "csound5-" + getPlatform() + "-" + str(today()) + ".zip"

def buildzip(env, target, source):

    os.chdir('..')
    directories = string.split("csound5")

    extensions = ".def .sh .dll .so .exe "
    extensions = extensions + ".doc .mso .png .xml .gif .jpg .jpeg .nb .wks .xls "
    extensions = extensions + ".c .C .cpp .cxx .h .hpp .H .hxx .py .rc .res .fl .i .java .class "
    extensions = extensions + ".sf2 .SF2 .csd .aif .aiff .jar .smf .mid"
    extensions = string.split(extensions)

    specificFiles = "SConstruct _CsoundVST.* _loris.* loris.py lorisgens.C lorisgens.h morphdemo.py trymorph.csd CsoundCOM.dll msvcp70.dll libsndfile.dll portaudio.dll.0.0.19 msvcr70.dll csound csound.exe CsoundVST CsoundVST.exe CsoundVST.* soundfonts.dll lib"+pythonLibs+".a "
    specificFiles = specificFiles + "README Doxyfile ChangeLog COPYING INSTALL MANIFEST COPYRIGHT AUTHORS TODO all_strings french-strings english-strings"
    specificFiles = string.split(specificFiles)

    print "Types of files to be archived..."
    extensions.sort()
    for extension in extensions:
        print extension
    print

    print "Compiling list of files to archive..."
    pathnames = []
    for directory in directories:
        for root, directories, files in os.walk(directory):
            if files:
                print root
                if root.find("vstsdk2") == -1:
                    for filename in files:
                        basename, extension = os.path.splitext(filename)
                        if extension in extensions or filename in specificFiles:
                            pathname = os.path.join(root, filename)
                            pathnames.append(pathname)
    print
    pathnames.sort()
    for filename in pathnames:
        basename, extension = os.path.splitext(filename)
        if extension in ['.exe', '.dll', '.so']:
            os.system('strip %s' % filename)
            print "Stripped", filename
    print "Creating archive..."
    archive = zipfile.ZipFile("csound5/" + zipfilename, "w", zipfile.ZIP_DEFLATED)
    pathnames.sort()
    for filename in pathnames:
        # print filename
        archive.write(filename)
    archive.close()
    os.chdir('csound5')
    print
    print "Finished packaging '" + zipfilename + "'."

# library version is CS_VERSION.CS_APIVERSION
csoundLibraryVersion = '5.1'
csoundLibraryName = 'csound'
if commonEnvironment['useDouble'] != '0':
    csoundLibraryName += '64'
elif getPlatform() == 'win32':
    csoundLibraryName += '32'
# flags for linking with the Csound library
libCsoundLinkFlags = []
libCsoundLibs = [csoundLibraryName, 'sndfile']

buildOSXFramework = 0
if getPlatform() == 'darwin':
    if commonEnvironment['dynamicCsoundLibrary'] != '0':
        buildOSXFramework = 1
        CsoundLib_OSX = 'CsoundLib'
        # comment the next two lines out to disable building a separate
        # framework (CsoundLib64) for double precision
        if commonEnvironment['useDouble'] != '0':
            CsoundLib_OSX += '64'
        OSXFrameworkBaseDir = '%s.Framework' % CsoundLib_OSX
        tmp = OSXFrameworkBaseDir + '/Versions/%s'
        OSXFrameworkCurrentVersion = tmp % csoundLibraryVersion

csoundLibraryEnvironment = commonEnvironment.Copy()

if commonEnvironment['buildNewParser'] != '0':
    print 'CONFIGURATION DECISION: Building with new parser enabled'
    csoundLibraryEnvironment.Append(YACCFLAGS = ['-d', '-p','csound_orc'])
    csoundLibraryEnvironment.Append(LEXFLAGS = ['-d', '-Pcsound_orc'])
    csoundLibraryEnvironment.Append(CPPFLAGS = ['-DENABLE_NEW_PARSER'])
    yaccBuild = csoundLibraryEnvironment.CFile(target = 'Engine/csound_orcparse.c',
                               source = 'Engine/csound_orc.y')
    lexBuild = csoundLibraryEnvironment.CFile(target = 'Engine/csound_orclex.c',
                               source = 'Engine/csound_orc.l')
else:
    print 'CONFIGURATION DECISION: Not building with new parser'

csoundLibraryEnvironment.Append(CPPFLAGS = ['-D__BUILDING_LIBCSOUND'])
if commonEnvironment['buildRelease'] != '0':
    csoundLibraryEnvironment.Append(CPPFLAGS = ['-D_CSOUND_RELEASE_'])
    if getPlatform() == 'linux':
        if commonEnvironment['Word64'] == '0':
            tmp = '%s/lib/csound/plugins' % commonEnvironment['prefix']
        else:
            tmp = '%s/lib64/csound/plugins' % commonEnvironment['prefix']
        if commonEnvironment['useDouble'] != '0':
            tmp += '64'
        s = '-DCS_DEFAULT_PLUGINDIR=\\"%s\\"' % tmp
        csoundLibraryEnvironment.Append(CPPFLAGS = [s])
    elif buildOSXFramework != 0:
        tmp = '/Library/Frameworks/%s' % OSXFrameworkCurrentVersion
        tmp += '/Resources/Opcodes'
        if commonEnvironment['useDouble'] != '0':
            tmp += '64'
        s = '-DCS_DEFAULT_PLUGINDIR=\\"%s\\"' % tmp
        csoundLibraryEnvironment.Append(CPPFLAGS = [s])
csoundDynamicLibraryEnvironment = csoundLibraryEnvironment.Copy()
csoundDynamicLibraryEnvironment.Append(LIBS = ['sndfile'])
if getPlatform() == 'win32':
    # These are the Windows system call libraries.
    if not withMSVC():
        csoundWindowsLibraries = Split('''
            kernel32 gdi32 wsock32 ws2_32 ole32 uuid winmm
            kernel32 gdi32 wsock32 ws2_32 ole32 uuid winmm
        ''')
    else:
        print 'MSVC'
        csoundWindowsLibraries = Split('''
            kernel32 gdi32 wsock32 ole32 uuid winmm user32.lib ws2_32.lib
            comctl32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib
            ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
            kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
            advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
            odbc32.lib odbccp32.lib
        ''')
if getPlatform() == 'win32':
    csoundDynamicLibraryEnvironment.Append(LIBS = csoundWindowsLibraries)
    csoundDynamicLibraryEnvironment.Append(SHLINKFLAGS = ['-module'])
    csoundDynamicLibraryEnvironment['ENV']['PATH'] = os.environ['PATH']
elif getPlatform() == 'linux':
    csoundDynamicLibraryEnvironment.Append(LIBS = ['dl', 'm', 'pthread'])
csoundInterfacesEnvironment = csoundDynamicLibraryEnvironment.Copy()

if buildOSXFramework:
    csoundFrameworkEnvironment = csoundDynamicLibraryEnvironment.Copy()
    # create directory structure for the framework
    tmp = [OSXFrameworkBaseDir]
    tmp += ['%s/Versions' % OSXFrameworkBaseDir]
    tmp += [OSXFrameworkCurrentVersion]
    tmp += ['%s/Headers' % OSXFrameworkCurrentVersion]
    tmp += ['%s/Resources' % OSXFrameworkCurrentVersion]
    if commonEnvironment['useDouble'] == '0':
        tmp += ['%s/Resources/Opcodes' % OSXFrameworkCurrentVersion]
    else:
        tmp += ['%s/Resources/Opcodes64' % OSXFrameworkCurrentVersion]
    for i in tmp:
        try:
            os.mkdir(i, 0755)
        except:
            pass
    # set up symbolic links
    tmp = [['/Versions/Current', csoundLibraryVersion]]
    tmp += [['/' + CsoundLib_OSX, 'Versions/Current/' + CsoundLib_OSX]]
    tmp += [['/Headers', 'Versions/Current/Headers']]
    tmp += [['/Resources', 'Versions/Current/Resources']]
    for i in tmp:
        try:
            os.remove('%s/%s' % (OSXFrameworkBaseDir, i[0]))
        except:
            pass
        os.symlink(i[1], '%s/%s' % (OSXFrameworkBaseDir, i[0]))

def MacOSX_InstallHeader(headerName):
    if not buildOSXFramework:
        return
    baseName = headerName[(headerName.rfind('/') + 1):]
    targetName = '%s/Headers/%s' % (OSXFrameworkCurrentVersion, baseName)
    cmd = 'cp -f %s %s' % (headerName, targetName)
    csoundFrameworkEnvironment.Command(targetName, headerName, cmd)

def MacOSX_InstallPlugin(fileName):
    if buildOSXFramework:
        pluginDir = '%s/Resources/Opcodes' % OSXFrameworkCurrentVersion
        if commonEnvironment['useDouble'] != '0':
            pluginDir += '64'
        cmd = 'cp -f %s %s/' % (fileName, pluginDir)
        csoundFrameworkEnvironment.Command('%s/%s' % (pluginDir, fileName),
                                           fileName, cmd)

def makePlugin(env, pluginName, srcs):
    pluginLib = env.SharedLibrary(pluginName, srcs)
    pluginLibraries.append(pluginLib)
    MacOSX_InstallPlugin('lib' + pluginName + '.dylib')
    return pluginLib

libCsoundSources = Split('''
Engine/auxfd.c
Engine/cfgvar.c
Engine/entry1.c
Engine/envvar.c
Engine/express.c
Engine/extract.c
Engine/fgens.c
Engine/insert.c
Engine/linevent.c
Engine/memalloc.c
Engine/memfiles.c
Engine/musmon.c
Engine/namedins.c
Engine/otran.c
Engine/rdorch.c
Engine/rdscor.c
Engine/scsort.c
Engine/scxtract.c
Engine/sort.c
Engine/sread.c
Engine/swrite.c
Engine/twarp.c
InOut/libsnd.c
InOut/libsnd_u.c
InOut/midifile.c
InOut/midirecv.c
InOut/midisend.c
InOut/winascii.c
InOut/windin.c
InOut/window.c
InOut/winEPS.c
OOps/aops.c
OOps/bus.c
OOps/cmath.c
OOps/diskin.c
OOps/diskin2.c
OOps/disprep.c
OOps/dumpf.c
OOps/fftlib.c
OOps/goto_ops.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/mxfft.c
OOps/oscils.c
OOps/pstream.c
OOps/pvfileio.c
OOps/pvsanal.c
OOps/random.c
OOps/remote.c
OOps/schedule.c
OOps/sndinfUG.c
OOps/str_ops.c
OOps/ugens1.c
OOps/ugens2.c
OOps/ugens3.c
OOps/ugens4.c
OOps/ugens5.c
OOps/ugens6.c
OOps/ugrw1.c
OOps/ugrw2.c
OOps/vdelay.c
Top/argdecode.c
Top/cscore_internal.c
Top/cscorfns.c
Top/csmodule.c
Top/csound.c
Top/getstring.c
Top/main.c
Top/new_opts.c
Top/one_file.c
Top/opcode.c
Top/threads.c
Top/utility.c
''')

newParserSources = Split('''
Engine/csound_orclex.c
Engine/csound_orcparse.c
Engine/csound_orc_semantics.c
Engine/csound_orc_expressions.c
Engine/csound_orc_optimize.c
Engine/csound_orc_compile.c
Engine/new_orc_parser.c
Engine/symbtab.c
''')

if commonEnvironment['buildNewParser'] != '0':
	libCsoundSources += newParserSources

if commonEnvironment['dynamicCsoundLibrary'] == '1':
    print 'CONFIGURATION DECISION: Building dynamic Csound library'
    if getPlatform() == 'linux':
        libName = 'lib' + csoundLibraryName + '.so'
        libName2 = libName + '.' + csoundLibraryVersion
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', libName])
        os.symlink(libName2, libName)
        tmp = csoundDynamicLibraryEnvironment['SHLINKFLAGS']
        tmp += ['-Wl,-soname=%s' % libName2]
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            libName2, libCsoundSources,
            SHLINKFLAGS = tmp, SHLIBPREFIX = '', SHLIBSUFFIX = '')
    elif getPlatform() == 'darwin':
        libName = CsoundLib_OSX
        libVersion = csoundLibraryVersion
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -compatibility_version -Xlinker %s
        ''' % libVersion))
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -current_version -Xlinker %s
        ''' % libVersion))
        csoundFrameworkEnvironment.Append(SHLINKFLAGS = Split('''
            -install_name /Library/Frameworks/%s/%s
        ''' % (OSXFrameworkCurrentVersion, libName)))
        csoundLibrary = csoundFrameworkEnvironment.SharedLibrary(
            libName, libCsoundSources, SHLIBPREFIX = '', SHLIBSUFFIX = '')
        csoundFrameworkEnvironment.Command(
            '%s/%s' % (OSXFrameworkCurrentVersion, libName),
            libName,
            'cp -f %s %s/' % (libName, OSXFrameworkCurrentVersion))
        for i in headers:
            MacOSX_InstallHeader(i)
        csoundFrameworkEnvironment.Command(
            'CsoundLib_install',
            libName,
            'rm -r /Library/Frameworks/%s; cp -R %s /Library/Frameworks/' % (OSXFrameworkBaseDir, OSXFrameworkBaseDir))
        libCsoundLinkFlags = ['-F.', '-framework', libName, '-lsndfile']
        libCsoundLibs = []
    elif getPlatform() == 'win32':
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            csoundLibraryName, libCsoundSources,
            SHLIBSUFFIX = '.dll.%s' % csoundLibraryVersion)
    else:
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            csoundLibraryName, libCsoundSources)
else:
    print 'CONFIGURATION DECISION: Building static Csound library'
    csoundLibrary = csoundLibraryEnvironment.Library(
        csoundLibraryName, libCsoundSources)
libs.append(csoundLibrary)

pluginEnvironment = commonEnvironment.Copy()
pluginEnvironment.Append(LIBS = ['sndfile'])

if getPlatform() == 'darwin':
    pluginEnvironment.Append(LINKFLAGS = Split('''
        -framework CoreMidi -framework CoreFoundation -framework CoreAudio
    '''))
    # pluginEnvironment.Append(LINKFLAGS = ['-dynamiclib'])
    pluginEnvironment['SHLIBSUFFIX'] = '.dylib'
    pluginEnvironment.Prepend(CXXFLAGS = "-fno-rtti")

csoundProgramEnvironment = commonEnvironment.Copy()
csoundProgramEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
csoundProgramEnvironment.Append(LIBS = libCsoundLibs)

vstEnvironment = commonEnvironment.Copy()
fltkConfigFlags = 'fltk-config --use-images --cflags --cxxflags'
if getPlatform() != 'darwin':
    fltkConfigFlags += ' --ldflags'
try:
    if vstEnvironment.ParseConfig(fltkConfigFlags):
    	print 'Parsed fltk-config.'
except:
    print 'Unable to parse fltk-config.'
if getPlatform() == 'darwin':
    vstEnvironment.Append(LIBS = Split('''
        fltk fltk_images fltk_png z fltk_jpeg
    '''))
guiProgramEnvironment = commonEnvironment.Copy()

if getPlatform() == 'win32':
    if not withMSVC():
        vstEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        guiProgramEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        vstEnvironment.Append(LIBS = ['stdc++', 'supc++'])
        guiProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
    else:
        csoundProgramEnvironment.Append(LINKFLAGS = ["/IMPLIB:dummy.lib"])
    csoundProgramEnvironment.Append(LIBS = csoundWindowsLibraries)
    vstEnvironment.Append(LIBS = csoundWindowsLibraries)
    guiProgramEnvironment.Append(LIBS = csoundWindowsLibraries)
else:
    if getPlatform() == 'linux':
        csoundProgramEnvironment.Append(LIBS = ['dl'])
        vstEnvironment.Append(LIBS = ['dl'])
        guiProgramEnvironment.Append(LIBS = ['dl'])
    csoundProgramEnvironment.Append(LIBS = ['pthread', 'm'])
    vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    if getPlatform() == 'darwin':
        csoundProgramEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
        '''))

#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

if getPlatform() == 'win32':
    PYDLL = r'%s\%s' % (os.environ['SystemRoot'], pythonLibs[0])
if getPlatform() == 'win32' and pythonLibs[0] < 'python24' and not withMSVC():
    pythonImportLibrary = csoundInterfacesEnvironment.Command(
        '/usr/local/lib/lib%s.a' % (pythonLibs[0]),
        PYDLL,
        ['pexports %s > %s.def' % (PYDLL, pythonLibs[0]),
         'dlltool --input-def %s.def --dllname %s.dll --output-lib /usr/local/lib/lib%s.a' % (pythonLibs[0], PYDLL, pythonLibs[0])])

def fixCFlagsForSwig(env):
    if '-pedantic' in env['CCFLAGS']:
        env['CCFLAGS'].remove('-pedantic')
    if '-pedantic' in env['CXXFLAGS']:
        env['CXXFLAGS'].remove('-pedantic')
    if not withMSVC():
        # work around non-ANSI type punning in SWIG generated wrapper files
        env['CCFLAGS'].append('-fno-strict-aliasing')

def makePythonModule(env, targetName, srcs):
    if getPlatform() == 'darwin':
        env.Prepend(LINKFLAGS = ['-bundle'])
        pyModule_ = env.Program('_%s.so' % targetName, srcs)
    else:
        pyModule_ = env.SharedLibrary('_%s' % targetName, srcs)
        if getPlatform() == 'win32' and pythonLibs[0] < 'python24':
            Depends(pyModule_, pythonImportLibrary)
    pythonModules.append(pyModule_)
    pythonModules.append('%s.py' % targetName)
    return pyModule_

if not ((pythonFound or luaFound or javaFound) and swigFound and commonEnvironment['buildInterfaces'] == '1'):
    print 'CONFIGURATION DECISION: Not building Csound interfaces library.'
else:
    print 'CONFIGURATION DECISION: Building Csound interfaces library.'
    print "Python Version" + commonEnvironment['pythonVersion']
    csoundInterfacesEnvironment.Append(CPPPATH = ['interfaces'])
    csoundInterfacesSources = []
    for i in Split('CppSound CsoundFile Soundfile csPerfThread cs_glue filebuilding'):
        csoundInterfacesSources.append(
            csoundInterfacesEnvironment.SharedObject('interfaces/%s.cpp' % i))
    if commonEnvironment['dynamicCsoundLibrary'] == '1' or getPlatform() == 'win32':
        csoundInterfacesEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
        csoundInterfacesEnvironment.Prepend(LIBS = libCsoundLibs)
    else:
        for i in libCsoundSources:
            csoundInterfacesSources.append(
                csoundInterfacesEnvironment.SharedObject(i))
    if getPlatform() == 'win32':
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
    elif getPlatform() == 'linux':
        csoundInterfacesEnvironment.Prepend(LIBS = ['util'])
    if withMSVC() != '1':
        csoundInterfacesEnvironment.Prepend(LIBS = ['stdc++'])
    csoundInterfacesEnvironment.Append(SWIGFLAGS = Split('''
        -c++ -includeall -verbose
    '''))
    csoundWrapperEnvironment = csoundInterfacesEnvironment.Copy()
    fixCFlagsForSwig(csoundWrapperEnvironment)
    csoundWrapperEnvironment.Append(CPPFLAGS = '-D__BUILDING_CSOUND_INTERFACES')
    for option in csoundWrapperEnvironment['CCFLAGS']:
        if string.find(option, '-D') == 0:
            csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    for option in csoundWrapperEnvironment['CPPFLAGS']:
        if string.find(option, '-D') == 0:
            csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    for option in csoundWrapperEnvironment['CPPPATH']:
        option = '-I' + option
        csoundWrapperEnvironment.Append(SWIGFLAGS = [option])
    swigflags = csoundWrapperEnvironment['SWIGFLAGS']
    if javaFound and commonEnvironment['buildJavaWrapper'] == '1':
        print 'CONFIGURATION DECISION: Building Java wrappers for Csound interfaces library.'
        csoundJavaWrapperEnvironment = csoundInterfacesEnvironment.Copy()
        if getPlatform() == 'darwin':
            csoundWrapperEnvironment.Append(CPPPATH =
                ['/System/Library/Frameworks/JavaVM.Framework/Headers'])
        if getPlatform() == 'linux':
            # ugly hack to work around bug that requires running scons twice
            tmp = [csoundWrapperEnvironment['SWIG']]
            for i in swigflags:
                tmp += [i]
            tmp += ['-java', '-package', 'csnd']
            tmp += ['-o', 'interfaces/java_interface_wrap.cc']
            tmp += ['interfaces/java_interface.i']
            if os.spawnvp(os.P_WAIT, tmp[0], tmp) != 0:
                Exit(-1)
            csoundJavaWrapperSources = [csoundWrapperEnvironment.SharedObject(
                'interfaces/java_interface_wrap.cc')]
        else:
            csoundJavaWrapperSources = [csoundWrapperEnvironment.SharedObject(
                'interfaces/java_interface.i',
                SWIGFLAGS = [swigflags, '-java', '-package', 'csnd'])]
        csoundJavaWrapperSources += ['interfaces/pyMsgCb_stub.cpp']
        csoundJavaWrapperSources += csoundInterfacesSources
        if getPlatform() == 'darwin':
            csoundJavaWrapperEnvironment.Prepend(LINKFLAGS = ['-bundle'])
            csoundJavaWrapperEnvironment.Append(LINKFLAGS =
                ['-framework', 'JavaVM', '-Wl'])
            csoundJavaWrapper = csoundJavaWrapperEnvironment.Program(
                'lib_jcsound.jnilib', csoundJavaWrapperSources)
        else:
            csoundJavaWrapper = csoundJavaWrapperEnvironment.SharedLibrary(
                '_jcsound', csoundJavaWrapperSources)
        Depends(csoundJavaWrapper, csoundLibrary)
        libs.append(csoundJavaWrapper)
        jcsnd = csoundJavaWrapperEnvironment.Java(
            target = './interfaces', source = './interfaces',
            JAVACFLAGS = ['-source', '1.4', '-target', '1.4'])
        try:
            os.mkdir('interfaces/csnd', 0755)
        except:
            pass
        jcsndJar = csoundJavaWrapperEnvironment.Jar(
            'csnd.jar', ['interfaces/csnd'], JARCHDIR = 'interfaces')
        Depends(jcsndJar, jcsnd)
        libs.append(jcsndJar)
    else:
        print 'CONFIGURATION DECISION: Not building Java wrappers for Csound interfaces library.'
    csoundInterfacesSources.insert(0,
        csoundInterfacesEnvironment.SharedObject('interfaces/pyMsgCb.cpp'))
    if pythonFound:
        csoundWrapperEnvironment.Append(CPPPATH = pythonIncludePath)
        csoundInterfacesEnvironment.Append(LINKFLAGS = pythonLinkFlags)
        csoundInterfacesEnvironment.Prepend(LIBPATH = pythonLibraryPath)
        csoundInterfacesEnvironment.Prepend(LIBS = pythonLibs)
        csoundPythonInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/python_interface.i',
            SWIGFLAGS = [swigflags, '-python', '-outdir', '.'])
        if getPlatform() == 'win32' and pythonLibs[0] < 'python24' and not withMSVC():
            Depends(csoundPythonInterface, pythonImportLibrary)
        if getPlatform() != 'darwin':
            csoundInterfacesSources.insert(0, csoundPythonInterface)
            pythonModules.append('csnd.py')
    if not luaFound:
        print 'CONFIGURATION DECISION: Not building Csound Lua interface library.'
    else:
        print 'CONFIGURATION DECISION: Building Csound Lua interface library.'
        csoundLuaInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/lua_interface.i',
            SWIGFLAGS = [swigflags, '-lua', '-outdir', '.'])
        if getPlatform() != 'darwin':
            csoundInterfacesSources.insert(0, csoundLuaInterface)
            if getPlatform() == 'win32':
                csoundInterfacesEnvironment.Prepend(LIBS = ['lua51'])
            else:
                csoundInterfacesEnvironment.Prepend(LIBS = ['lua'])
    if getPlatform() == 'linux':
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', '_csnd.so'])
        os.symlink('lib_csnd.so', '_csnd.so')
        csoundInterfacesEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
    if getPlatform() == 'darwin':
        ilibName = "lib_csnd.dylib"
        ilibVersion = csoundLibraryVersion
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -compatibility_version -Xlinker %s
        ''' % ilibVersion))
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''
            -Xlinker -current_version -Xlinker %s
        ''' % ilibVersion))
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = Split('''
            -install_name /Library/Frameworks/CsoundLib.framework/Versions/%s/%s
        ''' % ('5.1', ilibName)))
    csoundInterfaces = csoundInterfacesEnvironment.SharedLibrary(
        '_csnd', csoundInterfacesSources)
    Depends(csoundInterfaces, csoundLibrary)
    libs.append(csoundInterfaces)
    if getPlatform() == 'darwin' and (pythonFound or luaFound):
        csoundInterfacesBundleEnvironment = csoundInterfacesEnvironment.Copy()
        csoundInterfacesBundleSources = []
        if pythonFound:
            csoundInterfacesBundleSources += csoundPythonInterface
        if luaFound:
            csoundInterfacesBundleSources += csoundLuaInterface
            csoundInterfacesBundleEnvironment.Prepend(LIBS = ['lua'])
        csoundInterfacesBundleEnvironment.Prepend(LIBS = ['_csnd'])
        csoundInterfacesBundle = makePythonModule(
            csoundInterfacesBundleEnvironment,
            'csnd', csoundInterfacesBundleSources)
        Depends(csoundInterfacesBundle, csoundInterfaces)
        Depends(csoundInterfacesBundle, csoundLibrary)

if commonEnvironment['generatePdf'] == '0':
    print 'CONFIGURATION DECISION: Not generating PDF documentation.'
else:
    print 'CONFIGURATION DECISION: Generating PDF documentation.'
    refmanTex = commonEnvironment.Command('doc/latex/refman.tex', 'Doxyfile', ['doxygen $SOURCE'])
    Depends(refmanTex, csoundLibrary)
    zipDependencies.append(refmanTex)
    csoundPdf = commonEnvironment.Command('refman.pdf', 'doc/latex/refman.tex', ['pdflatex --include-directory=doc/latex --interaction=nonstopmode --job-name=CsoundAPI $SOURCE'])
    Depends(csoundPdf, refmanTex)
    zipDependencies.append(csoundPdf)

# Plugin opcodes.

makePlugin(pluginEnvironment, 'stdopcod', Split('''
    Opcodes/ambicode.c      Opcodes/bbcut.c         Opcodes/biquad.c
    Opcodes/butter.c        Opcodes/clfilt.c        Opcodes/cross2.c
    Opcodes/dam.c           Opcodes/dcblockr.c      Opcodes/filter.c
    Opcodes/flanger.c       Opcodes/follow.c        Opcodes/fout.c
    Opcodes/freeverb.c      Opcodes/ftconv.c        Opcodes/ftgen.c
    Opcodes/gab/gab.c       Opcodes/gab/vectorial.c Opcodes/grain.c
    Opcodes/locsig.c        Opcodes/lowpassr.c      Opcodes/metro.c
    Opcodes/midiops2.c      Opcodes/midiops3.c      Opcodes/newfils.c
    Opcodes/nlfilt.c        Opcodes/oscbnk.c        Opcodes/pluck.c
    Opcodes/repluck.c       Opcodes/reverbsc.c      Opcodes/seqtime.c
    Opcodes/sndloop.c       Opcodes/sndwarp.c       Opcodes/space.c
    Opcodes/spat3d.c        Opcodes/syncgrain.c     Opcodes/ugens7.c
    Opcodes/ugens9.c        Opcodes/ugensa.c        Opcodes/uggab.c
    Opcodes/ugmoss.c        Opcodes/ugnorman.c      Opcodes/ugsc.c
    Opcodes/wave-terrain.c
    Opcodes/stdopcod.c
'''))



if getPlatform() == 'linux' or getPlatform() == 'darwin':
    makePlugin(pluginEnvironment, 'control', ['Opcodes/control.c'])
makePlugin(pluginEnvironment, 'ftest', ['Opcodes/ftest.c'])
makePlugin(pluginEnvironment, 'mixer', ['Opcodes/mixer.cpp'])
makePlugin(pluginEnvironment, 'modal4',
           ['Opcodes/modal4.c', 'Opcodes/physutil.c'])
makePlugin(pluginEnvironment, 'physmod', Split('''
    Opcodes/physmod.c Opcodes/physutil.c Opcodes/mandolin.c Opcodes/singwave.c
    Opcodes/fm4op.c Opcodes/moog1.c Opcodes/shaker.c Opcodes/bowedbar.c
'''))
makePlugin(pluginEnvironment, 'pitch',
           ['Opcodes/pitch.c', 'Opcodes/pitch0.c', 'Opcodes/spectra.c'])
makePlugin(pluginEnvironment, 'scansyn',
           ['Opcodes/scansyn.c', 'Opcodes/scansynx.c'])
sfontEnvironment = pluginEnvironment.Copy()
if (not withMSVC()):
    sfontEnvironment.Append(CCFLAGS = ['-fno-strict-aliasing'])
if sys.byteorder == 'big':
    sfontEnvironment.Append(CCFLAGS = ['-DWORDS_BIGENDIAN'])
makePlugin(sfontEnvironment, 'sfont', ['Opcodes/sfont.c'])
makePlugin(pluginEnvironment, 'babo', ['Opcodes/babo.c'])
makePlugin(pluginEnvironment, 'barmodel', ['Opcodes/bilbar.c'])
makePlugin(pluginEnvironment, 'compress', ['Opcodes/compress.c'])
makePlugin(pluginEnvironment, 'grain4', ['Opcodes/grain4.c'])
makePlugin(pluginEnvironment, 'hrtferX', ['Opcodes/hrtferX.c'])
makePlugin(pluginEnvironment, 'loscilx', ['Opcodes/loscilx.c'])
makePlugin(pluginEnvironment, 'minmax', ['Opcodes/minmax.c'])
makePlugin(pluginEnvironment, 'phisem', ['Opcodes/phisem.c'])
makePlugin(pluginEnvironment, 'pvoc', Split('''
    Opcodes/dsputil.c Opcodes/pvadd.c Opcodes/pvinterp.c Opcodes/pvocext.c
    Opcodes/pvread.c Opcodes/ugens8.c Opcodes/vpvoc.c Opcodes/pvoc.c
'''))
makePlugin(pluginEnvironment, 'pvs_ops', Split('''
    Opcodes/ifd.c Opcodes/partials.c Opcodes/psynth.c Opcodes/pvsbasic.c
    Opcodes/pvscent.c Opcodes/pvsdemix.c Opcodes/pvs_ops.c
'''))
makePlugin(pluginEnvironment, 'stackops', ['Opcodes/stackops.c'])
makePlugin(pluginEnvironment, 'vbap',
           ['Opcodes/vbap.c', 'Opcodes/vbap_eight.c', 'Opcodes/vbap_four.c',
            'Opcodes/vbap_sixteen.c', 'Opcodes/vbap_zak.c'])
makePlugin(pluginEnvironment, 'vaops', ['Opcodes/vaops.c'])
makePlugin(pluginEnvironment, 'ugakbari', ['Opcodes/ugakbari.c'])
makePlugin(pluginEnvironment, 'harmon', ['Opcodes/harmon.c'])
makePlugin(pluginEnvironment, 'ampmidid', ['Opcodes/ampmidid.cpp'])
makePlugin(pluginEnvironment, 'date', ['Opcodes/date.c'])
makePlugin(pluginEnvironment, 'system_call', ['Opcodes/system_call.c'])
makePlugin(pluginEnvironment, 'ptrack', ['Opcodes/pitchtrack.c'])
makePlugin(pluginEnvironment, 'mutexops', ['Opcodes/mutexops.cpp'])

# Plugins with External Dependencies

# FLTK widgets

if not (commonEnvironment['useFLTK'] == '1' and fltkFound):
    print 'CONFIGURATION DECISION: Not building with FLTK graphs and widgets.'
else:
    widgetsEnvironment = pluginEnvironment.Copy()
    widgetsEnvironment.Append(CCFLAGS = ['-DCS_VSTHOST'])
    if (commonEnvironment['noFLTKThreads'] == '1'):
        widgetsEnvironment.Append(CCFLAGS = ['-DNO_FLTK_THREADS'])
    if getPlatform() == 'linux':
        widgetsEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        widgetsEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        widgetsEnvironment.Append(LIBS = ['fltk'])
        if (not withMSVC()):
            widgetsEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            widgetsEnvironment.Prepend(
                LINKFLAGS = ['-Wl,--enable-runtime-pseudo-reloc'])
        widgetsEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        widgetsEnvironment.Append(LIBS = ['fltk', 'stdc++', 'pthread', 'm'])
        widgetsEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
            -framework ApplicationServices
        '''))
    makePlugin(widgetsEnvironment, 'widgets',
               ['InOut/FL_graph.cpp', 'InOut/winFLTK.c', 'InOut/widgets.cpp'])

    if commonEnvironment['buildVirtual'] == '0' or not fltk117Found:
        print "CONFIGURATION DECISION: Not building Virtual Keyboard plugin. (FLTK 1.1.7+ required)"
    else:
        print "CONFIGURATION DECISION: Building Virtual Keyboard plugin."
        widgetsEnvironment.Append(CPPPATH = ['./InOut', './InOut/virtual_keyboard'])
        makePlugin(widgetsEnvironment, 'virtual',
               ['InOut/virtual_keyboard/FLTKKeyboard.cpp',
               'InOut/virtual_keyboard/FLTKKeyboardWindow.cpp',
               'InOut/virtual_keyboard/FLTKKeyboardWidget.cpp',
               'InOut/virtual_keyboard/virtual_keyboard.cpp',
               'InOut/virtual_keyboard/Bank.cpp',
               'InOut/virtual_keyboard/KeyboardMapping.cpp',
               'InOut/virtual_keyboard/Program.cpp',
               'InOut/virtual_keyboard/SliderBank.cpp',
               'InOut/virtual_keyboard/SliderData.cpp'])

# REAL TIME AUDIO

if commonEnvironment['useCoreAudio'] == '1' and getPlatform() == 'darwin':
    print "CONFIGURATION DECISION: Building CoreAudio plugin."
    coreaudioEnvironment = pluginEnvironment.Copy()
    coreaudioEnvironment.Append(CCFLAGS = ['-I/system/library/Frameworks/CoreAudio.framework/Headers'])
    makePlugin(coreaudioEnvironment, 'rtcoreaudio', ['InOut/rtcoreaudio.c'])
else:
    print "CONFIGURATION DECISION: Not building CoreAudio plugin."

if not (commonEnvironment['useALSA'] == '1' and alsaFound):
    print "CONFIGURATION DECISION: Not building ALSA plugin."
else:
    print "CONFIGURATION DECISION: Building ALSA plugin."
    alsaEnvironment = pluginEnvironment.Copy()
    alsaEnvironment.Append(LIBS = ['asound', 'pthread'])
    makePlugin(alsaEnvironment, 'rtalsa', ['InOut/rtalsa.c'])

if getPlatform() == 'win32':
    winmmEnvironment = pluginEnvironment.Copy()
    winmmEnvironment.Append(LIBS = ['winmm', 'gdi32', 'kernel32'])
    makePlugin(winmmEnvironment, 'rtwinmm', ['InOut/rtwinmm.c'])

if not (commonEnvironment['usePortAudio'] == '1' and portaudioFound):
    print "CONFIGURATION DECISION: Not building PortAudio module."
else:
    print "CONFIGURATION DECISION: Building PortAudio module."
    portaudioEnvironment = pluginEnvironment.Copy()
    portaudioEnvironment.Append(LIBS = ['portaudio'])
    if (getPlatform() == 'linux'):
        if (commonEnvironment['useJack']=='1' and jackFound):
            print "Adding Jack library for PortAudio"
            portaudioEnvironment.Append(LIBS = ['jack'])
        portaudioEnvironment.Append(LIBS = ['asound', 'pthread'])
    elif getPlatform() == 'win32':
        portaudioEnvironment.Append(LIBS = ['winmm', 'dsound'])
        portaudioEnvironment.Append(LIBS = csoundWindowsLibraries)
    makePlugin(portaudioEnvironment, 'rtpa', ['InOut/rtpa.c'])

if not (commonEnvironment['useJack'] == '1' and jackFound):
    print "CONFIGURATION DECISION: Not building JACK plugin."
else:
    print "CONFIGURATION DECISION: Building JACK plugin."
    jackEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'linux':
        jackEnvironment.Append(LIBS = ['jack', 'asound', 'pthread'])
    else:
        jackEnvironment.Append(LIBS = ['jack', 'pthread'])
    makePlugin(jackEnvironment, 'rtjack', ['InOut/rtjack.c'])

if commonEnvironment['usePortMIDI'] == '1' and portmidiFound:
    print 'CONFIGURATION DECISION: Building with PortMIDI.'
    portMidiEnvironment = pluginEnvironment.Copy()
    portMidiEnvironment.Append(LIBS = ['portmidi'])
    if getPlatform() != 'darwin':
        portMidiEnvironment.Append(LIBS = ['porttime'])
    if getPlatform() == 'win32':
        portMidiEnvironment.Append(LIBS = ['winmm'])
    if getPlatform() == 'linux' and alsaFound:
        portMidiEnvironment.Append(LIBS = ['asound'])
    makePlugin(portMidiEnvironment, 'pmidi', ['InOut/pmidi.c'])
else:
    print 'CONFIGURATION DECISION: Not building with PortMIDI.'

if not (commonEnvironment['useOSC'] == '1' and oscFound):
    print "CONFIGURATION DECISION: Not building OSC plugin."
else:
    print "CONFIGURATION DECISION: Building OSC plugin."
    oscEnvironment = pluginEnvironment.Copy()
    oscEnvironment.Append(LIBS = ['lo', 'pthread'])
    if getPlatform() == 'win32':
        oscEnvironment.Append(LIBS = csoundWindowsLibraries)
        oscEnvironment.Append(SHLINKFLAGS = ['-Wl,--enable-stdcall-fixup'])
    makePlugin(oscEnvironment, 'osc', ['Opcodes/OSC.c'])

# UDP opcodes

if commonEnvironment['useUDP'] == '0':
    print "CONFIGURATION DECISION: Not building UDP plugins."
else:
    print "CONFIGURATION DECISION: Building UDP plugins."
    udpEnvironment = pluginEnvironment.Copy()
    udpEnvironment.Append(LIBS = ['pthread'])
    makePlugin(udpEnvironment, 'udprecv', ['Opcodes/sockrecv.c'])
    makePlugin(udpEnvironment, 'udpsend', ['Opcodes/socksend.c'])

# end udp opcodes

# FLUIDSYNTH OPCODES

if not configure.CheckHeader("fluidsynth.h", language = "C"):
    print "CONFIGURATION DECISION: Not building fluid opcodes."
else:
    print "CONFIGURATION DECISION: Building fluid opcodes."
    fluidEnvironment = pluginEnvironment.Copy()
    fluidEnvironment.Append(LIBS = ['fluidsynth'])
    if getPlatform() == 'win32':
        fluidEnvironment.Append(CPPFLAGS = ['-DFLUIDSYNTH_NOT_A_DLL'])
        fluidEnvironment.Append(LIBS = ['winmm', 'dsound'])
        fluidEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'linux' or getPlatform() == 'darwin':
        fluidEnvironment.Append(LIBS = ['pthread'])
    makePlugin(fluidEnvironment, 'fluidOpcodes',
               ['Opcodes/fluidOpcodes/fluidOpcodes.c'])

# VST HOST OPCODES

if (getPlatform() == 'win32' or getPlatform() == 'linux') and fltkFound:
    vst4Environment = vstEnvironment.Copy()
    vst4Environment.Append(LIBS = ['fltk'])
    vst4Environment.Append(CPPFLAGS = ['-DCS_VSTHOST'])
    if not withMSVC():
        vst4Environment.Append(LIBS = ['stdc++'])
    if getPlatform() == 'win32':
        vst4Environment.Append(LIBS = csoundWindowsLibraries)
    vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
    makePlugin(vst4Environment, 'vst4cs', Split('''
        Opcodes/vst4cs/src/vst4cs.cpp Opcodes/vst4cs/src/fxbank.cpp
        Opcodes/vst4cs/src/vsthost.cpp
    '''))
elif getPlatform() == 'darwin' and fltkFound:
    vst4Environment = vstEnvironment.Copy()
    vst4Environment.Append(LIBS = ['fltk'])
    vst4Environment.Append(LIBS = ['stdc++'])
    vst4Environment.Append(LINKFLAGS=['-framework', 'carbon', '-framework', 'ApplicationServices'])
    vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
    vst4Environment.Append(CPPFLAGS = ['-DCS_VSTHOST'])
    makePlugin(vst4Environment, 'vst4cs', Split('''
        Opcodes/vst4cs/src/vst4cs.cpp Opcodes/vst4cs/src/fxbank.cpp
        Opcodes/vst4cs/src/vsthost.cpp
    '''))

# Build the Loris and Python opcodes here

if not (commonEnvironment['buildLoris'] == '1' and configure.CheckHeader("Opcodes/Loris/src/loris.h") and configure.CheckHeader("fftw3.h")):
    print "CONFIGURATION DECISION: Not building Loris Python extension and Csound opcodes."
else:
    print "CONFIGURATION DECISION: Building Loris Python extension and Csound opcodes."
    # For Loris, we build only the loris Python extension module and
    # the Csound opcodes (modified for Csound 5).
    # It is assumed that you have copied all contents of the Loris
    # distribution into the csound5/Opcodes/Loris directory, e.g.
    # csound5/Opcodes/Loris/src/*, etc.
    lorisEnvironment = pluginEnvironment.Copy()
    lorisEnvironment.Append(CCFLAGS = '-DHAVE_FFTW3_H')
    if commonEnvironment['buildRelease'] == '0':
        lorisEnvironment.Append(CCFLAGS = '-DDEBUG_LORISGENS')
    if getPlatform() == 'win32':
        lorisEnvironment.Append(CCFLAGS = '-D_MSC_VER')
    if not withMSVC():
        lorisEnvironment.Append(CCFLAGS = Split('''
            -Wno-comment -Wno-unknown-pragmas -Wno-sign-compare
        '''))
    lorisEnvironment.Append(CPPPATH = Split('''
        Opcodes/Loris Opcodes/Loris/src ./
    '''))
    lorisSources = glob.glob('Opcodes/Loris/src/*.[Cc]')
    if 'Opcodes/Loris/src/lorisgens.C' in lorisSources:
        lorisSources.remove('Opcodes/Loris/src/lorisgens.C')
    lorisLibrarySources = []
    for i in lorisSources:
        lorisLibrarySources += lorisEnvironment.SharedObject(i)
    lorisLibrary = lorisEnvironment.StaticLibrary(
        'lorisbase', lorisLibrarySources)
    lorisEnvironment.Prepend(LIBS = ['lorisbase', 'fftw3', 'stdc++'])
    # The following file has been patched for Csound 5
    # and you should update it from Csound 5 CVS.
    lorisOpcodes = makePlugin(lorisEnvironment, 'loris',
                              ['Opcodes/Loris/lorisgens5.C'])
    Depends(lorisOpcodes, lorisLibrary)
    lorisPythonEnvironment = lorisEnvironment.Copy()
    fixCFlagsForSwig(lorisPythonEnvironment)
    lorisPythonEnvironment.Append(CPPPATH = pythonIncludePath)
    lorisPythonEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    lorisPythonEnvironment.Append(LIBPATH = pythonLibraryPath)
    if getPlatform() != 'darwin':
        lorisPythonEnvironment.Prepend(LIBS = pythonLibs)
    lorisPythonEnvironment.Append(SWIGPATH = ['./'])
    lorisPythonEnvironment.Prepend(SWIGFLAGS = Split('''
        -module loris -c++ -includeall -verbose -outdir . -python
        -DHAVE_FFTW3_H -I./Opcodes/Loris/src -I.
    '''))
    lorisPythonWrapper = lorisPythonEnvironment.SharedObject(
        'Opcodes/Loris/scripting/loris.i')
    lorisPythonEnvironment['SHLIBPREFIX'] = ''
    lorisPythonModule = makePythonModule(lorisPythonEnvironment,
                                         'loris', lorisPythonWrapper)
    Depends(lorisPythonModule, lorisLibrary)

if not (commonEnvironment['buildStkOpcodes'] == '1' and stkFound):
    print 'CONFIGURATION DECISION: Not building STK opcodes.'
else:
    print 'CONFIGURATION DECISION: Building STK opcodes.'
    # For the STK opcodes, the STK distribution include, src, and rawwaves
    # directories should be copied thusly:
    #   csound5/Opcodes/stk/include
    #   csound5/Opcodes/stk/src
    #   csound5/Opcodes/stk/rawwaves
    # Then, the following sources (and any other future I/O or OS dependent
    # sources) should be ignored:
    removeSources = Split('''
        Opcodes/stk/src/InetWvIn.cpp    Opcodes/stk/src/InetWvOut.cpp
        Opcodes/stk/src/Mutex.cpp       Opcodes/stk/src/RtAudio.cpp
        Opcodes/stk/src/RtMidi.cpp      Opcodes/stk/src/RtDuplex.cpp
        Opcodes/stk/src/RtWvIn.cpp      Opcodes/stk/src/RtWvOut.cpp
        Opcodes/stk/src/Socket.cpp      Opcodes/stk/src/TcpClient.cpp
        Opcodes/stk/src/TcpServer.cpp   Opcodes/stk/src/Thread.cpp
        Opcodes/stk/src/UdpSocket.cpp
    ''')
    stkEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'win32':
        stkEnvironment.Append(CCFLAGS = '-D__OS_WINDOWS__')
    elif getPlatform() == 'linux':
        stkEnvironment.Append(CCFLAGS = '-D__OS_LINUX__')
    elif getPlatform() == 'darwin':
        stkEnvironment.Append(CCFLAGS = '-D__OS_MACOSX__')
    if sys.byteorder == 'big':
        stkEnvironment.Append(CCFLAGS = '-D__BIG_ENDIAN__')
    else:
        stkEnvironment.Append(CCFLAGS = '-D__LITTLE_ENDIAN__')
    stkEnvironment.Prepend(CPPPATH = Split('''
        Opcodes/stk/include Opcodes/stk/src ./ ./../include
    '''))
    stkSources_ = glob.glob('Opcodes/stk/src/*.cpp')
    stkSources = []
    for source in stkSources_:
        stkSources.append(source.replace('\\', '/'))
    for removeMe in removeSources:
        stkSources.remove(removeMe)
    stkLibrarySources = []
    for i in stkSources:
        stkLibrarySources += stkEnvironment.SharedObject(i)
    stkLibrary = stkEnvironment.StaticLibrary('stk_base', stkLibrarySources)
    stkEnvironment.Prepend(LIBS = ['stk_base'])
    if not withMSVC():
        stkEnvironment.Append(LIBS = ['stdc++'])
    if getPlatform() == 'win32':
        stkEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'linux' or getPlatform() == 'darwin':
        stkEnvironment.Append(LIBS = ['pthread'])
    # This is the one that actually defines the opcodes.
    # They are straight wrappers, as simple as possible.
    stk = makePlugin(stkEnvironment, 'stk', ['Opcodes/stk/stkOpcodes.cpp'])
    Depends(stk, stkLibrary)

if not (pythonFound and commonEnvironment['buildPythonOpcodes'] != '0'):
    print "CONFIGURATION DECISION: Not building Python opcodes."
else:
    print "CONFIGURATION DECISION: Building Python opcodes."
    pyEnvironment = pluginEnvironment.Copy()
    pyEnvironment.Append(CPPPATH = pythonIncludePath)
    pyEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    pyEnvironment.Append(LIBPATH = pythonLibraryPath)
    pyEnvironment.Append(LIBS = pythonLibs)
    if getPlatform() == 'linux':
        pyEnvironment.Append(LIBS = ['util', 'dl', 'm'])
    elif getPlatform() == 'darwin':
        pyEnvironment.Append(LIBS = ['dl', 'm'])
    elif getPlatform() == 'win32':
        pyEnvironment['ENV']['PATH'] = os.environ['PATH']
        pyEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
    pythonOpcodes = makePlugin(pyEnvironment, 'py',
                               ['Opcodes/py/pythonopcodes.c'])
    if getPlatform() == 'win32' and pythonLibs[0] < 'python24':
        Depends(pythonOpcodes, pythonImportLibrary)

# Utility programs.

stdutilSources = Split('''
    util/atsa.c         util/cvanal.c       util/dnoise.c
    util/envext.c       util/xtrct.c        util/het_export.c
    util/het_import.c   util/hetro.c        util/lpanal.c
    util/lpc_export.c   util/lpc_import.c   util/mixer.c
    util/pvanal.c       util/pvlook.c       util/scale.c
    util/sndinfo.c      util/srconv.c       util/pv_export.c
    util/pv_import.c
    util/std_util.c
''')
stdutilSources += pluginEnvironment.SharedObject('util/sdif', 'SDIF/sdif.c')
makePlugin(pluginEnvironment, 'stdutil', stdutilSources)

if (commonEnvironment['buildUtilities'] != '0'):
    utils = [
        ['atsa',        'util/atsa_main.c'    ],
        ['cvanal',      'util/cvl_main.c'     ],
        ['dnoise',      'util/dnoise_main.c'  ],
        ['envext',      'util/env_main.c'     ],
        ['extractor',   'util/xtrc_main.c'    ],
        ['het_export',  'util/hetx_main.c'    ],
        ['het_import',  'util/heti_main.c'    ],
        ['hetro',       'util/het_main.c'     ],
        ['lpanal',      'util/lpc_main.c'     ],
        ['lpc_export',  'util/lpcx_main.c'    ],
        ['lpc_import',  'util/lpci_main.c'    ],
        ['mixer',       'util/mixer_main.c'   ],
        ['pvanal',      'util/pvc_main.c'     ],
        ['pvlook',      'util/pvl_main.c'     ],
        ['pv_export',   'util/pvx_main.c'     ],
        ['pv_import',   'util/pvi_main.c'     ],
        ['scale',       'util/scale_main.c'   ],
        ['sndinfo',     'util/sndinfo_main.c' ],
        ['srconv',      'util/srconv_main.c'  ]]
    for i in utils:
        executables.append(csoundProgramEnvironment.Program(i[0], i[1]))

executables.append(csoundProgramEnvironment.Program('scsort',
    ['util1/sortex/smain.c']))
executables.append(csoundProgramEnvironment.Program('extract',
    ['util1/sortex/xmain.c']))
executables.append(commonEnvironment.Program('cs',
    ['util1/csd_util/cs.c']))
executables.append(commonEnvironment.Program('csb64enc',
    ['util1/csd_util/base64.c', 'util1/csd_util/csb64enc.c']))
executables.append(commonEnvironment.Program('makecsd',
    ['util1/csd_util/base64.c', 'util1/csd_util/makecsd.c']))
executables.append(commonEnvironment.Program('scot',
    ['util1/scot/scot_main.c', 'util1/scot/scot.c']))

#executables.append(csoundProgramEnvironment.Program('cscore',
#    ['util1/cscore/cscore_main.c']))
executables.append(commonEnvironment.Program('sdif2ad',
    ['SDIF/sdif2adsyn.c', 'SDIF/sdif.c', 'SDIF/sdif-mem.c']))

makedb = commonEnvironment.Program('makedb', ['strings/makedb.c'])
zipDependencies.append(makedb)

# Front ends.

def addOSXResourceFork(env, baseName, dirName):
    if getPlatform() == 'darwin':
        if dirName != '':
            fileName = dirName + '/' + baseName
        else:
            fileName = baseName
        env.Command(('%s/resources' % fileName).replace('/', '_'), fileName,
                    "/Developer/Tools/Rez -i APPL -o $SOURCE cs5.r")

csoundProgramSources = ['frontends/csound/csound_main.c']
if getPlatform() == 'linux':
    csoundProgramSources = ['frontends/csound/sched.c'] + csoundProgramSources
csoundProgram = csoundProgramEnvironment.Program('csound', csoundProgramSources)
executables.append(csoundProgram)
Depends(csoundProgram, csoundLibrary)

def fluidTarget(env, dirName, baseName, objFiles):
    flFile = dirName + '/' + baseName + '.fl'
    cppFile = dirName + '/' + baseName + '.cpp'
    hppFile = dirName + '/' + baseName + '.hpp'
    env.Command(cppFile, flFile,
                'fluid -c -o %s -h %s %s' % (cppFile, hppFile, flFile))
    for i in objFiles:
        Depends(i, cppFile)
    return cppFile

if not (commonEnvironment['buildCsound5GUI'] != '0' and fltk117Found):
    print 'CONFIGURATION DECISION: Not building FLTK CSOUND5GUI frontend.'
else:
    print 'CONFIGURATION DECISION: Building FLTK GUI CSOUND5GUI frontend.'
    csound5GUIEnvironment = csoundProgramEnvironment.Copy()
    csound5GUIEnvironment.Append(CPPPATH = ['./interfaces'])
    if jackFound:
        csound5GUIEnvironment.Append(LIBS = ['jack'])
        csound5GUIEnvironment.Prepend(CPPFLAGS = ['-DHAVE_JACK'])
    if getPlatform() == 'linux':
        csound5GUIEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        csound5GUIEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        csound5GUIEnvironment.Append(LIBS = ['fltk'])
        if (not withMSVC()):
            csound5GUIEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            csound5GUIEnvironment.Prepend(LINKFLAGS = Split('''
                -mwindows -Wl,--enable-runtime-pseudo-reloc
            '''))
        csound5GUIEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        csound5GUIEnvironment.Prepend(CXXFLAGS = "-fno-rtti")
        csound5GUIEnvironment.Append(LIBS = Split('''
            fltk stdc++ pthread m
        '''))
        csound5GUIEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework ApplicationServices
        '''))
    csound5GUISources = Split('''
        frontends/fltk_gui/ConfigFile.cpp
        frontends/fltk_gui/CsoundCopyrightInfo.cpp
        frontends/fltk_gui/CsoundGlobalSettings.cpp
        frontends/fltk_gui/CsoundGUIConsole.cpp
        frontends/fltk_gui/CsoundGUIMain.cpp
        frontends/fltk_gui/CsoundPerformance.cpp
        frontends/fltk_gui/CsoundPerformanceSettings.cpp
        frontends/fltk_gui/CsoundUtility.cpp
        frontends/fltk_gui/CsoundEditor.cpp
        frontends/fltk_gui/Fl_Native_File_Chooser.cxx
        frontends/fltk_gui/main.cpp
    ''')
    csound5GUIFluidSources = Split('''
        CsoundAboutWindow_FLTK
        CsoundGlobalSettingsPanel_FLTK
        CsoundGUIConsole_FLTK
        CsoundGUIMain_FLTK
        CsoundPerformanceSettingsPanel_FLTK
        CsoundUtilitiesWindow_FLTK
    ''')
    csound5GUIObjectFiles = []
    csound5GUIFluidObjectFiles = []
    for i in csound5GUISources:
        csound5GUIObjectFiles += csound5GUIEnvironment.Object(i)
    csound5GUIObjectFiles += csound5GUIEnvironment.Object(
        'frontends/fltk_gui/csPerfThread', 'interfaces/csPerfThread.cpp')
    for i in csound5GUIFluidSources:
        csound5GUIFluidObjectFiles += csound5GUIEnvironment.Object(
            fluidTarget(csound5GUIEnvironment, 'frontends/fltk_gui', i,
                        csound5GUIObjectFiles))
    csound5GUIObjectFiles += csound5GUIFluidObjectFiles
    csound5GUI = csound5GUIEnvironment.Program('csound5gui',
                                               csound5GUIObjectFiles)
    Depends(csound5GUI, csoundLibrary)
    executables.append(csound5GUI)
    if getPlatform() == 'darwin':
        appDir = 'frontends/fltk_gui/Csound5GUI.app/Contents/MacOS'
        addOSXResourceFork(csound5GUIEnvironment, 'csound5gui', '')
        csound5GUIEnvironment.Command(
            '%s/csound5gui' % appDir, 'csound5gui', "cp $SOURCE %s/" % appDir)
        addOSXResourceFork(csound5GUIEnvironment, 'csound5gui', appDir)

if not ((commonEnvironment['buildCsoundVST'] == '1') and boostFound and fltkFound):
    print 'CONFIGURATION DECISION: Not building CsoundVST plugin and standalone.'
else:
    print 'CONFIGURATION DECISION: Building CsoundVST plugin and standalone.'
    headers += glob.glob('frontends/CsoundVST/*.h')
    headers += glob.glob('frontends/CsoundVST/*.hpp')
    vstEnvironment.Append(CPPPATH = ['frontends/CsoundVST', 'interfaces'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST', 'interfaces'])
    vstEnvironment.Append(CPPPATH = pythonIncludePath)
    vstEnvironment.Append(LINKFLAGS = pythonLinkFlags)
    vstEnvironment.Append(LIBPATH = pythonLibraryPath)
    if getPlatform() != 'darwin':
        vstEnvironment.Prepend(LIBS = pythonLibs)
    vstEnvironment.Prepend(LIBS = ['_csnd'])
    vstEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    vstEnvironment.Append(LIBS = libCsoundLibs)
    vstEnvironment.Append(SWIGFLAGS = Split('-c++ -includeall -verbose -outdir .'))
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['util', 'dl', 'm'])
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-Wl,-rpath-link,.'])
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', '_CsoundVST.so'])
        os.symlink('lib_CsoundVST.so', '_CsoundVST.so')
    elif getPlatform() == 'darwin':
        vstEnvironment.Append(LIBS = ['dl', 'm'])
        # vstEnvironment.Append(CXXFLAGS = ['-fabi-version=0']) # if gcc3.2-3
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        vstEnvironment['SHLIBSUFFIX'] = '.dylib'
    elif getPlatform() == 'win32':
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        vstEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
        vstEnvironment.Append(CCFLAGS = ['-DNDEBUG'])
        guiProgramEnvironment.Prepend(LINKFLAGS = Split('''
            -mwindows -Wl,--enable-runtime-pseudo-reloc
        '''))
        vstEnvironment.Prepend(
            LINKFLAGS = ['-Wl,--enable-runtime-pseudo-reloc'])
        vstEnvironment.Append(LIBS = ['fltk_images', 'fltk'])
        guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    for option in vstEnvironment['CCFLAGS']:
        if string.find(option, '-D') == 0:
            vstEnvironment.Append(SWIGFLAGS = [option])
    for option in vstEnvironment['CPPFLAGS']:
        if string.find(option, '-D') == 0:
            vstEnvironment.Append(SWIGFLAGS = [option])
    for option in vstEnvironment['CPPPATH']:
        option = '-I' + option
        vstEnvironment.Append(SWIGFLAGS = [option])
    print 'PATH =', commonEnvironment['ENV']['PATH']
    csoundVstBaseSources = []
    for i in ['AudioEffect', 'audioeffectx', 'Conversions', 'Shell', 'System']:
        csoundVstBaseSources += vstEnvironment.SharedObject(
            'frontends/CsoundVST/%s.cpp' % i)
    csoundVstSources = csoundVstBaseSources + Split('''
    frontends/CsoundVST/Cell.cpp
    frontends/CsoundVST/Composition.cpp
    frontends/CsoundVST/Counterpoint.cpp
    frontends/CsoundVST/CounterpointNode.cpp
    frontends/CsoundVST/CsoundVST.cpp
    frontends/CsoundVST/CsoundVstFltk.cpp
    frontends/CsoundVST/CsoundVSTMain.cpp
    frontends/CsoundVST/CsoundVstUi.cpp
    frontends/CsoundVST/Event.cpp
    frontends/CsoundVST/Hocket.cpp
    frontends/CsoundVST/ImageToScore.cpp
    frontends/CsoundVST/Lindenmayer.cpp
    frontends/CsoundVST/MCRM.cpp
    frontends/CsoundVST/Midifile.cpp
    frontends/CsoundVST/MusicModel.cpp
    frontends/CsoundVST/Node.cpp
    frontends/CsoundVST/Random.cpp
    frontends/CsoundVST/Rescale.cpp
    frontends/CsoundVST/Score.cpp
    frontends/CsoundVST/ScoreNode.cpp
    frontends/CsoundVST/Sequence.cpp
    frontends/CsoundVST/Soundfile.cpp
    frontends/CsoundVST/StrangeAttractor.cpp
    frontends/CsoundVST/Voicelead.cpp
    frontends/CsoundVST/VoiceleadingNode.cpp
    ''')
    # These are the Windows system call libraries.
    if getPlatform() == 'win32':
        vstEnvironment.Append(LIBS = csoundWindowsLibraries)
        vstEnvironment.Append(SHLINKFLAGS = ['-module'])
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        csoundVstSources.append('frontends/CsoundVST/_CsoundVST.def')
    swigflags = vstEnvironment['SWIGFLAGS']
    vstWrapperEnvironment = vstEnvironment.Copy()
    fixCFlagsForSwig(vstWrapperEnvironment)
    csoundVstPythonWrapper = vstWrapperEnvironment.SharedObject(
        'frontends/CsoundVST/CsoundVST.i', SWIGFLAGS = [swigflags, '-python'])
    if getPlatform() != 'darwin':
        csoundVstSources.insert(0, csoundVstPythonWrapper)
        pythonModules.append('CsoundVST.py')
    csoundvst =  vstEnvironment.SharedLibrary('_CsoundVST', csoundVstSources)
    libs.append(csoundvst)
    # Depends(csoundvst, 'frontends/CsoundVST/CsoundVST_wrap.cc')
    Depends(csoundvst, csoundInterfaces)
    Depends(csoundvst, csoundLibrary)
    if getPlatform() == 'darwin':
        vstPythonEnvironment = vstEnvironment.Copy()
        vstPythonEnvironment.Prepend(LIBS = ['_CsoundVST'])
        csoundvstPythonModule = makePythonModule(
            vstPythonEnvironment, 'CsoundVST', csoundVstPythonWrapper)
        Depends(csoundvstPythonModule, csoundvst)

    scoregenSources = csoundVstBaseSources + Split('''
    frontends/CsoundVST/ScoreGenerator.cpp
    frontends/CsoundVST/ScoreGeneratorVst.cpp
    frontends/CsoundVST/ScoreGeneratorVstUi.cpp
    frontends/CsoundVST/ScoreGeneratorVstFltk.cpp
    frontends/CsoundVST/ScoreGeneratorVstMain.cpp
    ''')
    if getPlatform() == 'win32':
        scoregenSources.append('frontends/CsoundVST/_scoregen.def')
    scoregenWrapperEnvironment = vstEnvironment.Copy()
    fixCFlagsForSwig(scoregenWrapperEnvironment)
    scoregenPythonWrapper = scoregenWrapperEnvironment.SharedObject(
        'frontends/CsoundVST/ScoreGeneratorVST.i',
        SWIGFLAGS = [swigflags, '-python'])
    scoregenSources.insert(0, scoregenPythonWrapper)
    scoregenEnvironment = vstEnvironment.Copy()
    scoregenEnvironment['SHLIBPREFIX'] = ''
    scoregen = makePythonModule(
        scoregenEnvironment, 'scoregen', scoregenSources)
    Depends(scoregen, csoundInterfaces)
    Depends(scoregen, csoundLibrary)

    guiProgramEnvironment.Prepend(LIBS = ['_CsoundVST', '_csnd'])
    guiProgramEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    guiProgramEnvironment.Append(LIBS = libCsoundLibs)
    csoundvstGui = guiProgramEnvironment.Program(
        'CsoundVST', ['frontends/CsoundVST/csoundvst_main.cpp'])
    executables.append(csoundvstGui)
    Depends(csoundvstGui, csoundvst)

    counterpoint = vstEnvironment.Program(
        'counterpoint', ['frontends/CsoundVST/CounterpointMain.cpp'])
    zipDependencies.append(counterpoint)

if not ((commonEnvironment['buildCSEditor'] == '1') and fltkFound):
    print 'CONFIGURATION DECISION: Not building Csound Text Editor.'
else:
    if getPlatform() == 'linux' or getPlatform() == 'darwin':
        csEdit = commonEnvironment.Copy()
        csEditor = csEdit.Command('cseditor', 'frontends/cseditor/cseditor.cxx', "fltk-config --compile frontends/cseditor/cseditor.cxx")
        executables.append(csEditor)
    else:
    	csEditor = vstEnvironment.Program('cseditor', ['frontends/cseditor/cseditor.cxx'])
    	executables.append(csEditor)

if commonEnvironment['buildPDClass'] == '1' and pdhfound:
    print "CONFIGURATION DECISION: Building PD csoundapi~ class"
    pdClassEnvironment = commonEnvironment.Copy()
    pdClassEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    pdClassEnvironment.Append(LIBS = libCsoundLibs)
    if getPlatform() == 'darwin':
        pdClassEnvironment.Append(LINKFLAGS = Split('''
            -bundle -flat_namespace -undefined suppress
            -framework Carbon -framework ApplicationServices
        '''))
        pdClass = pdClassEnvironment.Program(
            'csoundapi~.pd_darwin',
            'frontends/csoundapi_tilde/csoundapi_tilde.c')
    elif getPlatform() == 'linux':
        pdClass = pdClassEnvironment.SharedLibrary(
            'csoundapi~.pd_linux',
            'frontends/csoundapi_tilde/csoundapi_tilde.c',
            SHLIBPREFIX = '', SHLIBSUFFIX = '')
    elif getPlatform() == 'win32':
        pdClassEnvironment.Append(LIBS = ['pd'])
        pdClassEnvironment.Append(LIBS = csoundWindowsLibraries)
        pdClassEnvironment.Append(SHLINKFLAGS = ['-module'])
        pdClassEnvironment['ENV']['PATH'] = os.environ['PATH']
        pdClass = pdClassEnvironment.SharedLibrary(
            'csoundapi~', 'frontends/csoundapi_tilde/csoundapi_tilde.c')
    Depends(pdClass, csoundLibrary)
    libs.append(pdClass)

if commonEnvironment['buildTclcsound'] == '1' and tclhfound:
    print "CONFIGURATION DECISION: Building Tclcsound frontend"
    csTclEnvironment = commonEnvironment.Copy()
    csTclEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    csTclEnvironment.Append(LIBS = libCsoundLibs)
    if getPlatform() == 'darwin':
        csTclEnvironment.Append(CCFLAGS = Split('''
            -I/Library/Frameworks/Tcl.Framework/Headers
            -I/Library/Frameworks/Tk.Framework/Headers
            -I/System/Library/Frameworks/Tcl.Framework/Headers
            -I/System/Library/Frameworks/Tk.Framework/Headers
        '''))
        csTclEnvironment.Append(LINKFLAGS = Split('''
            -framework tk -framework tcl
        '''))
    elif getPlatform() == 'linux':
        csTclEnvironment.Append(CPPPATH = tclIncludePath)
        csTclEnvironment.Append(LIBS = ['tcl8.4', 'tk8.4', 'dl', 'pthread'])
    elif getPlatform() == 'win32':
        csTclEnvironment.Append(LIBS = ['tcl84', 'tk84'])
        csTclEnvironment.Append(LIBS = csoundWindowsLibraries)
        csTclEnvironment.Append(SHLINKFLAGS = ['-module'])
    csTclCmdObj = csTclEnvironment.SharedObject(
        'frontends/tclcsound/commands.c')
    csTcl = csTclEnvironment.Program(
        'cstclsh', ['frontends/tclcsound/main_tclsh.c', csTclCmdObj])
    csTk = csTclEnvironment.Program(
        'cswish', ['frontends/tclcsound/main_wish.c', csTclCmdObj])
    Tclcsoundlib = csTclEnvironment.SharedLibrary(
        'tclcsound', ['frontends/tclcsound/tclcsound.c', csTclCmdObj],
        SHLIBPREFIX = '')
    if getPlatform() == 'darwin':
        csTclEnvironment.Command('cswish_resources', 'cswish', "/Developer/Tools/Rez -i APPL -o cswish frontends/tclcsound/cswish.r")
        if commonEnvironment['dynamicCsoundLibrary'] == '1':
           csTclEnvironment.Command('tclcsound_install', 'tclcsound.dylib', 'mkdir /Library/Frameworks/CsoundLib.framework/Resources/TclTk; cp -R tclcsound.dylib /Library/Frameworks/CsoundLib.framework/Resources/TclTk/')
    Depends(csTcl, csoundLibrary)
    Depends(csTk, csoundLibrary)
    Depends(Tclcsoundlib, csoundLibrary)
    executables.append(csTcl)
    executables.append(csTk)
    libs.append(Tclcsoundlib)
    try:
            os.mkdir('tclcsound', 0755)
    except:
            pass
#    if getPlatform() == 'darwin':
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound.dylib','cp tclcsound.dylib tclcsound; tclsh pkgbuild.tcl')
#    elif getPlatform() == 'linux':
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound.so','cp tclcsound.so tclcsound; tclsh pkgbuild.tcl')
#    elif  getPlatform() == 'win32':
#      csTclEnvironment.Command('tclcsound/tclcsound.dll', 'tclcsound.dll','cp tclcsound.dll tclcsound')
#      csTclEnvironment.Command('tclcsound/pkgIndex.tcl', 'tclcsound/tclcsound.dll','tclsh84 pkgbuild.tcl')

else:
    print "CONFIGURATION DECISION: Not building Tclcsound"

if commonEnvironment['buildWinsound'] == '1' and fltkFound:
    print "CONFIGURATION DECISION: Building Winsound frontend"
    # should these be installed ?
    # headers += glob.glob('frontends/winsound/*.h')
    csWinEnvironment = commonEnvironment.Copy()
    csWinEnvironment.Append(LINKFLAGS = libCsoundLinkFlags)
    csWinEnvironment.Append(LIBS = libCsoundLibs)
    # not used
    # if (commonEnvironment['noFLTKThreads'] == '1'):
    #     csWinEnvironment.Append(CCFLAGS = ['-DNO_FLTK_THREADS'])
    if getPlatform() == 'linux':
        csWinEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        csWinEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'win32':
        csWinEnvironment.Append(LIBS = ['fltk'])
        if not withMSVC():
            csWinEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            csWinEnvironment.Prepend(LINKFLAGS = Split('''
                -mwindows -Wl,--enable-runtime-pseudo-reloc
            '''))
        csWinEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        csWinEnvironment.Append(CXXFLAGS = ['-fno-rtti'])
        csWinEnvironment.Append(LIBS = ['fltk', 'stdc++', 'pthread', 'm'])
        csWinEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
            -framework ApplicationServices
        '''))
        appDir = 'frontends/winsound/Winsound.app/Contents/MacOS'
        addOSXResourceFork(csWinEnvironment, 'winsound', '')
        csWinEnvironment.Command(
            '%s/winsound' % appDir, 'winsound', "cp $SOURCE %s/" % appDir)
        addOSXResourceFork(csWinEnvironment, 'winsound', appDir)
    winsoundFL = 'frontends/winsound/winsound.fl'
    winsoundSrc = 'frontends/winsound/winsound.cxx'
    winsoundHdr = 'frontends/winsound/winsound.h'
    csWinEnvironment.Command(
        winsoundSrc, winsoundFL,
        'fluid -c -o %s -h %s %s' % (winsoundSrc, winsoundHdr, winsoundFL))
    winsoundMain = csWinEnvironment.Object('frontends/winsound/main.cxx')
    Depends(winsoundMain, winsoundSrc)
    winsound5 = csWinEnvironment.Program(
        'winsound', [winsoundMain, winsoundSrc])
    Depends(winsound5, csoundLibrary)
    executables.append(winsound5)
else:
    print "CONFIGURATION DECISION: Not building Winsound"

if (getPlatform() == 'darwin' and commonEnvironment['buildOSXGUI'] == '1'):
    print "CONFIGURATION DECISION: building OSX GUI frontend"
    csOSXGUIEnvironment = commonEnvironment.Copy()
    OSXGUI = csOSXGUIEnvironment.Command(
        '''frontends/OSX/build/Csound 5.app/Contents/MacOS/Csound 5''',
        'frontends/OSX/main.c',
        "cd frontends/OSX; xcodebuild -buildstyle Deployment")
    Depends(OSXGUI, csoundLibrary)
else:
    print "CONFIGURATION DECISION: Not building OSX GUI frontend"

if (commonEnvironment['buildDSSI'] == '1' and getPlatform() == 'linux'):
    print "CONFIGURATION DECISION: Building DSSI plugin host opcodes."
    dssiEnvironment = pluginEnvironment.Copy()
    dssiEnvironment.Append(LIBS = ['dl'])
    makePlugin(dssiEnvironment, 'dssi4cs',
               ['Opcodes/dssi4cs/src/load.c', 'Opcodes/dssi4cs/src/dssi4cs.c'])
else:
    print "CONFIGURATION DECISION: Not building DSSI plugin host opcodes."

if (commonEnvironment['generateTags']=='0') or (getPlatform() != 'darwin' and getPlatform() != 'linux'):
    print "CONFIGURATION DECISION: Not calling TAGS"
else:
    print "CONFIGURATION DECISION: Calling TAGS"
    allSources = string.join(glob.glob('*/*.h*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.hpp'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.h'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.hpp'))
    tags = commonEnvironment.Command('TAGS', Split(allSources), 'etags $SOURCES')
    zipDependencies.append(tags)
    Depends(tags, csoundLibrary)

if commonEnvironment['generateXmg'] == '1':
    print "CONFIGURATION DECISION: Calling makedb"
    if getPlatform() == 'win32':
        makedbCmd = 'makedb'
    else:
        makedbCmd = './makedb'
    xmgs = commonEnvironment.Command(
                'American.xmg', ['strings/all_strings'],
                '%s strings/all_strings American' % makedbCmd)
    xmgs1 = commonEnvironment.Command(
                'English.xmg', ['strings/english-strings'],
                '%s strings/english-strings English' % makedbCmd)
    xmgs2 = commonEnvironment.Command(
                'csound.xmg', ['strings/english-strings'],
                '%s strings/english-strings csound' % makedbCmd)
    Depends(xmgs, makedb)
    zipDependencies.append(xmgs)
    Depends(xmgs1, makedb)
    zipDependencies.append(xmgs1)
    Depends(xmgs2, makedb)
    zipDependencies.append(xmgs2)
else:
    print "CONFIGURATION DECISION: Not calling makedb"

zipDependencies += executables
zipDependencies += libs
zipDependencies += pluginLibraries
zipDependencies += pythonModules

if commonEnvironment['generateZip'] == '0':
    print 'CONFIGURATION DECISION: Not compiling zip file for release.'
else:
    print 'CONFIGURATION DECISION: Compiling zip file for release.'
    zip = commonEnvironment.Command(zipfilename, csoundLibrary, buildzip)
    for node in zipDependencies:
        Depends(zip, node)

# INSTALL OPTIONS

PREFIX = commonEnvironment['prefix']

BIN_DIR = PREFIX + "/bin"
INCLUDE_DIR = PREFIX + "/include/csound"

if (commonEnvironment['Word64'] == '1'):
    LIB_DIR = PREFIX + "/lib64"
    PYTHON_DIR = '%s/lib64' % sys.prefix
else:
    LIB_DIR = PREFIX + "/lib"
    PYTHON_DIR = '%s/lib' % sys.prefix
PYTHON_DIR += '/python%s/site-packages' % commonEnvironment['pythonVersion']

for i in sys.path:
    if i[:sys.prefix.__len__()] == sys.prefix and i[-13:] == 'site-packages':
        PYTHON_DIR = i

if commonEnvironment['useDouble'] == '0':
    PLUGIN_DIR = LIB_DIR + "/csound/plugins"
else:
    PLUGIN_DIR = LIB_DIR + "/csound/plugins64"

if commonEnvironment['install'] == '1':
    installExecutables = Alias('install-executables',
        Install(BIN_DIR, executables))
    installOpcodes = Alias('install-opcodes',
        Install(PLUGIN_DIR, pluginLibraries))
    installHeaders = Alias('install-headers',
        Install(INCLUDE_DIR, headers))
    installLibs = Alias('install-libs',
        Install(LIB_DIR, libs))
    installPythonModules = Alias('install-py',
        Install(PYTHON_DIR, pythonModules))
    Alias('install', [installExecutables, installOpcodes, installLibs, installHeaders, installPythonModules])

if getPlatform() == 'darwin' and commonEnvironment['useFLTK'] == '1':
    print "CONFIGURATION DECISION: Adding resource fork for csound"
    addOSXResourceFork(commonEnvironment, 'csound', '')

