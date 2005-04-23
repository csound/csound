print '''
C S O U N D   5

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
For Cygwin, run in the Cygwin shell
    and use Cygwin Python to run 'scons'.
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
headers = glob.glob('H/*.h')
libs = []

def today():
    return time.strftime("%Y-%m-%d",time.localtime())

def getPlatform():
    if sys.platform[:5] == 'linux':
        return 'linux'
    elif sys.platform == 'cygwin':
        return 'cygwin'
    elif sys.platform[:3] == 'win' and sys.platform != 'cygwin':
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
opts.Add('useOldPortAudioPlugin',
    'Set to 1 to use old PortAudio plugin (rtpa.c and pa_blocking.c).',
    '0')
opts.Add('useALSA',
    'Set to 1 to use ALSA for real-time audio input and output.',
    '1')
opts.Add('useJack',
    'Set to 1 if you compiled PortAudio to use Jack',
    '1')
opts.Add('useFLTK',
    'Set to 1 to use FLTK for graphs and widget opcodes.',
    '1')
opts.Add('buildCsoundVST',
    'Set to 1 to build CsoundVST (needs FLTK, boost, Python 2.3, SWIG).',
    '1')
opts.Add('buildJavaWrapper',
    'Set to 1 to build Java wrapper for CsoundVST (needs CsoundVST).',
    '1')
opts.Add('noCygwin',
    'Set to 1 to build with -mno-cygwin when using Cygwin',
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
opts.Add('prefix',
    'Base directory for installs.  Defaults to /usr/local.',
    '/usr/local')
opts.Add('usePortMIDI',
    'Use portmidi library rather than internal MIDI (experimental).',
    '1')
opts.Add('noDebug',
    'Build without debugging information.',
    '0')
opts.Add('gcc3opt',
    'Enable gcc 3.3.x or later optimizations for i686.',
    '0')
opts.Add('arch',
    'Architecture type to use with gcc optimzation flag (-march=arch). Requires gcc3opt to be enabled',
    'i686')
opts.Add('cpu',
    'CPU type to use with gcc optimization flag (-mcpu=cpu). Requires gcc3opt to be enabled',
    'pentium3')
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

commonEnvironment.Prepend(CPPPATH  = ['.', './H'])
if (commonEnvironment['gcc3opt']=='1'):
  flags = '-DCSOUND_WITH_API -O3 -march=%s -mcpu=%s -fomit-frame-pointer -ffast-math -falign-functions=16'
  flags = flags%(commonEnvironment['arch'], commonEnvironment['cpu'])
  commonEnvironment.Prepend(CCFLAGS = Split(flags))
elif (commonEnvironment['noDebug']=='0'):
  commonEnvironment.Prepend(CCFLAGS = Split('-DCSOUND_WITH_API -g -gstabs -O2'))
else:
  commonEnvironment.Prepend(CCFLAGS = Split('-DCSOUND_WITH_API -O2'))
if (commonEnvironment['useGprof']=='1'):
  commonEnvironment.Append(CCFLAGS = ['-pg'])
  commonEnvironment.Append(LINKFLAGS = ['-pg'])
commonEnvironment.Prepend(CXXFLAGS = Split('-DCSOUND_WITH_API -fexceptions'))
commonEnvironment.Prepend(LIBPATH = ['.', '#.'])
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

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CPPPATH = '/usr/X11R6/include')
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
elif getPlatform() == 'mingw' or getPlatform() == 'cygwin':
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-D_WIN32")
    commonEnvironment.Append(CCFLAGS = "-DWIN32")
    commonEnvironment.Append(CCFLAGS = "-DHAVE_STRING_H")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(CCFLAGS = "-DOS_IS_WIN32")
    commonEnvironment.Append(CCFLAGS = "-mthreads")
    #commonEnvironment.Append(CCFLAGS = "-mtune=pentium4")

if (getPlatform() == 'linux'):
    commonEnvironment.Append(LINKFLAGS = Split('-Wl,-Bdynamic'))

# Adding libraries and flags if using -mno-cygwin with cygwin

if (commonEnvironment['noCygwin'] == '1' and getPlatform() == 'cygwin'):
    print 'CONFIGURATION DECISION: Using -mno-cygwin.'
    commonEnvironment.Prepend(CCFLAGS = ['-mno-cygwin'])
    commonEnvironment.Prepend(CPPFLAGS = ['-mno-cygwin'])
    commonEnvironment.Append(LIBS = ['m'])

# Check for prerequisites.
# We check only for headers;
# checking for libs may fail even if they are present,
# due to secondary dependencies.
# Python is assumed to be present because scons requires it.

configure = commonEnvironment.Configure()
sndfileFound = configure.CheckHeader("sndfile.h", language = "C")

if not sndfileFound:
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

if getPlatform() == 'mingw':
    commonEnvironment['ENV']['PATH'] = os.environ['PATH']
    commonEnvironment['SYSTEMROOT'] = os.environ['SYSTEMROOT']

# Define macros that configure and config.h used to define.

if configure.CheckHeader("io.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_IO_H')
if configure.CheckHeader("fcntl.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_FCNTL_H')
if configure.CheckHeader("unistd.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_UNISTD_H')
if configure.CheckHeader("stdint.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_STDINT_H')
if configure.CheckHeader("malloc.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_MALLOC_H')
if configure.CheckHeader("os.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_OS_H')
if configure.CheckHeader("sys/ioctl.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_IOCTL_H')
if configure.CheckHeader("sys/time.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_TIME_H')
if configure.CheckHeader("sys/types.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_TYPES_H')
if configure.CheckHeader("ctype.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_CTYPE_H')
if configure.CheckHeader("termios.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_TERMIOS_H')
if configure.CheckHeader("string.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_STRING_H')
elif configure.CheckHeader("strings.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_STRINGS_H')
if getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = '-DHAVE_DIRENT_H')
elif configure.CheckHeader("dirent.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_DIRENT_H')
if not (configure.CheckHeader("Opcodes/Loris/src/loris.h") and configure.CheckHeader("fftw3.h")):
    commonEnvironment["buildLoris"] = 0
    print "CONFIGURATION DECISION: Not building Loris Python extension and Csound opcodes."
else:
    print "CONFIGURATION DECISION: Building Loris Python extension and Csound opcodes."
if configure.CheckHeader("jni.h", language = "C++"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_JNI_H')

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
            print "Stripped",filename
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

pluginEnvironment = commonEnvironment.Copy()
pluginEnvironment.Append(LIBS = ['sndfile'])

if getPlatform() == 'darwin':
    pluginEnvironment.Append(LINKFLAGS = ['-dynamiclib'])
    pluginEnvironment['SHLIBSUFFIX'] = '.dylib'

csoundProgramEnvironment = commonEnvironment.Copy()
csoundProgramEnvironment.Append(LIBS = ['csound', 'sndfile'])
csoundProgramEnvironment.ParseConfig('fltk-config --cflags --cxxflags --ldflags')

vstEnvironment = commonEnvironment.Copy()
if vstEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags'):
    print "Parsed fltk-config."

guiProgramEnvironment = commonEnvironment.Copy()

if commonEnvironment['useALSA']=='1' and alsaFound:
    guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
elif commonEnvironment['usePortAudio']=='1' and portaudioFound:
    guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')

if getPlatform() == 'linux':
    csoundProgramEnvironment.Append(LIBS = ['dl', 'pthread'])
    vstEnvironment.Append(LIBS = ['dl'])
    guiProgramEnvironment.Append(LIBS = ['dl'])

if (commonEnvironment['useFLTK'] == '1' and fltkFound):
    csoundLibraryEnvironment.Append(CCFLAGS = '-DWINDOWS')
    pluginEnvironment.Append(CCFLAGS = '-DWINDOWS')
    csoundProgramEnvironment.Append(CCFLAGS = '-DWINDOWS')
    vstEnvironment.Append(CCFLAGS = '-DWINDOWS')
    guiProgramEnvironment.Append(CCFLAGS = '-DWINDOWS')
    csoundLibraryEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    pluginEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    csoundProgramEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    vstEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    guiProgramEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    csoundProgramEnvironment.Append(LIBS = ['fltk'])
    if getPlatform() == 'linux' or getPlatform() == 'cygwin':
        csoundProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'mingw':
        vstEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        guiProgramEnvironment.Append(LINKFLAGS = "--subsystem:windows")
        csoundProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
        vstEnvironment.Append(LIBS = ['stdc++', 'supc++'])
        guiProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
    elif getPlatform() == 'darwin':
        csoundProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
        csoundProgramEnvironment.Append(LINKFLAGS = ['-framework', 'Carbon'])
        csoundProgramEnvironment.Append(LINKFLAGS = ['-framework', 'CoreAudio'])
        csoundProgramEnvironment.Append(LINKFLAGS = ['-framework', 'CoreMidi'])

if getPlatform() == 'mingw':
    # These are the Windows system call libraries.
    csoundProgramEnvironment.Append(LIBS = ['kernel32'])
    csoundProgramEnvironment.Append(LIBS = ['gdi32'])
    csoundProgramEnvironment.Append(LIBS = ['wsock32'])
    csoundProgramEnvironment.Append(LIBS = ['ole32'])
    csoundProgramEnvironment.Append(LIBS = ['uuid'])
    csoundProgramEnvironment.Append(LIBS = ['winmm'])

    vstEnvironment.Append(LIBS = ['kernel32'])
    vstEnvironment.Append(LIBS = ['gdi32'])
    vstEnvironment.Append(LIBS = ['wsock32'])
    vstEnvironment.Append(LIBS = ['ole32'])
    vstEnvironment.Append(LIBS = ['uuid'])
    vstEnvironment.Append(LIBS = ['winmm'])

#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

makedb = commonEnvironment.Program('makedb', ['strings/makedb.c'])
zipDependencies.append(makedb)

libCsoundSources = Split('''
Engine/auxfd.c
Engine/cfgvar.c
Engine/entry1.c
Engine/entry2.c
Engine/envvar.c
Engine/express.c
Engine/extract.c
Engine/fgens.c
Engine/filopen.c
Engine/insert.c
Engine/linevent.c
Engine/memalloc.c
Engine/memfiles.c
Engine/musmon.c
Engine/namedins.c
Engine/oload.c
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
OOps/cmath.c
OOps/control.c
OOps/diskin.c
OOps/diskin2.c
OOps/disprep.c
OOps/dsputil.c
OOps/dumpf.c
OOps/fftlib.c
OOps/fout.c
OOps/fprint.c
OOps/lptrkfns.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/mxfft.c
OOps/oscils.c
OOps/pstream.c
OOps/pvadd.c
OOps/pvfileio.c
OOps/pvinterp.c
OOps/pvoc.c
OOps/pvocext.c
OOps/pvread.c
OOps/pvsanal.c
OOps/pvxanal.c
OOps/schedule.c
OOps/sdif.c
OOps/sdif-mem.c
OOps/sndinfUG.c
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
Top/dl_opcodes.c
Top/getstring.c
Top/main.c
Top/natben.c
Top/new_opts.c
Top/one_file.c
Top/opcode.c
Top/scot.c
Top/threads.c
Top/utility.c
''')

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
    print 'CONFIGURATION DECISION: Building without real time MIDI support.'

if not ((commonEnvironment['useFLTK'] == '1' and fltkFound)):
    print 'CONFIGURATION DECISION: Not building with FLTK for graphs and widgets.'
else:
    print 'CONFIGURATION DECISION: Building with FLTK for graphs and widgets.'
    libCsoundSources.append('InOut/FL_graph.cpp')
    libCsoundSources.append('InOut/winFLTK.c')
    libCsoundSources.append('InOut/widgets.cpp')

if (commonEnvironment['dynamicCsoundLibrary'] == '1'):
  print 'CONFIGURATION DECISION: Building dynamic Csound library'
  csoundLibrary = csoundLibraryEnvironment.SharedLibrary('csound',
                                                         libCsoundSources)
else:
  print 'CONFIGURATION DECISION: Building static Csound library'
  csoundLibrary = csoundLibraryEnvironment.Library('csound', libCsoundSources)
libs.append(csoundLibrary)
zipDependencies.append(csoundLibrary)

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

pluginLibraries.append(pluginEnvironment.SharedLibrary('babo',
    ['Opcodes/babo.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('bbcut',
    ['Opcodes/bbcut.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('biquad',
    ['Opcodes/biquad.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('butter',
    ['Opcodes/butter.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('clfilt',
    ['Opcodes/clfilt.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('cross2',
    ['Opcodes/cross2.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('dam',
    ['Opcodes/dam.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('dcblockr',
    ['Opcodes/dcblockr.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('filter',
    ['Opcodes/filter.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('flanger',
    ['Opcodes/flanger.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('follow',
    ['Opcodes/follow.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('freeverb',
    ['Opcodes/freeverb.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ftconv',
    ['Opcodes/ftconv.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('grain',
    ['Opcodes/grain.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('grain4',
    ['Opcodes/grain4.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('hrtferX',
    Split('''Opcodes/hrtferX.c
    Opcodes/natben.c''')))
pluginLibraries.append(pluginEnvironment.SharedLibrary('locsig',
    ['Opcodes/locsig.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('lowpassr',
    ['Opcodes/lowpassr.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('metro',
    ['Opcodes/metro.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('midiops2',
    ['Opcodes/midiops2.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('midiops3',
    ['Opcodes/midiops3.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('modal4',
    Split('''Opcodes/modal4.c
    Opcodes/physutil.c''')))
pluginLibraries.append(pluginEnvironment.SharedLibrary('newfils',
    ['Opcodes/newfils.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('nlfilt',
    ['Opcodes/nlfilt.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('oscbnk',
    ['Opcodes/oscbnk.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('phisem',
    ['Opcodes/phisem.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('physmod',
    Split('''Opcodes/physmod.c
    Opcodes/physutil.c
    Opcodes/mandolin.c
    Opcodes/singwave.c
    Opcodes/fm4op.c
    Opcodes/moog1.c
    Opcodes/shaker.c
    Opcodes/bowedbar.c''')))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pitch',
    Split('''Opcodes/pitch.c
    Opcodes/pitch0.c
    Opcodes/spectra.c''')))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pluck',
    ['Opcodes/pluck.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pvsdemix',
    ['Opcodes/pvsdemix.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('repluck',
    ['Opcodes/repluck.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('reverbsc',
    ['Opcodes/reverbsc.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('scansyn',
    ['Opcodes/scansyn.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('scansynx',
    ['Opcodes/scansynx.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('seqtime',
    ['Opcodes/seqtime.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('sfont',
    ['Opcodes/sfont.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('sndwarp',
    ['Opcodes/sndwarp.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('space',
    ['Opcodes/space.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('spat3d',
    ['Opcodes/spat3d.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('syncgrain',
    ['Opcodes/syncgrain.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugens7',
    ['Opcodes/ugens7.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugens9',
    ['Opcodes/ugens9.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugensa',
    ['Opcodes/ugensa.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('uggab',
    ['Opcodes/uggab.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugmoss',
    ['Opcodes/ugmoss.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugsc',
    ['Opcodes/ugsc.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('vdelayk',
    ['Opcodes/vdelayk.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('wave-terrain',
    ['Opcodes/wave-terrain.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('bus',
    ['Opcodes/bus.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pvsbasic',
    ['Opcodes/pvsbasic.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pvscent',
    ['Opcodes/pvscent.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('ftest',
    ['Opcodes/ftest.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('gab_gab',
    ['Opcodes/gab/gab.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('gab_vectorial',
    ['Opcodes/gab/vectorial.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('mixer',
    ['Opcodes/mixer.cpp']))

# Plugins with External Dependencies

# REAL TIME AUDIO

if (not(commonEnvironment['useALSA']=='1' and alsaFound)):
    print "CONFIGURATION DECISION: Not building ALSA plugin."
else:
    print "CONFIGURATION DECISION: Building ALSA plugin."
    alsaEnvironment = pluginEnvironment.Copy()
    alsaEnvironment.Append(LIBS = ['asound'])
    alsaEnvironment.Append(LIBS = ['pthread'])
    pluginLibraries.append(alsaEnvironment.SharedLibrary('rtalsa',
                                                         ['InOut/rtalsa.c']))

if (not(commonEnvironment['usePortAudio']=='1' and portaudioFound)):
    print "CONFIGURATION DECISION: Not building PortAudio module."
else:
    print "CONFIGURATION DECISION: Building PortAudio module."
    portaudioEnvironment = pluginEnvironment.Copy()
    portaudioEnvironment.Append(LIBS = ['portaudio'])
    if (getPlatform() == 'linux'):
        if (commonEnvironment['useJack']=='1' and jackFound):
            print "Adding Jack library for PortAudio"
            portaudioEnvironment.Append(LIBS = ['jack'])
        portaudioEnvironment.Append(LIBS = ['asound'])
        portaudioEnvironment.Append(LIBS = ['pthread'])
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw':
        portaudioEnvironment.Append(LIBS = ['winmm'])
        portaudioEnvironment.Append(LIBS = ['dsound'])
        portaudioEnvironment.Append(LIBS = ['kernel32'])
        portaudioEnvironment.Append(LIBS = ['gdi32'])
        portaudioEnvironment.Append(LIBS = ['wsock32'])
        portaudioEnvironment.Append(LIBS = ['ole32'])
        portaudioEnvironment.Append(LIBS = ['uuid'])
    if (commonEnvironment['useOldPortAudioPlugin']=='1'):
        pluginLibraries.append(portaudioEnvironment.SharedLibrary('rtpa',
                               Split('''InOut/rtpa.c InOut/pa_blocking.c''')))
    else:
        pluginLibraries.append(portaudioEnvironment.SharedLibrary('rtpa',
                                                        ['InOut/rtpa_new.c']))

if (not(commonEnvironment['useJack']=='1' and jackFound)):
    print "CONFIGURATION DECISION: Not building JACK plugin."
else:
    print "CONFIGURATION DECISION: Building JACK plugin."
    jackEnvironment = pluginEnvironment.Copy()
    jackEnvironment.Append(LIBS = ['jack'])
    jackEnvironment.Append(LIBS = ['asound'])
    jackEnvironment.Append(LIBS = ['pthread'])
    pluginLibraries.append(jackEnvironment.SharedLibrary('rtjack',
                                                         ['InOut/rtjack.c']))

if (not(commonEnvironment['useOSC']=='1' and oscFound)):
    print "CONFIGURATION DECISION: Not building OSC plugin."
else:
    print "CONFIGURATION DECISION: Building OSC plugin."
    oscEnvironment = pluginEnvironment.Copy()
    oscEnvironment.Append(LIBS = ['lo'])
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
    if getPlatform() == 'cygwin' or getPlatform() == 'mingw':
        fluidEnvironment = pluginEnvironment.Copy()
        fluidEnvironment.Append(LIBS = ['fluidsynth', 'stdc++'])
        fluidEnvironment.Append(LIBS = ['winmm', 'dsound'])
        fluidEnvironment.Append(CCFLAGS = ['-DFLUIDSYNTH_NOT_A_DLL', '-DMAKEDLL','-DBUILDING_DLL'])
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluidOpcodes',
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp']))

# VST HOST OPCODES

if getPlatform() == 'mingw' and fltkFound:
    vst4Environment = vstEnvironment.Copy()
    vst4Environment.Append(LIBS = ['stdc++', 'fltk'])
    if getPlatform() == 'mingw':
        vst4Environment.Append(LIBS = ['kernel32'])
        vst4Environment.Append(LIBS = ['gdi32'])
        vst4Environment.Append(LIBS = ['wsock32'])
        vst4Environment.Append(LIBS = ['ole32'])
        vst4Environment.Append(LIBS = ['uuid'])
    vst4Environment.Append(CPPPATH = ['frontends/CsoundVST'])
    zipDependencies.append(vst4Environment.SharedLibrary('vst4cs',
         Split('''
         Opcodes/vst4cs/src/vst4cs.cpp
         Opcodes/vst4cs/src/fxbank.cpp
         Opcodes/vst4cs/src/vsthost.cpp
         ''')))

# Experimental OSC Opcodes ** THIS DOES NOT WORK **
if getPlatform() == 'linux':
    oscEnvironment = pluginEnvironment.Copy()
    pluginLibraries.append(oscEnvironment.SharedLibrary('osc-sock',
        Split('''
        Opcodes/osc-sock.c
        Opcodes/OSC-Kit/NetworkReturnAddress.c
        Opcodes/OSC-Kit/OSC-address-space.c
        Opcodes/OSC-Kit/OSC-callbacklist.c
        Opcodes/OSC-Kit/OSC-client.c
        Opcodes/OSC-Kit/OSC-drop.c
        Opcodes/OSC-Kit/OSC-pattern-match.c
        Opcodes/OSC-Kit/OSC-priority-queue.c
        Opcodes/OSC-Kit/OSC-receive.c
        Opcodes/OSC-Kit/OSC-string-help.c
        Opcodes/OSC-Kit/OSC-system-dependent.c
        Opcodes/OSC-Kit/OSC-timetag.c
        ''')))

# Utility programs.

pluginLibraries.append(pluginEnvironment.SharedLibrary('cvanal',
    ['util/cvanal.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('dnoise',
    ['util/dnoise.c']))
#pluginLibraries.append(pluginEnvironment.SharedLibrary('hetro',
#    ['util/hetro.c']))
#pluginLibraries.append(pluginEnvironment.SharedLibrary('lpanal',
#    ['util/lpanal.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pvanal',
    ['util/pvanal.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('pvlook',
    ['util/pvlook.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('sndinfo',
    ['util/sndinfo.c']))

executables.append(csoundProgramEnvironment.Program('cscore',
    ['util1/cscore/cscore_main.c']))
executables.append(csoundProgramEnvironment.Program('cvanal',
    ['util/cvl_main.c']))
executables.append(csoundProgramEnvironment.Program('dnoise',
    ['util/dnoise_main.c']))
executables.append(csoundProgramEnvironment.Program('envext',
    ['util2/envext/envext.c']))
executables.append(csoundProgramEnvironment.Program('extract',
    ['util1/sortex/xmain.c']))
executables.append(csoundProgramEnvironment.Program('extractor',
    ['util2/mixer/xtrct.c']))
executables.append(csoundProgramEnvironment.Program('het_export',
    ['util2/exports/het_export.c']))
executables.append(csoundProgramEnvironment.Program('het_import',
    ['util2/exports/het_import.c']))
executables.append(csoundProgramEnvironment.Program('hetro',
    ['util/het_main.c']))
executables.append(csoundProgramEnvironment.Program('lpanal',
    ['util/lpc_main.c']))
executables.append(csoundProgramEnvironment.Program('lpc_export',
    ['util2/exports/lpc_export.c']))
executables.append(csoundProgramEnvironment.Program('lpc_import',
    ['util2/exports/lpc_import.c']))
executables.append(csoundProgramEnvironment.Program('mixer',
    ['util2/mixer/mixer.c']))
executables.append(csoundProgramEnvironment.Program('pv_export',
    ['util2/exports/pv_export.c']))
executables.append(csoundProgramEnvironment.Program('pv_import',
    ['util2/exports/pv_import.c']))
executables.append(csoundProgramEnvironment.Program('pvanal',
    ['util/pvc_main.c']))
executables.append(csoundProgramEnvironment.Program('pvlook',
    ['util/pvl_main.c']))
executables.append(csoundProgramEnvironment.Program('scale',
    ['util2/scale.dir/scale.c']))
executables.append(csoundProgramEnvironment.Program('scot',
    ['util1/scot/scot_main.c']))
executables.append(csoundProgramEnvironment.Program('scsort',
    ['util1/sortex/smain.c']))
executables.append(csoundProgramEnvironment.Program('sdif2ad',
    Split('''SDIF/sdif2adsyn.c
    SDIF/sdif.c
    SDIF/sdif-mem.c''')))
executables.append(csoundProgramEnvironment.Program('sndinfo',
    ['util/sndinfo_main.c']))
executables.append(csoundProgramEnvironment.Program('srconv',
    ['util2/dnoise.dir/srconv.c']))
executables.append(commonEnvironment.Program('cs', ['util1/csd_util/cs.c']))
executables.append(commonEnvironment.Program('csb64enc',
                                             Split('''util1/csd_util/base64.c
                                                      util1/csd_util/csb64enc.c
                                                   ''')))
executables.append(commonEnvironment.Program('makecsd',
                                             Split('''util1/csd_util/base64.c
                                                      util1/csd_util/makecsd.c
                                                   ''')))

# Front ends.

executables.append(csoundProgramEnvironment.Program('csound',
    ['frontends/csound/csound_main.c']))

if not ((commonEnvironment['buildCsoundVST'] == '1') and boostFound and fltkFound):
    print 'CONFIGURATION DECISION: Not building CsoundVST plugin and standalone.'
else:
    print 'CONFIGURATION DECISION: Building CsoundVST plugin and standalone.'
    headers += glob.glob('frontends/CsoundVST/*.h')
    headers += glob.glob('frontends/CsoundVST/*.hpp')
    vstEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    vstEnvironment.Prepend(LIBS = ['csound', 'sndfile'])
    vstEnvironment.Append(SWIGFLAGS = Split('-c++ -includeall -verbose -outdir .'))
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['python2.3', 'util', 'dl', 'm'])
        vstEnvironment.Append(CPPPATH = ['/usr/include/python2.3'])
        if commonEnvironment['Word64']=='1':
            vstEnvironment.Append(LIBPATH = ['/usr/lib64/python2.3/config'])
        else:
            vstEnvironment.Append(LIBPATH = ['/usr/lib/python2.3/config'])
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-mwindows', '_CsoundVST.so'])
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw':
        pythonImportLibrary = vstEnvironment.Command('/usr/local/lib/libpython23.a',
        '$SYSTEMROOT/System32/python23.dll',
        ['pexports $SYSTEMROOT/System32/python23.dll > python23.def', 'dlltool --input-def python23.def --dllname python23.dll --output-lib /usr/local/lib/libpython23.a'])
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        vstEnvironment.Append(SHLINKFLAGS = '-Wl,--add-stdcall-alias')
        vstEnvironment.Append(CCFLAGS = ['-DNDEBUG'])
        if getPlatform() == 'cygwin':
                vstEnvironment.Append(CCFLAGS = ['-D_MSC_VER'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-mwindows'])
        vstEnvironment.Append(LIBS = ['python23'])
        vstEnvironment.Append(LIBS = ['fltk_images'])
        vstEnvironment.Append(LIBS = ['fltk'])
        guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    for option in vstEnvironment['CPPPATH']:
        option = '-I' + option
        vstEnvironment.Append(SWIGFLAGS = [option])
    for option in vstEnvironment['CCFLAGS']:
        if string.find(option, '-D') == 0:
           vstEnvironment.Append(SWIGFLAGS = [option])

    print 'PATH =',commonEnvironment['ENV']['PATH']
    csoundVstSources = Split('''
    frontends/CsoundVST/AudioEffect.cpp
    frontends/CsoundVST/audioeffectx.cpp
    frontends/CsoundVST/Composition.cpp
    frontends/CsoundVST/Conversions.cpp
    frontends/CsoundVST/Counterpoint.cpp
    frontends/CsoundVST/CounterpointNode.cpp
    frontends/CsoundVST/CppSound.cpp
    frontends/CsoundVST/CsoundFile.cpp
    frontends/CsoundVST/Cell.cpp
    frontends/CsoundVST/CsoundVST.cpp
    frontends/CsoundVST/csoundvst_api.cpp
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
    ''')
    # These are the Windows system call libraries.
    if getPlatform() == 'mingw':
        vstEnvironment.Append(LIBS = ['kernel32'])
        vstEnvironment.Append(LIBS = ['gdi32'])
        vstEnvironment.Append(LIBS = ['wsock32'])
        vstEnvironment.Append(LIBS = ['ole32'])
        vstEnvironment.Append(LIBS = ['uuid'])
        vstEnvironment.Append(SHLINKFLAGS = ['-module'])
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        csoundVstSources.append('frontends/CsoundVST/_CsoundVST.def')
    swigflags = vstEnvironment['SWIGFLAGS']
    csoundVstPythonWrapper = vstEnvironment.SharedObject('frontends/CsoundVST/CsoundVST.i', SWIGFLAGS = [swigflags,'-python'])
    csoundVstSources.append(csoundVstPythonWrapper)
    if configure.CheckHeader('jni.h', language = 'C++') and commonEnvironment['buildJavaWrapper']=='1':
        print 'CONFIGURATION DECISION: Building Java wrappers for CsoundVST.'
        csoundVstJavaWrapper = vstEnvironment.SharedObject('frontends/CsoundVST/JCsoundVST.i', SWIGFLAGS = [swigflags,'-java', '-package', 'CsoundVST'])
        csoundVstSources.append(csoundVstJavaWrapper)
        jcsound = vstEnvironment.Java(target = 'frontends/CsoundVST/classes', source = '.', JAVACFLAGS = Split('''-source 1.4 -target 1.4'''))
        zipDependencies.append(jcsound)
    	jcsoundJar = vstEnvironment.Jar('CsoundVST.jar', ['manifest.mf', 'frontends/CsoundVST/classes'], JARCHDIR = 'frontends/CsoundVST/classes')
    else:
	print 'CONFIGURATION DECISION: Not building Java wrappers for CsoundVST.'
    csoundvst = vstEnvironment.SharedLibrary('CsoundVST', csoundVstSources, SHLIBPREFIX = '_')
    Depends(csoundvst, 'frontends/CsoundVST/CsoundVST_wrap.cc')
    zipDependencies.append(csoundvst)
    libs.append(csoundvst)
    libs.append('CsoundVST.py')
    Depends(csoundvst, csoundLibrary)
    if getPlatform() == 'mingw' or getPlatform() == 'cygwin':
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
Opcodes/stk/src/Mutex.cpp
Opcodes/stk/src/RtAudio.cpp
Opcodes/stk/src/RtMidi.cpp
Opcodes/stk/src/RtDuplex.cpp
Opcodes/stk/src/RtWvIn.cpp
Opcodes/stk/src/RtWvOut.cpp
Opcodes/stk/src/Socket.cpp
Opcodes/stk/src/TcpWvIn.cpp
Opcodes/stk/src/TcpWvOut.cpp
Opcodes/stk/src/Thread.cpp
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

    pyEnvironment = pluginEnvironment.Copy();
    if getPlatform() == 'linux':
        pyEnvironment.Append(LIBS = ['python2.3', 'util', 'dl', 'm'])
        pyEnvironment.Append(CPPPATH = ['/usr/local/include/python2.3'])
        if commonEnvironment['Word64']=='1':
            pyEnvironment.Append(LIBPATH = ['/usr/local/lib64/python2.3/config'])
        else:
            pyEnvironment.Append(LIBPATH = ['/usr/local/lib/python2.3/config'])
        pyEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        pyEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw':
        pyEnvironment['ENV']['PATH'] = os.environ['PATH']
        pyEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        pyEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        if getPlatform() == 'cygwin':
            pyEnvironment.Append(CCFLAGS = ['-D_MSC_VER'])
        pyEnvironment.Append(LIBS = ['python23'])
    py = pyEnvironment.SharedLibrary('py', ['Opcodes/py/pythonopcodes.c'])
    Depends(py, csoundvst)
    pluginLibraries.append(py)

if (commonEnvironment['generateTags']=='0') or (getPlatform() != 'darwin' and getPlatform() != 'linux' and getPlatform() != 'cygwin'):
    print "CONFIGURATION DECISION: Not calling TAGS"
else:
    print "CONFIGURATION DECISION: Calling TAGS"
    allSources = string.join(glob.glob('*/*.h*'))
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

if commonEnvironment['Word64']=='1':
    LIB_DIR = PREFIX + "/lib64"
    OPCODE_DIR = PREFIX + "/lib64/csound/opcodes"
else:
    LIB_DIR = PREFIX + "/lib"
    OPCODE_DIR = PREFIX + "/lib/csound/opcodes"

if commonEnvironment['install']=='1':
    installExecutables = Alias('install-executables',
        Install(BIN_DIR, executables))

    installOpcodes = Alias('install-opcodes',
        Install(OPCODE_DIR, pluginLibraries))

    installHeaders = Alias('install-headers',
        Install(INCLUDE_DIR, headers))

    installLibs = Alias('install-libs',
        Install(LIB_DIR, libs))

    Alias('install', [installExecutables, installOpcodes, installLibs, installHeaders])

