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
#
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

def today():
    return time.strftime("%Y-%m-%d", time.localtime())

def getPlatform():
    if sys.platform[:5] == 'linux':
        return 'linux'
    elif sys.platform[:3] == 'win':
        return 'mingw'
    elif sys.platform[:6] == 'darwin':
        return 'darwin'
    else:
        return 'unsupported'

#############################################################################
#
#   DEFINE CONFIGURATION
#
#############################################################################

# Detect platform.

print "System platform is '" + sys.platform + "'."

# Define configuration options.

opts = Options('custom.py')
opts.Add('CC')
opts.Add('CXX')
opts.Add('LINK')
opts.Add('LINKFLAGS')
opts.Add('customCPPPATH', 'List of custom CPPPATH variables')
opts.Add('customCCFLAGS')
opts.Add('customCXXFLAGS')
opts.Add('customLIBS')
opts.Add('customLIBPATH')
opts.Add('customSHLINKFLAGS')
opts.Add('customSWIGFLAGS')
opts.Add('useDouble',
    'Set to 1 to use double-precision floating point for audio samples.',
    '0')
opts.Add('usePortAudio',
    'Set to 1 to use PortAudio for real-time audio input and output.',
    '1')
opts.Add('usePortMIDI',
    'Build PortMidi plugin for real time MIDI input and output.',
    '1')
opts.Add('useALSA',
    'Set to 1 to use ALSA for real-time audio and MIDI input and output.',
    '1')
opts.Add('useJack',
    'Set to 1 if you compiled PortAudio to use Jack; also builds Jack plugin.',
    '1')
opts.Add('useFLTK',
    'Set to 1 to use FLTK for graphs and widget opcodes.',
    '1')
if getPlatform() != 'darwin':
    opts.Add('noFLTKThreads',
        'Set to 1 to disable use of a separate thread for FLTK widgets.',
        '0')
else:
    opts.Add('noFLTKThreads',
        'Set to 0 to enable use of a separate thread for FLTK widgets.',
        '1')
opts.Add('pythonVersion',
    'Set to the Python version to be used.',
    '2.3')
opts.Add('buildCsoundVST',
    'Set to 1 to build CsoundVST (needs FLTK, boost, Python 2.3, SWIG).',
    '0')
opts.Add('generateTags',
    'Set to 1 to generate TAGS',
    '0')
opts.Add('generatePdf',
    'Set to 1 to generate PDF documentation',
    '0')
opts.Add('generateXmg',
    'Set to 1 to generate string database',
    '1')
opts.Add('generateZip',
    'Set to 1 to generate zip archive',
    '0')
opts.Add('buildLoris',
    'Set to 1 to build the Loris Python extension and opcodes',
    '1')
opts.Add('useOSC',
    'Set to 1 if you want OSC support',
    '0')
opts.Add('buildPythonOpcodes',
    'Set to 1 to build Python opcodes',
    '0')
opts.Add('prefix',
    'Base directory for installs. Defaults to /usr/local.',
    '/usr/local')
opts.Add('buildRelease',
    'Set to 1 to build for release (implies noDebug).',
    '0')
opts.Add('noDebug',
    'Build without debugging information.',
    '0')
opts.Add('gcc3opt',
    'Enable gcc 3.3.x or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
opts.Add('gcc4opt',
    'Enable gcc 4.0 or later optimizations for the specified CPU architecture (e.g. pentium3); implies noDebug.',
    '0')
opts.Add('useLrint',
    'Use lrint() and lrintf() for converting floating point values to integers.',
    '0')
opts.Add('useGprof',
    'Build with profiling information (-pg).',
    '0')
opts.Add('Word64',
    'Build for 64bit computer',
    '0')
opts.Add('dynamicCsoundLibrary',
    'Build dynamic Csound library instead of libcsound.a',
    '0')
opts.Add('buildStkOpcodes',
    "Build opcodes encapsulating Perry Cook's Synthesis Toolkit in C++ instruments and effects",
    '0')
opts.Add('install',
    'Enables the Install targets',
    '1')
opts.Add('buildPDClass',
    "build csoundapi~ PD class (needs m_pd.h in the standard places)",
    '0')
opts.Add('useCoreAudio',
    "Set to 1 to use CoreAudio for real-time audio input and output.",
    '1')
opts.Add('useAltivec',
    "On OSX use the gcc AltiVec optmisation flags",
    '0')
opts.Add('MSVC',
    "Using MSVC build tools",
    '0')
opts.Add('buildDSSI',
    "Build DSSI/LADSPA host opcodes",
    '1')
opts.Add('buildUtilities',
    "Build stand-alone executables for utilities that can also be used with -U",
    '1')
opts.Add('buildTclcsound',
    "Build Tclcsound frontend (cstclsh, cswish and tclcsound dynamic module). Requires Tcl/Tk headers and libs",
    '0')
opts.Add('buildInterfaces',
    "Build interface library for Python, JAVA, Lua, C++, and other languages.",
    '1')
opts.Add('buildJavaWrapper',
    'Set to 1 to build Java wrapper for the interface library.',
    '0')

# Define the common part of the build environment.
# This section also sets up customized options for third-party libraries, which
# should take priority over default options.

commonEnvironment = Environment(options = opts, ENV = {'PATH' : os.environ['PATH']})

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

Help(opts.GenerateHelpText(commonEnvironment))

# Define options for different platforms.

print "Build platform is '" + getPlatform() + "'."
print "SCons tools on this platform: ", commonEnvironment['TOOLS']
print

commonEnvironment.Prepend(CPPPATH = ['.', './H'])
if (commonEnvironment['useLrint'] != '0'):
  commonEnvironment.Prepend(CCFLAGS = ['-DUSE_LRINT'])
if (commonEnvironment['gcc3opt'] != '0' or commonEnvironment['gcc4opt'] != '0'):
  if (commonEnvironment['gcc4opt'] != '0'):
    commonEnvironment.Prepend(CCFLAGS = ['-ftree-vectorize'])
    cpuType = commonEnvironment['gcc4opt']
  else:
    cpuType = commonEnvironment['gcc3opt']
  commonEnvironment.Prepend(CCFLAGS = ['-fomit-frame-pointer', '-ffast-math'])
  if (getPlatform() == 'darwin'):
    flags = '-O3 -mcpu=%s -mtune=%s'%(cpuType, cpuType)
  else:
    flags = '-O3 -march=%s'%(cpuType)
  commonEnvironment.Prepend(CCFLAGS = Split(flags))
elif commonEnvironment['buildRelease'] != '0':
  if commonEnvironment['MSVC'] == '0':
    commonEnvironment.Prepend(CCFLAGS = Split('''
      -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
    '''))
elif commonEnvironment['noDebug'] == '0':
  if (getPlatform() == 'darwin'):
    commonEnvironment.Prepend(CCFLAGS = ['-g', '-O2'])
  else:
    commonEnvironment.Prepend(CCFLAGS = ['-g', '-gstabs', '-O2'])
else:
  commonEnvironment.Prepend(CCFLAGS = ['-O2'])
if (commonEnvironment['useGprof']=='1'):
  commonEnvironment.Append(CCFLAGS = ['-pg'])
  commonEnvironment.Append(LINKFLAGS = ['-pg'])
commonEnvironment.Prepend(CXXFLAGS = ['-fexceptions'])
commonEnvironment.Prepend(LIBPATH = ['.', '#.'])
if commonEnvironment['buildRelease'] == '0':
    commonEnvironment.Prepend(CPPFLAGS = ['-DBETA'])
if (commonEnvironment['Word64']=='1'):
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib64'])
else:
    commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/local/lib'])

if (commonEnvironment['useDouble']=='0'):
    print 'CONFIGURATION DECISION: Using single-precision floating point for audio samples.'
else:
    print 'CONFIGURATION DECISION: Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = ['-DUSE_DOUBLE'])

# Define different build environments for different types of targets.

if commonEnvironment['MSVC'] == '0':
    commonEnvironment.Prepend(CCFLAGS = "-Wall")

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CPPPATH = '/usr/X11R6/include')
    if commonEnvironment['buildInterfaces'] != '0':
        if commonEnvironment['buildJavaWrapper'] != '0':
            commonEnvironment.Append(CPPPATH = '/usr/lib/java/include')
            commonEnvironment.Append(CPPPATH = '/usr/lib/java/include/linux')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(LINKFLAGS = ['-Wl,-Bdynamic'])
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    if (commonEnvironment['useAltivec'] == '1'):
        print 'CONFIGURATION DECISION using Altivec optmisation'
        commonEnvironment.Append(CCFLAGS = "-faltivec")
    commonEnvironment.Prepend(CXXFLAGS = "-fno-rtti")
