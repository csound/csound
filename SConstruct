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

#############################################################################
#
#   UTILITY FUNCTIONS
#
#############################################################################

zipDependencies = []
pluginLibraries = []
executables = []

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
opts.Add('customCPPPATH', 'List of custom CPPPATH variables')
opts.Add('customCCFLAGS')
opts.Add('customCXXFLAGS')
opts.Add('customLIBS')
opts.Add('customLIBPATH')
opts.Add('customSHLINKFLAGS')
opts.Add('useDouble', 
    'Set to 1 to use double-precision floating point for audio samples.', 
    0)
opts.Add('usePortAudio', 
    'Set to 1 to use PortAudio for real-time audio input and output.', 
    1)
opts.Add('useJack',
    'Set to 1 if you compiled PortAudio to use Jack',
    1)
opts.Add('useFLTK', 
    'Set to 1 to use FLTK for graphs and widget opcodes.', 
    1)
opts.Add('buildCsoundVST', 
    'Set to 1 to build CsoundVST (needs FLTK, boost, Python 2.3, SWIG).', 
    1)
opts.Add('noCygwin',
    'Set to 1 to build with -mno-cygwin when using Cygwin',
    0)
opts.Add('generateTags',
    'Set to 1 to generate TAGS',
    0)
opts.Add('generatePDF',
    'Set to 1 to generate PDF documentation',
    0)
opts.Add('makeDynamic',
    'Set to 1 to generate dynamically linked programs',
    1)
opts.Add('generateXmg',
    'Set to 1 to generate string database',
    1)
opts.Add('generateZip',
    'Set to 1 to generate zip archive',
    0)
opts.Add('buildLoris',
    'Set to 1 to build the Loris Python extension and opcodes',
    1)

# Define the common part of the build environment.
# This section also sets up customized options for third-party libraries, which
# should take priority over default options.

commonEnvironment = Environment(options = opts)

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

Help(opts.GenerateHelpText(commonEnvironment))

# Define options for different platforms.

print "Build platform is '" + getPlatform() + "'."
print "SCons tools on this platform: ", commonEnvironment['TOOLS']
print

commonEnvironment.Prepend(CPPPATH  = ['.', './H'])
commonEnvironment.Prepend(CCFLAGS = Split('-DCSOUND_WITH_API -g -O2'))
commonEnvironment.Prepend(CXXFLAGS = Split('-DCSOUND_WITH_API -fexceptions'))
commonEnvironment.Prepend(LIBPATH = ['.', '#.'])
commonEnvironment.Prepend(CPPFLAGS = ['-DBETA'])
commonEnvironment.Prepend(LIBPATH = ['.', '#.', '/usr/lib', '/usr/local/lib'])

if commonEnvironment['useDouble']:
    print 'CONFIGURATION DECISION: Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = ['-DUSE_DOUBLE'])
else:
    print 'CONFIGURATION DECISION: Using single-precision floating point for audio samples.'

# Define different build environments for different types of targets.

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
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
    
if (commonEnvironment['makeDynamic'] == 0) and (getPlatform() != 'linux') and (getPlatform() != 'darwin'):
    commonEnvironment.Append(LINKFLAGS = '-static')
else:
    if (getPlatform() == 'linux'):
        commonEnvironment.Append(LINKFLAGS = Split('-Wl,-Bdynamic'))

# Adding libraries and flags if using -mno-cygwin with cygwin

if commonEnvironment['noCygwin'] and getPlatform() == 'cygwin':
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
fltkFound = configure.CheckHeader("FL/Fl.H", language = "C++")
boostFound = configure.CheckHeader("boost/any.hpp", language = "C++")

if getPlatform() == 'mingw':
    commonEnvironment['ENV']['PATH'] = os.environ['PATH']

# Define macros that configure and config.h used to define.

if configure.CheckHeader("io.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_IO_H')
if configure.CheckHeader("fcntl.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_FCNTL_H')
if configure.CheckHeader("unistd.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_UNISTD_H')
if configure.CheckHeader("malloc.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_MALLOC_H')
if configure.CheckHeader("sgtty.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SGTTY_H')
if configure.CheckHeader("os.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_OS_H')
if configure.CheckHeader("sys/ioctl.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_IOCTL_H')
if configure.CheckHeader("sys/time.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_TIME_H')
if configure.CheckHeader("sys/types.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_SYS_TYPES_H')
if configure.CheckHeader("term/ios.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_TERM_IOS_H')
if configure.CheckHeader("termios.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_TERMIOS_H')
if configure.CheckHeader("string.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_STRING_H')
if configure.CheckHeader("strings.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_STRINGS_H')
if configure.CheckHeader("dirent.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_DIRENT_H')
if configure.CheckFunc("itoa") or getPlatform() == 'mingw':
    commonEnvironment.Append(CCFLAGS = '-DHAVE_ITOA')
if not (configure.CheckHeader("Opcodes/Loris/src/loris.h") and configure.CheckHeader("fftw3.h")):
    commonEnvironment["buildLoris"] = 0
else:
    print "CONFIGURATION DECISION: Building Loris Python extension and Csound opcodes."
        
# Package contents.

zipfilename = "csound5-" + getPlatform() + "-" + str(today()) + ".zip"

def buildzip(env, target, source):

    os.chdir('..')
    directories = string.split("csound5")
    
    extensions = ".sln .csproj .vsproj .dev .def .am .sh .ac .in .dll .so .exe"
    extensions = extensions + ".htm .html .doc .mso .png .xml .mso .gif .jpg .jpeg .hlp .nb .wks .xls .pdf "
    extensions = extensions + ".c .C .cpp .cxx .h .hpp .H .hxx .py .rc .res .fl .i .java"
    extensions = extensions + ".sf2 .SF2 .csd .aif .aiff .jar .smf .mid"
    extensions = string.split(extensions)
    
    specificFiles = "SConstruct _CsoundVST.* _loris.* pyrun.* lori.py lorisgens.C lorisgens.h morphdemo.py trymorph.csd CsoundCOM.dll msvcp70.dll libsndfile.dll portaudio.dll msvcr70.dll csound csound.exe CsoundVST CsoundVST.exe CsoundVST.* soundfonts.dll libpython23.a "
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
    print
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


staticLibraryEnvironment = commonEnvironment.Copy()

pluginEnvironment = commonEnvironment.Copy()

if getPlatform() == 'darwin':
    pluginEnvironment.Append(LINKFLAGS = ['-dynamiclib'])
    pluginEnvironment['SHLIBSUFFIX'] = '.dylib'

csoundProgramEnvironment = commonEnvironment.Copy()
csoundProgramEnvironment.Append(LIBS = ['csound', 'sndfile'])
csoundProgramEnvironment.ParseConfig('fltk-config --cflags --cxxflags --ldflags') 

ustubProgramEnvironment = commonEnvironment.Copy()
ustubProgramEnvironment.Append(LIBS = ['ustub', 'sndfile'])

vstEnvironment = commonEnvironment.Copy()
if vstEnvironment.ParseConfig('fltk-config --use-images --cflags --cxxflags --ldflags'):
    print "Parsed fltk-config." 

guiProgramEnvironment = commonEnvironment.Copy()

if commonEnvironment['usePortAudio']==1 and portaudioFound:
    staticLibraryEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    pluginEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    csoundProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    ustubProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    vstEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    guiProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    csoundProgramEnvironment.Append(LIBS = ['portaudio'])
    vstEnvironment.Append(LIBS = ['portaudio'])
    if (getPlatform() == 'linux'): 
        csoundProgramEnvironment.Append(LIBS = ['asound'])
        vstEnvironment.Append(LIBS = ['asound'])
        if (commonEnvironment['useJack']==1):
            print "Adding Jack library for PortAudio"
            csoundProgramEnvironment.Append(LIBS = ['jack'])
            vstEnvironment.Append(LIBS = ['jack'])  
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw': 
        csoundProgramEnvironment.Append(LIBS = ['winmm'])
        vstEnvironment.Append(LIBS = ['winmm'])
        csoundProgramEnvironment.Append(LIBS = ['dsound'])
        vstEnvironment.Append(LIBS = ['dsound'])

if (commonEnvironment['useFLTK'] and fltkFound):
    staticLibraryEnvironment.Append(CCFLAGS = '-DWINDOWS')
    pluginEnvironment.Append(CCFLAGS = '-DWINDOWS')
    csoundProgramEnvironment.Append(CCFLAGS = '-DWINDOWS')
    ustubProgramEnvironment.Append(CCFLAGS = '-DWINDOWS')
    vstEnvironment.Append(CCFLAGS = '-DWINDOWS')
    guiProgramEnvironment.Append(CCFLAGS = '-DWINDOWS')
    staticLibraryEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    pluginEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    csoundProgramEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    ustubProgramEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    vstEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    guiProgramEnvironment.Append(CCFLAGS = '-DUSE_FLTK')
    csoundProgramEnvironment.Append(LIBS = ['fltk'])
    vstEnvironment.Append(LINKFLAGS = "--subsystem:windows")
    guiProgramEnvironment.Append(LINKFLAGS = "--subsystem:windows")
    if getPlatform() == 'linux' or getPlatform() == 'cygwin':
            csoundProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            ustubProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
    elif getPlatform() == 'mingw':
            csoundProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            ustubProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            vstEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            guiProgramEnvironment.Append(LIBS = ['stdc++', 'supc++'])
            if getPlatform() == 'linux':
                    csoundProgramEnvironment.Append(LIBS = ['dl'])
                    ustubProgramEnvironment.Append(LIBS = ['dl'])
                    vstEnvironment.Append(LIBS = ['dl'])
                    guiProgramEnvironment.Append(LIBS = ['dl'])
    elif getPlatform() == 'darwin':
            csoundProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            ustubProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            vstEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread', 'm'])
            csoundProgramEnvironment.Append(LINKFLAGS = ['-framework', 'Carbon'])

##### -framework ApplicationServices'))

if getPlatform() == 'mingw':
    # These are the Windows system call libraries.
    csoundProgramEnvironment.Append(LIBS = ['kernel32'])
    csoundProgramEnvironment.Append(LIBS = ['gdi32'])
    csoundProgramEnvironment.Append(LIBS = ['wsock32'])
    csoundProgramEnvironment.Append(LIBS = ['ole32'])
    csoundProgramEnvironment.Append(LIBS = ['uuid'])
    csoundProgramEnvironment.Append(LIBS = ['winmm'])

#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

if commonEnvironment['generatePDF']==1:
    print 'CONFIGURATION DECISION: Generating PDF documentation.'
    csoundPdf = commonEnvironment.Command('csound.pdf', 'csound.tex', 'pdflatex $SOURCE')
    zipDependencies.append(csoundPdf)

commonEnvironment.Alias('pdf', commonEnvironment.Command('csound.pdf', 'csound.tex', 'pdflatex $SOURCE'))
    
makedb = ustubProgramEnvironment.Program('makedb', 
    ['strings/makedb.c'])
zipDependencies.append(makedb)

libCsoundSources = Split('''
Engine/auxfd.c
Engine/entry1.c
Engine/entry2.c
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
InOut/winascii.c
InOut/windin.c
InOut/window.c
InOut/winEPS.c
OOps/aops.c
OOps/cmath.c
OOps/control.c
OOps/diskin.c
OOps/disprep.c
OOps/dnfft.c
OOps/dsputil.c
OOps/dumpf.c
OOps/fft.c
OOps/fout.c
OOps/fprint.c
OOps/lptrkfns.c
OOps/mididevice.c
OOps/midiinterop.c
OOps/midiops.c
OOps/midiout.c
OOps/midirecv.c
OOps/midisend.c
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
OOps/ugens9.c
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
Top/csound.c
Top/cvanal.c
Top/dl_opcodes.c
Top/dnoise.c
Top/getstring.c
Top/hetro.c
Top/lpanal.c
Top/main.c
Top/natben.c
Top/one_file.c
Top/opcode.c
Top/pvanal.c
Top/pvlook.c
Top/scot.c
Top/sndinfo.c
Top/threads.c
''')

if (commonEnvironment['usePortAudio']==1) and portaudioFound:
    print 'CONFIGURATION DECISION: Building with PortAudio.'
    libCsoundSources.append('InOut/rtpa.c')
    libCsoundSources.append('InOut/pa_blocking.c')
    
if (commonEnvironment['useFLTK'] and fltkFound):
    print 'CONFIGURATION DECISION: Building with FLTK for graphs and widgets.'
    libCsoundSources.append('InOut/FL_graph.cpp')
    libCsoundSources.append('InOut/winFLTK.c')
    libCsoundSources.append('InOut/widgets.cpp')
    
staticLibrary = staticLibraryEnvironment.Library('csound', 
    libCsoundSources)
zipDependencies.append(staticLibrary)
    
libUstubSources = Split('''
Engine/extract.c 
Engine/filopen.c 
Engine/memalloc.c 
Engine/memfiles.c 
Engine/scsort.c 
Engine/scxtract.c 
Engine/sort.c 
Engine/sread.c 
Engine/swrite.c 
Engine/twarp.c 
InOut/libsnd.c 
InOut/libsnd_u.c
InOut/winascii.c 
InOut/window.c 
InOut/winEPS.c 
InOut/winFLTK.c 
OOps/dsputil.c 
OOps/fft.c 
OOps/mxfft.c 
OOps/lptrkfns.c 
OOps/pvfileio.c 
OOps/pvoc.c 
OOps/pvocext.c 
OOps/pvread.c 
OOps/pvsanal.c 
OOps/pvxanal.c 
OOps/sdif.c 
Top/cvanal.c 
Top/getstring.c 
Top/hetro.c 
Top/lpanal.c 
Top/natben.c 
Top/pvanal.c 
Top/pvlook.c 
Top/scot.c 
Top/sndinfo.c 
Top/ustub.c 
''')

ustub = staticLibraryEnvironment.Library('ustub', 
    libUstubSources)
zipDependencies.append(ustub)

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
pluginLibraries.append(pluginEnvironment.SharedLibrary('grain', 
    ['Opcodes/grain.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('grain4', 
    ['Opcodes/grain4.c']))
pluginLibraries.append(pluginEnvironment.SharedLibrary('hrtferX', 
    Split('''Opcodes/hrtferX.c 
    Top/natben.c''')))
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
pluginLibraries.append(pluginEnvironment.SharedLibrary('repluck', 
    ['Opcodes/repluck.c']))
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
pluginLibraries.append(pluginEnvironment.SharedLibrary('ugens7', 
    ['Opcodes/ugens7.c']))
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

# Plugins with External Dependencies

# FLUIDSYNTH OPCODES

if configure.CheckHeader("fluidsynth.h", language = "C"):            
    if getPlatform() == 'linux':
        fluidEnvironment = pluginEnvironment.Copy()
        fluidEnvironment.Append(LIBS = ['fluidsynth'])
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluidOpcodes', 
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp']))
    if getPlatform() == 'cygwin' or getPlatform() == 'mingw':        
        vstEnvironment.Append(CCFLAGS = ['-DFLUIDSYNTH_NOT_A_DLL', '-DMAKEDLL','-DBUILDING_DLL'])    
        fluidEnvironment = vstEnvironment.Copy()    
        fluidEnvironment.Append(LIBS = ['fluidsynth', 'stdc++', 'fltk'])    
        fluidEnvironment.Append(LINKFLAGS = ['-mno-cygwin'])        
        fluidEnvironment.Append(LIBS = ['winmm','dsound'])
        if getPlatform() == 'mingw':
            fluidEnvironment.Append(LIBS = ['kernel32'])
            fluidEnvironment.Append(LIBS = ['gdi32'])
            fluidEnvironment.Append(LIBS = ['wsock32'])
            fluidEnvironment.Append(LIBS = ['ole32'])
            fluidEnvironment.Append(LIBS = ['uuid'])
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluidOpcodes', 
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp']))    
        pluginLibraries.append(fluidEnvironment.SharedLibrary('fluid', Split('''
            Opcodes/fluid/AudioEffect.cpp        
            Opcodes/fluid/audioeffectx.cpp        
            Opcodes/fluid/Soundfonts.cpp        
            Opcodes/fluid/SoundfontsMain.cpp        
            Opcodes/fluid/FluidsynthOpcode.cpp        
            ''')))
            
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

executables.append(csoundProgramEnvironment.Program('cscore', 
    ['util1/cscore/cscore_main.c']))
executables.append(csoundProgramEnvironment.Program('cvanal', 
    ['anal/convol/cvl_main.c']))
executables.append(csoundProgramEnvironment.Program('dnoise', 
    ['util2/dnoise.dir/dnoise_main.c']))
executables.append(ustubProgramEnvironment.Program('envext', 
    ['util2/envext/envext.c']))
executables.append(ustubProgramEnvironment.Program('extract', 
    ['util1/sortex/xmain.c']))
executables.append(ustubProgramEnvironment.Program('extractor', 
    ['util2/mixer/xtrct.c']))
executables.append(ustubProgramEnvironment.Program('het_export', 
    ['util2/exports/het_export.c']))
executables.append(ustubProgramEnvironment.Program('het_import',  
    ['util2/exports/het_import.c']))
executables.append(ustubProgramEnvironment.Program('hetro',  
    ['anal/adsyn/het_main.c']))
executables.append(ustubProgramEnvironment.Program('lpanal',  
    ['anal/lpc/lpc_main.c']))
executables.append(ustubProgramEnvironment.Program('lpc_export',  
    ['util2/exports/lpc_export.c']))
executables.append(ustubProgramEnvironment.Program('lpc_import',  
    ['util2/exports/lpc_import.c']))
executables.append(ustubProgramEnvironment.Program('mixer',  
    ['util2/mixer/mixer.c']))
executables.append(ustubProgramEnvironment.Program('pv_export',   
    ['util2/exports/pv_export.c']))
executables.append(ustubProgramEnvironment.Program('pv_import',  
    ['util2/exports/pv_import.c']))
executables.append(csoundProgramEnvironment.Program('pvanal',  
    ['anal/pvoc/pvc_main.c']))
executables.append(ustubProgramEnvironment.Program('pvlook',  
    ['util2/pvlook.dir/pvl_main.c']))
executables.append(ustubProgramEnvironment.Program('scale',  
    ['util2/scale.dir/scale.c']))
executables.append(ustubProgramEnvironment.Program('scot',  
    ['util1/scot/scot_main.c']))
executables.append(ustubProgramEnvironment.Program('scsort',  
    ['util1/sortex/smain.c']))
executables.append(ustubProgramEnvironment.Program('sdif2ad',  
    Split('''SDIF/sdif2adsyn.c 
    SDIF/sdif.c 
    SDIF/sdif-mem.c''')))
executables.append(ustubProgramEnvironment.Program('sndinfo', 
    ['util2/sndinfo/sndinfo_main.c']))
executables.append(ustubProgramEnvironment.Program('srconv', 
    ['util2/dnoise.dir/srconv.c']))

# Front ends.

executables.append(csoundProgramEnvironment.Program('csound', 
    ['frontends/csound/csound_main.c']))
    
if (commonEnvironment['buildCsoundVST'] == 1) and boostFound and fltkFound:    
    print 'CONFIGURATION DECISION: Building CsoundVST plugin and standalone.'
    vstEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    vstEnvironment.Prepend(LIBS = ['csound', 'sndfile'])
    vstEnvironment.Append(SWIGFLAGS = Split('-python -c++ -includeall -verbose -outdir .'))
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['swigpy', 'python2.3', 'util', 'dl', 'm'])
        vstEnvironment.Append(CPPPATH = ['/usr/include/python2.3'])
        vstEnvironment.Append(LIBPATH = ['/usr/lib/python2.3/config'])
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-mwindows', '_CsoundVST.so'])
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw':
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
        vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
        if getPlatform() == 'cygwin':
                vstEnvironment.Append(CCFLAGS = ['-D_MSC_VER'])
        guiProgramEnvironment.Prepend(LINKFLAGS = ['-mwindows', '_CsoundVST.dll'])
        vstEnvironment.Append(LIBS = ['python23'])
        pyrunEnvironment = vstEnvironment.Copy()
        pyrunEnvironment.Append(CCFLAGS = '-DSWIG_GLOBAL')
        pyrun = pyrunEnvironment.SharedLibrary('pyrun', ['frontends/CsoundVST/pyrun.c'])
        #vstEnvironment.Append(LIBS = ['pyrun'])
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
    frontends/CsoundVST/CppSound.cpp 
    frontends/CsoundVST/CsoundFile.cpp 
    frontends/CsoundVST/Cell.cpp 
    frontends/CsoundVST/CsoundVST.cpp
    frontends/CsoundVST/csoundvst_api.cpp 
    frontends/CsoundVST/CsoundVST.i
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
        vstEnvironment['ENV']['PATH'] = os.environ['PATH']
        csoundVstSources.append('frontends/CsoundVST/_CsoundVST.def')
    csoundvst = vstEnvironment.SharedLibrary('CsoundVST', csoundVstSources, SHLIBPREFIX = '_')
    Depends(csoundvst, 'frontends/CsoundVST/CsoundVST_wrap.cc')
    zipDependencies.append(csoundvst)
    Depends(csoundvst, staticLibrary)
    if getPlatform() == 'mingw' or getPlatform() == 'cygwin':
        Depends(csoundvst, pyrun)

    csoundvstGui = guiProgramEnvironment.Program('CsoundVST', ['frontends/CsoundVST/csoundvst_main.cpp']) 
    zipDependencies.append(csoundvstGui)
    Depends(csoundvstGui, csoundvst)
     
    # Build the Loris and Python opcodes here because they depend 
    # on the same things as CsoundVST.
    
    if commonEnvironment['buildLoris']:
        # For Loris, we build only the loris Python extension module and
        # the Csound opcodes (modified for Csound 5).
        # It is assumed that you have copied the contents of the Loris distribution
        # 'src' directory into 'Opcodes/Loris/src'.
        
        shutil.copy('Opcodes/Loris/lorisgens.h', 'Opcodes/Loris/src/')
        shutil.copy('Opcodes/Loris/lorisgens.C', 'Opcodes/Loris/src/')
        shutil.copy('Opcodes/Loris/loris.i', 'Opcodes/Loris/src/')
        shutil.copy('Opcodes/Loris/lorisPartialList.i', 'Opcodes/Loris/src/')
        lorisEnvironment = vstEnvironment.Copy();
        lorisEnvironment.Append(CCFLAGS = '-DHAVE_FFTW3_H')
        lorisEnvironment.Append(LIBS = ['fftw3'])
        lorisSources = glob.glob('Opcodes/Loris/src/*.C')
        lorisSources.append('Opcodes/Loris/src/loris.i')
        lorisEnvironment.Append(SWIGPATH = ['./'])
        lorisEnvironment.Append(SWIGFLAGS = ['-DHAVE_FFTW3_H'])
        loris = lorisEnvironment.SharedLibrary('loris', lorisSources, SHLIBPREFIX = '_')
        Depends(loris, csoundvst)
        pluginLibraries.append(loris)
                 
        pyEnvironment = pluginEnvironment.Copy();
        if getPlatform() == 'linux':
            pyEnvironment.Append(LIBS = ['swigpy', 'python2.3', 'util', 'dl', 'm'])
            pyEnvironment.Append(CPPPATH = ['/usr/local/include/python2.3'])
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

if (commonEnvironment['generateTags']) and (getPlatform() == 'linux' or getPlatform() == 'cygwin'):
    print "CONFIGURATION DECISION: Calling TAGS"
    allSources = string.join(glob.glob('*/*.h*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.hpp'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.h'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.hpp'))
    tags = commonEnvironment.Command('TAGS', Split(allSources), 'etags $SOURCES')
    zipDependencies.append(tags)
    Depends(tags, staticLibrary)

if commonEnvironment['generateXmg']:
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


zipDependencies += executables
zipDependencies += pluginLibraries
    
if commonEnvironment['generateZip']:    
    print 'CONFIGURATION DECISION: Compiling zip file for release.'
    zip = commonEnvironment.Command(zipfilename, staticLibrary, buildzip)
    for node in zipDependencies:
        Depends(zip, node)

# INSTALL OPTIONS
        
OPCODE_DIR = "/usr/local/share/csound/opcodes"
BIN_DIR = "/usr/local/bin"

installOpcodes = Alias('install-opcodes', 
    Install(OPCODE_DIR, pluginLibraries)) 
                            
installExecutables = Alias('install-executables',
    Install(BIN_DIR, executables))
    
Alias('install', [installOpcodes, installExecutables])