elif getPlatform() == 'mingw':
    commonEnvironment.Append(CCFLAGS = "-D_WIN32")
    commonEnvironment.Append(CCFLAGS = "-DWIN32")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(CCFLAGS = "-DOS_IS_WIN32")
    if commonEnvironment['MSVC'] == '0':
        commonEnvironment.Append(CPPPATH = '/usr/local/include')
        commonEnvironment.Append(CPPPATH = '/usr/include')
        commonEnvironment.Append(CCFLAGS = "-mthreads")
    else:
        commonEnvironment.Append(CCFLAGS = "-DMSVC")

if getPlatform() == 'linux':
    path1 = '/usr/include/python%s' % commonEnvironment['pythonVersion']
    path2 = '/usr/local/include/python%s' % commonEnvironment['pythonVersion']
    pythonIncludePath = [path1, path2]
    pythonLinkFlags = []
    if (commonEnvironment['Word64'] == '1'):
        tmp = '/usr/lib64/python%s/config' % commonEnvironment['pythonVersion']
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
elif getPlatform() == 'mingw':
    pythonIncludePath = []
    pythonLinkFlags = []
    pythonLibraryPath = []
    pythonLibs = ['python%s' % commonEnvironment['pythonVersion'].replace('.', '')]

# Check for prerequisites.
# We check only for headers; checking for libs may fail
# even if they are present, due to secondary dependencies.

configure = commonEnvironment.Configure()

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
boostFound = configure.CheckHeader("boost/any.hpp", language = "C++")
alsaFound = configure.CheckHeader("alsa/asoundlib.h", language = "C")
jackFound = configure.CheckHeader("jack/jack.h", language = "C")
oscFound = configure.CheckHeader("lo/lo.h", language = "C")
stkFound = configure.CheckHeader("Opcodes/stk/include/Stk.h", language = "C++")
pdhfound = configure.CheckHeader("m_pd.h", language = "C")
tclhfound = configure.CheckHeader("tcl.h", language = "C")
luaFound = configure.CheckHeader("lua.h", language = "C")
swigFound = 'swig' in commonEnvironment['TOOLS']
print 'Checking for SWIG... %s' % (['no', 'yes'][int(swigFound)])
pythonFound = configure.CheckHeader("Python.h", language = "C")
if not pythonFound:
    for i in pythonIncludePath:
        tmp = '%s/Python.h' % i
        pythonFound = pythonFound or configure.CheckHeader(tmp, language = "C")
if getPlatform() != 'darwin':
    javaFound = configure.CheckHeader("jni.h", language = "C++")
else:
    javaFound = configure.CheckHeader("/System/Library/Frameworks/JavaVM.Framework/Headers/jni.h", language = "C++")

if getPlatform() == 'mingw':
    commonEnvironment['ENV']['PATH'] = os.environ['PATH']
    commonEnvironment['SYSTEMROOT'] = os.environ['SYSTEMROOT']

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
        commonEnvironment.Append(CCFLAGS = h[1])

if getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = '-DHAVE_DIRENT_H')
elif configure.CheckHeader("dirent.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_DIRENT_H')

if not (configure.CheckHeader("Opcodes/Loris/src/loris.h") and configure.CheckHeader("fftw3.h")):
    commonEnvironment["buildLoris"] = 0
    print "CONFIGURATION DECISION: Not building Loris Python extension and Csound opcodes."
else:
    print "CONFIGURATION DECISION: Building Loris Python extension and Csound opcodes."

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

    specificFiles = "SConstruct _CsoundVST.* _loris.* loris.py lorisgens.C lorisgens.h morphdemo.py trymorph.csd CsoundCOM.dll msvcp70.dll libsndfile.dll portaudio.dll.0.0.19 msvcr70.dll csound csound.exe CsoundVST CsoundVST.exe CsoundVST.* soundfonts.dll libpython23.a "
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

csoundLibraryEnvironment = commonEnvironment.Copy()
csoundLibraryEnvironment.Append(CCFLAGS = ['-D__BUILDING_LIBCSOUND'])
if commonEnvironment['buildRelease'] != '0':
    csoundLibraryEnvironment.Append(CCFLAGS = ['-D_CSOUND_RELEASE_'])
csoundDynamicLibraryEnvironment = csoundLibraryEnvironment.Copy()
csoundDynamicLibraryEnvironment.Append(LIBS = ['sndfile'])
if getPlatform() == 'mingw':
    if commonEnvironment['MSVC'] == '0':
        # These are the Windows system call libraries.
        csoundWindowsLibraries = Split('''
            kernel32 gdi32 wsock32 ole32 uuid winmm
        ''')
    else:
        # These are the Windows system call libraries.
        csoundWindowsLibraries = Split('''
            kernel32 gdi32 wsock32 ole32 uuid winmm user32.lib ws2_32.lib
            comctl32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib
            ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
            kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
            advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
            odbc32.lib odbccp32.lib
        ''')
if getPlatform() == 'mingw':
    csoundDynamicLibraryEnvironment.Append(LIBS = csoundWindowsLibraries)
    csoundDynamicLibraryEnvironment.Append(SHLINKFLAGS = ['-module'])
    csoundDynamicLibraryEnvironment['ENV']['PATH'] = os.environ['PATH']
elif getPlatform() == 'linux':
    csoundDynamicLibraryEnvironment.Append(LIBS = ['dl', 'm', 'pthread'])
csoundInterfacesEnvironment = csoundDynamicLibraryEnvironment.Copy()

libCsoundSources = Split('''
Engine/auxfd.c
Engine/cfgvar.c
Engine/entry1.c
Engine/entry2.c
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
OOps/dsputil.c
OOps/dumpf.c
OOps/fftlib.c
OOps/goto_ops.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/mxfft.c
OOps/oscils.c
OOps/pstream.c
OOps/pvadd.c
OOps/pvfileio.c
OOps/pvinterp.c
OOps/pvocext.c
OOps/pvread.c
OOps/pvsanal.c
OOps/random.c
OOps/schedule.c
OOps/sndinfUG.c
OOps/str_ops.c
OOps/ugens1.c
OOps/ugens2.c
OOps/ugens3.c
OOps/ugens4.c
OOps/ugens5.c
OOps/ugens6.c
OOps/ugens8.c
OOps/ugrw1.c
OOps/ugrw2.c
OOps/vbap.c
OOps/vbap_eight.c
OOps/vbap_four.c
OOps/vbap_sixteen.c
OOps/vbap_zak.c
OOps/vdelay.c
OOps/vpvoc.c
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
Top/scot.c
Top/threads.c
Top/utility.c
''')

csoundLibraryName = 'csound'
if getPlatform() == 'linux' and commonEnvironment['useDouble'] != '0':
    csoundLibraryName += '64'

if (commonEnvironment['dynamicCsoundLibrary'] == '1'):
    print 'CONFIGURATION DECISION: Building dynamic Csound library'
    if getPlatform() == 'linux':
        # library version is CS_VERSION.CS_APIVERSION
        Libname = 'lib' + csoundLibraryName + '.so'
        libName2 = libName + '.5.1'
        os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', libName])
        os.symlink(libName2, libName)
        tmp = csoundDynamicLibraryEnvironment['SHLINKFLAGS']
        tmp += ['-Wl,-soname=%s' % libName2]
        csoundLibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            libName2, libCsoundSources,
            SHLINKFLAGS = tmp, SHLIBPREFIX = '', SHLIBSUFFIX = '')
    else:
        if getPlatform() == 'darwin':
           csoundFrameworkEnvironment = csoundDynamicLibraryEnvironment.Copy();
           libName = 'CsoundLib'
           csoundFrameworkEnvironment.Append(SHLINKFLAGS=['-Xlinker', '-compatibility_version', '-Xlinker', '5.0.0'])
           csoundFrameworkEnvironment.Append(SHLINKFLAGS=['-Xlinker','-current_version','-Xlinker', '5.0.0'])        
           csoundFrameworkEnvironment.Append(SHLINKFLAGS = ['-install_name','/Library/Frameworks/CsoundLib.Framework/CsoundLib'])
           csoundLibrary = csoundFrameworkEnvironment.SharedLibrary(libName, libCsoundSources, SHLIBPREFIX = '', SHLIBSUFFIX = '')
           csoundFrameworkEnvironment.Command('CsoundLib_current', 'CsoundLib', "cd CsoundLib.Framework/Versions; ln -sf 5.0 Current") 
           csoundFrameworkEnvironment.Command('CsoundLib.Framework/Headers/csound.h', 'H/csound.h', "cd CsoundLib.Framework; ln -s Versions/5.0/Headers Headers; cd ..; cp H/*.h CsoundLib.Framework/Versions/5.0/Headers")
           csoundFrameworkEnvironment.Command('CsoundLib.Framework/Resources/opcodes/libstdopcod.dylib', 'libstdopcod.dylib', "cd CsoundLib.Framework; ln -s Versions/5.0/Resources Resources; cd ..;cp *.dylib CsoundLib.Framework/Versions/5.0/Resources/Opcodes/")
           csoundFrameworkEnvironment.Command('CsoundLib.Framework/Versions/5.0/CsoundLib', 'CsoundLib', "cp CsoundLib CsoundLib.Framework/Versions/5.0")
           csoundFrameworkEnvironment.Command('CsoundLib.Framework/CsoundLib', 'CsoundLib.Framework/Versions/5.0/CsoundLib', "cd CsoundLib.Framework; ln -sf Versions/5.0/CsoundLib  CsoundLib") 
           csoundFrameworkEnvironment.Command('/Library/Frameworks/CsoundLib.Framework/CsoundLib', 'CsoundLib', "cp -RL CsoundLib.Framework /Library/Frameworks") 
           
 

        else:
           csoundlibrary = csoundDynamicLibraryEnvironment.SharedLibrary(
            csoundLibraryName, libCsoundSources)
else:
    print 'CONFIGURATION DECISION: Building static Csound library'
    csoundLibrary = csoundLibraryEnvironment.Library(
        csoundLibraryName, libCsoundSources)
libs.append(csoundLibrary)
zipDependencies.append(csoundLibrary)

pluginEnvironment = commonEnvironment.Copy()
pluginEnvironment.Append(LIBS = ['sndfile'])

if getPlatform() == 'darwin':
    pluginEnvironment.Append(LINKFLAGS = Split('''
        -framework CoreMidi -framework CoreFoundation -framework CoreAudio
    '''))
    # pluginEnvironment.Append(LINKFLAGS = ['-dynamiclib'])
    pluginEnvironment['SHLIBSUFFIX'] = '.dylib'

csoundProgramEnvironment = commonEnvironment.Copy()
if getPlatform() == 'darwin' and csoundProgramEnvironment['dynamicCsoundLibrary'] == '1': 
  csoundProgramEnvironment.Append(LINKFLAGS = Split(''' -F. -framework CsoundLib -lsndfile'''))
else:
  csoundProgramEnvironment.Append(LIBS = [csoundLibraryName, 'sndfile'])


vstEnvironment = commonEnvironment.Copy()
fltkConfigFlags = 'fltk-config --use-images --cflags --cxxflags'
if getPlatform() != 'darwin':
    fltkConfigFlags += ' --ldflags'
if vstEnvironment.ParseConfig(fltkConfigFlags):
    print "Parsed fltk-config."
if getPlatform() == 'darwin':
    vstEnvironment.Append(LIBS = Split('''
        fltk fltk_images fltk_png z fltk_jpeg
    '''))
guiProgramEnvironment = commonEnvironment.Copy()

if getPlatform() == 'mingw':
    if commonEnvironment['MSVC'] == '0':
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

makedb = commonEnvironment.Program('makedb', ['strings/makedb.c'])
zipDependencies.append(makedb)

if (commonEnvironment['usePortMIDI']=='1' and portmidiFound):
    print 'CONFIGURATION DECISION: Building with PortMIDI.'
    portMidiEnvironment = pluginEnvironment.Copy()
    portMidiEnvironment.Append(LIBS = ['portmidi'])
    if getPlatform() != 'darwin':
        portMidiEnvironment.Append(LIBS = ['porttime'])
    if getPlatform() == 'mingw':
        portMidiEnvironment.Append(LIBS = ['winmm'])
    if getPlatform() == 'linux' and alsaFound:
        portMidiEnvironment.Append(LIBS = ['asound'])
    pluginLibraries.append(portMidiEnvironment.SharedLibrary('pmidi',
                                                             ['InOut/pmidi.c']))
else:
    print 'CONFIGURATION DECISION: Not building with PortMIDI.'

if not ((pythonFound or luaFound or javaFound) and swigFound and commonEnvironment['buildInterfaces'] == '1'):
    print 'CONFIGURATION DECISION: Not building Csound interfaces library.'
else:
    print 'CONFIGURATION DECISION: Building Csound interfaces library.'
    csoundInterfacesEnvironment.Append(CPPPATH = ['interfaces'])
    csoundInterfacesSources = Split('''
        interfaces/cs_glue.cpp
        interfaces/filebuilding.cpp
        interfaces/CppSound.cpp
        interfaces/CsoundFile.cpp
    ''')
    if commonEnvironment['dynamicCsoundLibrary'] == '1' and getPlatform() == 'darwin':
          csoundInterfacesEnvironment.Append(LINKFLAGS = Split(''' -F. -framework CsoundLib '''))
    elif commonEnvironment['dynamicCsoundLibrary'] == '1' or getPlatform() == 'mingw': 
        csoundInterfacesEnvironment.Prepend(LIBS = [csoundLibraryName])
    else:
        csoundInterfacesSources += libCsoundSources
    if getPlatform() == 'mingw':
        csoundInterfacesEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
    elif getPlatform() == 'linux':
        csoundInterfacesEnvironment.Prepend(LIBS = ['util'])
    if commonEnvironment['MSVC'] != '1':
        csoundInterfacesEnvironment.Prepend(LIBS = ['stdc++'])
    csoundInterfacesEnvironment.Append(SWIGFLAGS = Split('''
        -c++ -includeall -verbose
    '''))
    csoundWrapperEnvironment = csoundInterfacesEnvironment.Copy()
    wrapCFlags = csoundWrapperEnvironment['CCFLAGS']
    wrapCXXFlags = csoundWrapperEnvironment['CXXFLAGS']
    if '-pedantic' in wrapCFlags:
        wrapCFlags.remove('-pedantic')
    if '-pedantic' in wrapCXXFlags:
        wrapCXXFlags.remove('-pedantic')
    if commonEnvironment['MSVC'] == '0':
        # work around non-ANSI type punning in SWIG generated wrapper files
        wrapCFlags.append('-fno-strict-aliasing')
    wrapCFlags.append('-D__BUILDING_CSOUND_INTERFACES')
    csoundWrapperEnvironment['CCFLAGS'] = wrapCFlags
    csoundWrapperEnvironment['CXXFLAGS'] = wrapCXXFlags
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
    if pythonFound:
        if getPlatform() == 'mingw':
            pythonImportLibrary = csoundInterfacesEnvironment.Command(
                '/usr/local/lib/libpython23.a',
                '$SYSTEMROOT/System32/python23.dll',
                ['pexports $SYSTEMROOT/System32/python23.dll > python23.def',
                 'dlltool --input-def python23.def --dllname python23.dll --output-lib /usr/local/lib/libpython23.a'])
        csoundWrapperEnvironment.Append(CPPPATH = pythonIncludePath)
        csoundInterfacesEnvironment.Append(LINKFLAGS = pythonLinkFlags)
        csoundInterfacesEnvironment.Prepend(LIBPATH = pythonLibraryPath)
        csoundInterfacesEnvironment.Prepend(LIBS = pythonLibs)
        csoundPythonInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/python_interface.i',
            SWIGFLAGS = [swigflags, '-python', '-outdir', '.'])
        csoundInterfacesSources.insert(0, csoundPythonInterface)
    if javaFound and commonEnvironment['buildJavaWrapper'] == '1':
        print 'CONFIGURATION DECISION: Building Java wrappers for Csound interfaces library.'
        if getPlatform() == 'darwin':
            csoundInterfacesEnvironment.Append(LINKFLAGS =
                ['-framework', 'JavaVM'])
            csoundWrapperEnvironment.Append(CPPPATH =
                ['/System/Library/Frameworks/JavaVM.Framework/Headers'])
        csoundJavaInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/java_interface.i',
            SWIGFLAGS = [swigflags, '-java', '-package', 'csnd'])
        csoundInterfacesSources.append(csoundJavaInterface)
        jcsnd = csoundInterfacesEnvironment.Java(
            target = 'interfaces', source = 'interfaces',
            JAVACFLAGS = ['-source', '1.4', '-target', '1.4'])
        zipDependencies.append(jcsnd)
        jcsndJar = csoundInterfacesEnvironment.Jar(
            'csnd.jar', ['interfaces/csnd'], JARCHDIR = 'interfaces')
        Depends(jcsndJar, jcsnd)
    else:
        print 'CONFIGURATION DECISION: Not building Java wrappers for Csound interfaces library.'
    if not luaFound:
        print 'CONFIGURATION DECISION: Not building Csound Lua interface library.'
    else:
        print 'CONFIGURATION DECISION: Building Csound Lua interface library.'
        csoundLuaInterface = csoundWrapperEnvironment.SharedObject(
            'interfaces/lua_interface.i',
            SWIGFLAGS = [swigflags, '-lua', '-outdir', '.'])
        csoundInterfacesSources.insert(0, csoundLuaInterface)
        csoundInterfacesEnvironment.Prepend(LIBS = ['lua50'])
    if getPlatform() == 'darwin':
        csoundInterfacesEnvironment.Append(LINKFLAGS = ['-Wl'])
        csoundInterfacesEnvironment.Prepend(LINKFLAGS = ['-bundle'])
        csoundInterfaces = csoundInterfacesEnvironment.Program(
            'csnd', csoundInterfacesSources,
            PROGPREFIX = '_', PROGSUFFIX = '.so')
        csoundInterfacesEnvironment.Command('lib_csnd.jnilib', '_csnd.so', "ln -s _csnd.so lib_csnd.jnilib")
    else:
        if getPlatform() == 'linux':
            os.spawnvp(os.P_WAIT, 'rm', ['rm', '-f', 'lib_csnd.so'])
            os.symlink('_csnd.so', 'lib_csnd.so')
        csoundInterfacesEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
        csoundInterfaces = csoundInterfacesEnvironment.SharedLibrary(
            'csnd', csoundInterfacesSources, SHLIBPREFIX = '_')
    Depends(csoundInterfaces, csoundLibrary)
    libs.append(csoundInterfaces)
    zipDependencies.append(csoundInterfaces)

if commonEnvironment['generatePdf']=='0':
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

pluginLibraries.append(pluginEnvironment.SharedLibrary('stdopcod', Split('''
    Opcodes/ambicode.c      Opcodes/babo.c          Opcodes/bbcut.c
    Opcodes/biquad.c        Opcodes/butter.c        Opcodes/clfilt.c
    Opcodes/cross2.c        Opcodes/dam.c           Opcodes/dcblockr.c
    Opcodes/filter.c        Opcodes/flanger.c       Opcodes/follow.c
    Opcodes/fout.c          Opcodes/freeverb.c      Opcodes/ftconv.c
    Opcodes/ftgen.c         Opcodes/gab/gab.c       Opcodes/gab/vectorial.c
    Opcodes/grain.c         Opcodes/grain4.c        Opcodes/hrtferX.c
    Opcodes/ifd.c           Opcodes/locsig.c        Opcodes/lowpassr.c
    Opcodes/metro.c         Opcodes/midiops2.c      Opcodes/midiops3.c
    Opcodes/newfils.c       Opcodes/nlfilt.c        Opcodes/oscbnk.c
    Opcodes/partials.c      Opcodes/phisem.c        Opcodes/pluck.c
    Opcodes/psynth.c        Opcodes/pvsbasic.c      Opcodes/pvscent.c
    Opcodes/pvsdemix.c      Opcodes/repluck.c       Opcodes/reverbsc.c
    Opcodes/scansyn.c       Opcodes/scansynx.c      Opcodes/seqtime.c
    Opcodes/sndloop.c       Opcodes/sndwarp.c       Opcodes/space.c
    Opcodes/spat3d.c        Opcodes/syncgrain.c     Opcodes/ugens7.c
    Opcodes/ugens9.c        Opcodes/ugensa.c        Opcodes/uggab.c
    Opcodes/ugmoss.c        Opcodes/ugnorman.c      Opcodes/ugsc.c
    Opcodes/wave-terrain.c
    Opcodes/stdopcod.c
''')))

if getPlatform() == 'linux' or getPlatform() == 'darwin':
    pluginLibraries.append(pluginEnvironment.SharedLibrary('control',
        ['Opcodes/control.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ftest',
    ['Opcodes/ftest.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('mixer',
    ['Opcodes/mixer.cpp']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('modal4',
    ['Opcodes/modal4.c', 'Opcodes/physutil.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('physmod', Split('''
    Opcodes/physmod.c Opcodes/physutil.c Opcodes/mandolin.c Opcodes/singwave.c
    Opcodes/fm4op.c Opcodes/moog1.c Opcodes/shaker.c Opcodes/bowedbar.c
    ''')))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pitch',
    ['Opcodes/pitch.c', 'Opcodes/pitch0.c', 'Opcodes/spectra.c']))
sfontEnvironment = pluginEnvironment.Copy()
if (commonEnvironment['MSVC'] == '0'):
    sfontEnvironment.Append(CCFLAGS = ['-fno-strict-aliasing'])
pluginLibraries.append(sfontEnvironment.SharedLibrary('sfont',
    ['Opcodes/sfont.c']))

# Plugins with External Dependencies

# FLTK widgets

if not (commonEnvironment['useFLTK'] == '1' and fltkFound):
    print 'CONFIGURATION DECISION: Not building with FLTK graphs and widgets.'
else:
    print 'CONFIGURATION DECISION: Building with FLTK graphs and widgets.'
    widgetsEnvironment = pluginEnvironment.Copy()
    if (commonEnvironment['noFLTKThreads'] == '1'):
        widgetsEnvironment.Append(CCFLAGS = ['-DNO_FLTK_THREADS'])
    if getPlatform() == 'linux':
        widgetsEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags')
        widgetsEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'mingw':
        widgetsEnvironment.Append(LIBS = ['fltk'])
        if (commonEnvironment['MSVC'] == '0'):
            widgetsEnvironment.Append(LIBS = ['stdc++', 'supc++'])
        widgetsEnvironment.Append(LIBS = csoundWindowsLibraries)
    elif getPlatform() == 'darwin':
        widgetsEnvironment.Append(LIBS = ['fltk', 'stdc++', 'pthread', 'm'])
        widgetsEnvironment.Append(LINKFLAGS = Split('''
            -framework Carbon -framework CoreAudio -framework CoreMidi
            -framework ApplicationServices
        '''))
    pluginLibraries.append(widgetsEnvironment.SharedLibrary('widgets',
        ['InOut/FL_graph.cpp', 'InOut/winFLTK.c', 'InOut/widgets.cpp']))

# REAL TIME AUDIO

if (commonEnvironment['useCoreAudio']=='1' and getPlatform() == 'darwin'):
    print "CONFIGURATION DECISION: Building CoreAudio plugin."
    coreaudioEnvironment = pluginEnvironment.Copy()
    coreaudioEnvironment.Append(CCFLAGS = ['-I/system/library/Frameworks/CoreAudio.framework/Headers'])
    pluginLibraries.append(coreaudioEnvironment.SharedLibrary('rtcoreaudio',
                                                        ['InOut/rtcoreaudio.c']))
else:
    print "CONFIGURATION DECISION: Not building CoreAudio plugin."

if not (commonEnvironment['useALSA']=='1' and alsaFound):
    print "CONFIGURATION DECISION: Not building ALSA plugin."
else:
    print "CONFIGURATION DECISION: Building ALSA plugin."
    alsaEnvironment = pluginEnvironment.Copy()
    alsaEnvironment.Append(LIBS = ['asound', 'pthread'])
    pluginLibraries.append(alsaEnvironment.SharedLibrary('rtalsa',
                                                         ['InOut/rtalsa.c']))

if getPlatform() == 'mingw':
    winmmEnvironment = pluginEnvironment.Copy()
    winmmEnvironment.Append(LIBS = ['winmm', 'gdi32', 'kernel32'])
    pluginLibraries.append(winmmEnvironment.SharedLibrary('rtwinmm',
                                                          ['InOut/rtwinmm.c']))

if not (commonEnvironment['usePortAudio']=='1' and portaudioFound):
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
    elif getPlatform() == 'mingw':
        portaudioEnvironment.Append(LIBS = ['winmm', 'dsound'])
        portaudioEnvironment.Append(LIBS = csoundWindowsLibraries)
    pluginLibraries.append(portaudioEnvironment.SharedLibrary('rtpa',
                                                              ['InOut/rtpa.c']))

if not (commonEnvironment['useJack']=='1' and jackFound):
    print "CONFIGURATION DECISION: Not building JACK plugin."
else:
    print "CONFIGURATION DECISION: Building JACK plugin."
    jackEnvironment = pluginEnvironment.Copy()
    if getPlatform() == 'linux':
        jackEnvironment.Append(LIBS = ['jack', 'asound', 'pthread'])
    else:
        jackEnvironment.Append(LIBS = ['jack', 'pthread'])
    pluginLibraries.append(jackEnvironment.SharedLibrary('rtjack',
                                                         ['InOut/rtjack.c']))

if not (commonEnvironment['useOSC'] == '1' and oscFound):
    print "CONFIGURATION DECISION: Not building OSC plugin."
else:
    print "CONFIGURATION DECISION: Building OSC plugin."
    oscEnvironment = pluginEnvironment.Copy()
    oscEnvironment.Append(LIBS = ['lo'])
    oscEnvironment.Append(LIBS = ['pthread'])
    if getPlatform() == 'mingw':
        oscEnvironment.Append(LIBS = ['ws2_32'])
    pluginLibraries.append(oscEnvironment.SharedLibrary('osc',
                                                        ['Opcodes/OSC.c']))

# FLUIDSYNTH OPCODES

if not configure.CheckHeader("fluidsynth.h", language = "C"):
    print "CONFIGURATION DECISION: Not building fluid opcodes."
else:
    print "CONFIGURATION DECISION: Building fluid opcodes."
    if getPlatform() == 'linux':
        fluidEnvironment = pluginEnvironment.Copy()
        fluidEnvironment.Append(LIBS = ['fluidsynth'])
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluidOpcodes',
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp']))
    if getPlatform() == 'mingw':
        fluidEnvironment = pluginEnvironment.Copy()
        fluidEnvironment.Append(LIBS = ['fluidsynth', 'stdc++'])
        fluidEnvironment.Append(LIBS = ['winmm', 'dsound'])
        fluidEnvironment.Append(CCFLAGS = ['-DFLUIDSYNTH_NOT_A_DLL', '-DMAKEDLL', '-DBUILDING_DLL'])
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluidOpcodes',
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp']))

# VST HOST OPCODES

if getPlatform() == 'mingw' and fltkFound:
    vst4Environment = vstEnvironment.Copy()
    vst4Environment.Append(LIBS = ['fltk'])
    if (commonEnvironment['MSVC'] == '0'):
        vst4Environment.Append(LIBS = ['stdc++'])
        if (getPlatform() == 'mingw'):
            vst4Environment.Append(LIBS = csoundWindowsLibraries)
    else:
        vst4Environment.Append(LIBS = csoundWindowsLibraries)
    vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
    zipDependencies.append(vst4Environment.SharedLibrary('vst4cs', Split('''
        Opcodes/vst4cs/src/vst4cs.cpp Opcodes/vst4cs/src/fxbank.cpp
        Opcodes/vst4cs/src/vsthost.cpp
    ''')))

# Utility programs.

utilPlugins = [
    ['cvanal',      'util/cvanal.c'     ],
    ['dnoise',      'util/dnoise.c'     ],
    ['envext',      'util/envext.c'     ],
    ['extractor',   'util/xtrct.c'      ],
    ['het_export',  'util/het_export.c' ],
    ['het_import',  'util/het_import.c' ],
    ['hetro',       'util/hetro.c'      ],
    ['lpanal',      'util/lpanal.c'     ],
    ['lpc_export',  'util/lpc_export.c' ],
    ['lpc_import',  'util/lpc_import.c' ],
    ['mixer_util',  'util/mixer.c'      ],
    ['pvanal',      'util/pvanal.c'     ],
    ['pvlook',      'util/pvlook.c'     ],
    ['scale',       'util/scale.c'      ],
    ['sndinfo',     'util/sndinfo.c'    ],
    ['srconv',      'util/srconv.c'     ]]

for i in utilPlugins:
    pluginLibraries.append(pluginEnvironment.SharedLibrary(i[0], i[1]))

if (commonEnvironment['buildUtilities'] != '0'):
    utils = [
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

#executables.append(csoundProgramEnvironment.Program('cscore',
#    ['util1/cscore/cscore_main.c']))
#executables.append(csoundProgramEnvironment.Program('pv_export',
#    ['util2/exports/pv_export.c']))
#executables.append(csoundProgramEnvironment.Program('pv_import',
#    ['util2/exports/pv_import.c']))
#executables.append(csoundProgramEnvironment.Program('scot',
#    ['util1/scot/scot_main.c']))
#executables.append(csoundProgramEnvironment.Program('sdif2ad',
#    ['SDIF/sdif2adsyn.c', 'SDIF/sdif.c', 'SDIF/sdif-mem.c']))

# Front ends.

csoundProgramSources = ['frontends/csound/csound_main.c']
if getPlatform() == 'linux':
    csoundProgramSources = ['frontends/csound/sched.c'] + csoundProgramSources
csoundProgram = csoundProgramEnvironment.Program('csound', csoundProgramSources)
executables.append(csoundProgram)
Depends(csoundProgram, csoundLibrary)

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
    vstEnvironment.Prepend(LIBS = [csoundLibraryName, 'sndfile', '_csnd'])
    vstEnvironment.Append(SWIGFLAGS = Split('-c++ -includeall -verbose -outdir .'))
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['util', 'dl', 'm'])
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(LINKFLAGS = ['-Wl,-rpath-link,.'])
        guiProgramEnvironment.Prepend(LINKFLAGS = Split('''
            -Wl,-rpath-link,. _CsoundVST.so
        '''))
    elif getPlatform() == 'darwin':
        vstEnvironment.Append(LIBS = ['dl', 'm'])
        # vstEnvironment.Append(CXXFLAGS = ['-fabi-version=0']) # if gcc3.2-3
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        vstEnvironment['SHLIBSUFFIX'] = '.dylib'
        guiProgramEnvironment.Prepend(LINKFLAGS = ['_CsoundVST.dylib'])
    elif getPlatform() == 'mingw':
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        vstEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
        vstEnvironment.Append(CCFLAGS = ['-DNDEBUG'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
        vstEnvironment.Append(LIBS = ['fltk_images'])
        vstEnvironment.Append(LIBS = ['fltk'])
        guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    for option in vstEnvironment['CCFLAGS']:
            if string.find(option, '-D') == 0:
                vstEnvironment.Append(SWIGFLAGS = [option])
    for option in vstEnvironment['CPPPATH']:
        option = '-I' + option
        vstEnvironment.Append(SWIGFLAGS = [option])
    #for option in vstEnvironment['CPPFLAGS']:
    #        if string.find(option, '-D') == 0:
    #                vstEnvironment.Append(SWIGFLAGS = [option])
    print 'PATH =', commonEnvironment['ENV']['PATH']
    csoundVstSources = Split('''
    frontends/CsoundVST/AudioEffect.cpp
    frontends/CsoundVST/audioeffectx.cpp
    frontends/CsoundVST/Composition.cpp
    frontends/CsoundVST/Conversions.cpp
    frontends/CsoundVST/Counterpoint.cpp
    frontends/CsoundVST/CounterpointNode.cpp
    frontends/CsoundVST/Cell.cpp
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
    frontends/CsoundVST/Shell.cpp
    frontends/CsoundVST/StrangeAttractor.cpp
    frontends/CsoundVST/System.cpp
    frontends/CsoundVST/Soundfile.cpp
    ''')
    # These are the Windows system call libraries.
    if getPlatform() == 'mingw':
        vstEnvironment.Append(LIBS = csoundWindowsLibraries)
        vstEnvironment.Append(SHLINKFLAGS = ['-module'])
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        csoundVstSources.append('frontends/CsoundVST/_CsoundVST.def')
    swigflags = vstEnvironment['SWIGFLAGS']
    csoundVstPythonWrapper = vstEnvironment.SharedObject('frontends/CsoundVST/CsoundVST.i', SWIGFLAGS = [swigflags, '-python'])
    csoundVstSources.insert(0, csoundVstPythonWrapper)
    csoundvst =  vstEnvironment.SharedLibrary('CsoundVST', csoundVstSources, SHLIBPREFIX = '_')

    if(getPlatform == 'darwin'):
        vstEnvironment.Prepend(LINKFLAGS='-bundle')
        vstEnvironment.Program('CsoundVST.so', csoundVstSources, PROGPREFIX = '_')
    #Depends(csoundvst, 'frontends/CsoundVST/CsoundVST_wrap.cc')
    zipDependencies.append(csoundvst)
    libs.append('CsoundVST.py')
    libs.append(csoundvst)
    Depends(csoundvst, csoundInterfaces)
    if getPlatform() == 'mingw':
        guiProgramEnvironment.Append(LIBS = ['CsoundVST'])

    csoundvstGui = guiProgramEnvironment.Program('CsoundVST', ['frontends/CsoundVST/csoundvst_main.cpp'])
    executables.append(csoundvstGui)
    zipDependencies.append(csoundvstGui)
    Depends(csoundvstGui, csoundvst)

    counterpoint = vstEnvironment.Program('counterpoint', ['frontends/CsoundVST/CounterpointMain.cpp' ])
    zipDependencies.append(counterpoint)

    # Build the Loris and Python opcodes here because they depend
    # on the same things as CsoundVST.

    if commonEnvironment['buildLoris']=='1':
        # For Loris, we build only the loris Python extension module and
        # the Csound opcodes (modified for Csound 5).
        # It is assumed that you have copied all contents of the Loris distribution
        # into the csound5/Opcodes/Loris directory, e.g.
        # csound5/Opcodes/Loris/src/*, etc.
        lorisEnvironment = vstEnvironment.Copy();
        lorisEnvironment.Append(CCFLAGS = '-DHAVE_FFTW3_H -DDEBUG_LORISGENS -D_MSC_VER')
        lorisEnvironment.Append(CPPPATH = Split('Opcodes/Loris Opcodes/Loris/src ./'))
        lorisEnvironment.Append(LIBS = ['fftw3'])
        lorisSources = glob.glob('Opcodes/Loris/src/*.C')
        lorisSources.append('Opcodes/Loris/scripting/loris.i')
        # The following file has been patched for Csound 5 and you should update it from Csound 5 CVS.
        lorisSources.append('Opcodes/Loris/lorisgens5.C')
        lorisEnvironment.Append(SWIGPATH = ['./'])
        lorisEnvironment.Prepend(SWIGFLAGS = Split('-module loris -c++ -python -DHAVE_FFTW3_H -I./Opcodes/Loris/src -I.'))
        loris = lorisEnvironment.SharedLibrary('loris', lorisSources, SHLIBPREFIX = '_')
        Depends(loris, csoundvst)
        pluginLibraries.append(loris)
        libs.append(loris)
        libs.append('loris.py')

    if not (commonEnvironment['buildStkOpcodes'] == '1' and stkFound):
        print 'CONFIGURATION DECISION: Not building STK opcodes.'
    else:
        print 'CONFIGURATION DECISION: Building STK opcodes.'
        # For the STK opcodes, the STK distribution include, src, and rawwaves directories should be copied thusly:
        # csound5/Opcodes/stk/include
        # csound5/Opcodes/stk/src
        # csound5/Opcodes/stk/rawwaves
        # Then, the following sources (and any other future I/O or OS dependent sources) should be ignored:
        removeSources = Split('''
Opcodes/stk/src/InetWvIn.cpp
Opcodes/stk/src/InetWvOut.cpp
Opcodes/stk/src/Mutex.cpp
Opcodes/stk/src/RtAudio.cpp
Opcodes/stk/src/RtMidi.cpp
Opcodes/stk/src/RtDuplex.cpp
Opcodes/stk/src/RtWvIn.cpp
Opcodes/stk/src/RtWvOut.cpp
Opcodes/stk/src/Socket.cpp
Opcodes/stk/src/TcpClient.cpp
Opcodes/stk/src/TcpServer.cpp
Opcodes/stk/src/Thread.cpp
Opcodes/stk/src/UdpSocket.cpp
''')
        stkEnvironment = vstEnvironment.Copy()
        if getPlatform() == 'mingw':
                stkEnvironment.Append(CCFLAGS = '-D__OS_WINDOWS__ -D__LITTLE_ENDIAN__')
        elif getPlatform() == 'linux':
                stkEnvironment.Append(CCFLAGS = '-D__OS_LINUX__ -D__LITTLE_ENDIAN__')
        elif getPlatform() == 'darwin':
                stkEnvironment.Append(CCFLAGS = '-D__OS_MACOSX__ -D__BIG_ENDIAN__')
        stkEnvironment.Prepend(CPPPATH = Split('Opcodes/stk/include Opcodes/stk/src ./ ./../include'))
        stkSources_ = glob.glob('Opcodes/stk/src/*.cpp')
        # This is the one that actually defines the opcodes. They are straight wrappers, as simple as possible.
        stkSources_.append('Opcodes/stk/stkOpcodes.cpp')
        stkSources = []
        for source in stkSources_:
                stkSources.append(source.replace('\\', '/'))
        for removeMe in removeSources:
            stkSources.remove(removeMe)
        stk = stkEnvironment.SharedLibrary('stk', stkSources)
        Depends(stk, csoundLibrary)
        pluginLibraries.append(stk)
        libs.append(stk)

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
    elif getPlatform() == 'mingw':
        pyEnvironment['ENV']['PATH'] = os.environ['PATH']
        pyEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
    py = pyEnvironment.SharedLibrary('py', ['Opcodes/py/pythonopcodes.c'])
    pluginLibraries.append(py)

if commonEnvironment['buildPDClass']=='1' and pdhfound:
    print "CONFIGURATION DECISION: Building PD csoundapi~ class"
    pdClassEnvironment = commonEnvironment.Copy()
    if getPlatform() == 'darwin' and csoundProgramEnvironment['dynamicCsoundLibrary'] == '1': 
      pdClassEnvironment.Append(LINKFLAGS = Split(''' -F. -framework CsoundLib -lsndfile'''))
    else:
      pdClassEnvironment.Append(LIBS = [csoundLibraryName, 'sndfile'])
    if getPlatform() == 'darwin':
        pdClassEnvironment.Append(LINKFLAGS = Split('''
            -bundle -flat_namespace -undefined suppress
            -framework Carbon -framework ApplicationServices
        '''))
        pdClass = pdClassEnvironment.Program('csoundapi~.pd_darwin', 'frontends/csoundapi_tilde/csoundapi_tilde.c')
    elif getPlatform() == 'linux':
        pdClassEnvironment.Append(LINKFLAGS = ['-shared'])
        pdClass = pdClassEnvironment.Program('csoundapi~.pd_linux', 'frontends/csoundapi_tilde/csoundapi_tilde.c')
    elif getPlatform() == 'mingw':
        pdClassEnvironment.Append(LINKFLAGS = ['-shared'])
        pdClass = pdClassEnvironment.SharedLibrary('csoundapi~', 'frontends/csoundapi_tilde/csoundapi_tilde.c', SHLIBPREFIX = '')
        pdClassEnvironment.Append(LIBS = ['pd'])
        pdClassEnvironment.Append(LIBS = csoundWindowsLibraries)
        pdClassEnvironment.Append(SHLINKFLAGS = ['-module'])
        pdClassEnvironment['ENV']['PATH'] = os.environ['PATH']
    Depends(pdClass, csoundLibrary)
    zipDependencies.append(pdClass)
    libs.append(pdClass)

if commonEnvironment['buildTclcsound'] == '1' and tclhfound:
    print "CONFIGURATION DECISION: Building Tclcsound frontend"
    csTclEnvironment = commonEnvironment.Copy()
    if getPlatform() == 'darwin' and csoundProgramEnvironment['dynamicCsoundLibrary'] == '1': 
       csTclEnvironment.Append(LINKFLAGS = Split(''' -F. -framework CsoundLib -lsndfile'''))
    else:
       csTclEnvironment.Append(LIBS = [csoundLibraryName, 'sndfile'])
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
        csTclEnvironment.Append(LIBS = ['tcl8.4', 'tk8.4', 'dl', 'pthread'])
    elif getPlatform() == 'mingw':
        if commonEnvironment['MSVC'] == '1':
            csTclEnvironment.Append(LIBS = ['tcl84', 'tk84'])
        else:
            csTclEnvironment.Append(LIBS = ['tcl', 'tk'])
        csTclEnvironment.Append(LIBS = csoundWindowsLibraries)
    csTkEnvironment = csTclEnvironment.Copy()
    csTclEnvironment.Append(CCFLAGS = ['-DTCLSH'])
    csTkEnvironment.Append(CCFLAGS = ['-DWISH'])
    csTclEnvironment.Object('frontends/tclcsound/main_tclsh.o',
                            'frontends/tclcsound/main.c')
    csTclEnvironment.Object('frontends/tclcsound/commands.o',
                            'frontends/tclcsound/commands.c')
    csTcl = csTclEnvironment.Program('cstclsh', Split('''
        frontends/tclcsound/main_tclsh.o frontends/tclcsound/commands.o
    '''))
    csTkEnvironment.Object('frontends/tclcsound/main_wish.o',
                           'frontends/tclcsound/main.c')
    csTk = csTkEnvironment.Program('cswish', Split('''
        frontends/tclcsound/main_wish.o frontends/tclcsound/commands.o
    '''))
    csTkEnvironment.Prepend(LFLAGS = '-DTCL_USE_STUBS')
    Tclcsoundlib = csTkEnvironment.SharedLibrary('tclcsound', ['frontends/tclcsound/tclcsound.c', 'frontends/tclcsound/commands.c'], SHLIBPREFIX='')
    if getPlatform() == 'darwin':
        csTkEnvironment.Command('cswish_resources', 'cswish', "/Developer/Tools/Rez -i APPL -o cswish frontends/tclcsound/cswish.r")
    Depends(csTcl, csoundLibrary)
    Depends(csTk, csoundLibrary)
    Depends(Tclcsoundlib, csoundLibrary)
    zipDependencies.append(csTcl)
    zipDependencies.append(csTk)
    zipDependencies.append(Tclcsoundlib)
else:
    print "CONFIGURATION DECISION: Not building Tclcsound"

if (commonEnvironment['buildDSSI'] == '1') and (getPlatform() == 'linux'):
    print "CONFIGURATION DECISION: Building DSSI plugin host opcodes."
    dssiEnvironment = pluginEnvironment.Copy()
    dssiEnvironment.Append(LIBS = ['dl'])
    pluginLibraries.append(dssiEnvironment.SharedLibrary('dssi4cs',
        ['Opcodes/dssi4cs/src/load.c', 'Opcodes/dssi4cs/src/dssi4cs.c']))
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

if commonEnvironment['generateXmg']=='1':
    print "CONFIGURATION DECISION: Calling makedb"
    if getPlatform() == 'mingw':
        xmgs = commonEnvironment.Command('American.xmg', ['strings/all_strings'], 'makedb strings/all_strings American')
        xmgs1 = commonEnvironment.Command('English.xmg', ['strings/english-strings'], 'makedb strings/english-strings English')
        xmgs2 = commonEnvironment.Command('csound.xmg', ['strings/english-strings'], 'makedb strings/english-strings csound')
    else:
        xmgs = commonEnvironment.Command('American.xmg', ['strings/all_strings'], './makedb strings/all_strings American')
        xmgs1 = commonEnvironment.Command('English.xmg', ['strings/english-strings'], './makedb strings/english-strings English')
        xmgs2 = commonEnvironment.Command('csound.xmg', ['strings/english-strings'], './makedb strings/english-strings csound')
    Depends(xmgs, makedb)
    zipDependencies.append(xmgs)
    Depends(xmgs1, makedb)
    zipDependencies.append(xmgs1)
    Depends(xmgs2, makedb)
    zipDependencies.append(xmgs2)
else:
    print "CONFIGURATION DECISION: Not calling makedb"

zipDependencies += executables
zipDependencies += pluginLibraries

if (commonEnvironment['generateZip']=='0'):
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
else:
    LIB_DIR = PREFIX + "/lib"

if (commonEnvironment['useDouble'] == '0'):
    PLUGIN_DIR = LIB_DIR + "/csound/plugins"
else:
    PLUGIN_DIR = LIB_DIR + "/csound/plugins64"

if commonEnvironment['install']=='1':
    installExecutables = Alias('install-executables',
        Install(BIN_DIR, executables))

    installOpcodes = Alias('install-opcodes',
        Install(PLUGIN_DIR, pluginLibraries))

    installHeaders = Alias('install-headers',
        Install(INCLUDE_DIR, headers))

    installLibs = Alias('install-libs',
        Install(LIB_DIR, libs))

    Alias('install', [installExecutables, installOpcodes, installLibs, installHeaders])

if (getPlatform() == 'darwin' and commonEnvironment['useFLTK'] == '1'):
    print "CONFIGURATION DECISION: Adding resource fork for csound"
    commonEnvironment.Command('resources', 'csound', "/Developer/Tools/Rez -i APPL -o $SOURCE cs5.r")

