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

#############################################################################
#
#   UTILITY FUNCTIONS
#
#############################################################################

def today():
    return time.strftime("%Y-%m-%d",time.localtime())
    
def getPlatform():
    if sys.platform[:5] == 'linux':
        return 'linux'
    elif sys.platform == 'cygwin':
        return 'cygwin'
    elif sys.platform[:3] == 'win' and commonEnvironment['TOOLS'][1] == 'mingw':
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
    0)
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
    0)
opts.Add('generateXmg',
    'Set to 1 to generate string database',
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

commonEnvironment.Append(CPPPATH  = ['.', './H'])
commonEnvironment.Append(CCFLAGS = Split('-DCSOUND_WITH_API -g -O2'))
commonEnvironment.Append(CXXFLAGS = Split('-DCSOUND_WITH_API -fexceptions'))

# Define options for different platforms.

print "Build platform is '" + getPlatform() + "'."
print "SCons tools on this platform: ", commonEnvironment['TOOLS']
print

commonEnvironment.Append(LIBPATH = '#.')
commonEnvironment.Append(CPPFLAGS = "-DBETA")
if commonEnvironment['useDouble']:
    print 'Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = "-DUSE_DOUBLE")
else:
    print 'Using single-precision floating point for audio samples.'

# Define different build environments for different types of targets.

if getPlatform() == 'linux':
    commonEnvironment.Append(CCFLAGS = "-DLINUX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(LIBPATH = ['.', '#.', '/usr/lib', '/usr/local/lib'])
elif getPlatform() == 'darwin':
    commonEnvironment.Append(CCFLAGS = "-DMACOSX")
    commonEnvironment.Append(CPPPATH = '/usr/local/include')
    commonEnvironment.Append(CPPPATH = '/usr/include')
    commonEnvironment.Append(CCFLAGS = "-Wall")
    commonEnvironment.Append(CCFLAGS = "-DPIPES")
    commonEnvironment.Append(LIBPATH = ['.', '#.', '/usr/lib', '/usr/local/lib'])
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
    commonEnvironment.Append(LIBPATH = ['.', '#.', '/usr/include/lib', '/usr/local/lib'])    
    
if (commonEnvironment['makeDynamic'] == 0) and (getPlatform() != 'linux'):
    commonEnvironment.Append(LINKFLAGS = '-static')
else:
    commonEnvironment.Append(LINKFLAGS = Split('-Bdynamic --no-allow-shlib-undefined'))

# Adding libraries and flags if using -mno-cygwin with cygwin

if commonEnvironment['noCygwin'] and getPlatform() == 'cygwin':
    print 'Using -mno-cygwin.'
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

# Package contents.

zipfilename = "csoundvst-" + getPlatform() + "-" + str(today()) + ".zip"

def buildzip(env, target, source):

    os.chdir('..')
    directories = string.split("csound5")
    
    extensions = ".sln .csproj .vsproj .dev .def .am .sh .ac .in .dll .so .exe"
    extensions = extensions + ".htm .html .doc .mso .png .xml .mso .gif .jpg .jpeg .hlp .nb .wks .xls .pdf "
    extensions = extensions + ".c .C .cpp .cxx .h .hpp .H .hxx .py .rc .res .fl .i .java"
    extensions = extensions + ".sf2 .SF2 .csd .aif .aiff .jar .smf .mid"
    extensions = string.split(extensions)
    
    specificFiles = "SConstruct _CsoundVST.* _loris.* pyrun.* CsoundCOM.dll msvcp70.dll msvcr70.dll CsoundVST.exe CsoundVST.* soundfonts.dll "
    specificFiles = specificFiles + "README Doxyfile ChangeLog COPYING INSTALL MANIFEST COPYRIGHT AUTHORS TODO"
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
        print filename
    print
    print "Creating archive..."
    archive = zipfile.ZipFile("csound5/" + zipfilename, "w", zipfile.ZIP_DEFLATED)
    pathnames.sort()
    for filename in pathnames:
        print filename
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

if commonEnvironment['usePortAudio'] and portaudioFound:
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
        if commonEnvironment['useJack']:
            print "Adding Jack library for PortAudio"
            csoundProgramEnvironment.Append(LIBS = ['jack'])
            vstEnvironment.Append(LIBS = ['jack'])  
    elif getPlatform() == 'cygwin' or getPlatform() == 'mingw': 
        csoundProgramEnvironment.Append(LIBS = ['winmm'])
        vstEnvironment.Append(LIBS = ['winmm'])
        csoundProgramEnvironment.Append(LIBS = ['dsound'])
        vstEnvironment.Append(LIBS = ['dsound'])

    if commonEnvironment['useFLTK'] and fltkFound:
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
    
if getPlatform() == 'mingw':
    # These are the Windows system call libraries.
    csoundProgramEnvironment.Append(LIBS = ['kernel32'])
    csoundProgramEnvironment.Append(LIBS = ['gdi32'])
    csoundProgramEnvironment.Append(LIBS = ['wsock32'])
    csoundProgramEnvironment.Append(LIBS = ['ole32'])
    csoundProgramEnvironment.Append(LIBS = ['uuid'])

#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################


if commonEnvironment['generatePDF']:
    print 'Generating PDF documentation.'
    csoundPdf = commonEnvironment.Command('csound.pdf', 'csound.tex', 'pdflatex $SOURCE')

makedb = ustubProgramEnvironment.Program('makedb', 
    ['strings/makedb.c'])

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
OOps/ugens7.c
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
''')

if commonEnvironment['usePortAudio'] and portaudioFound:
    print 'Building with PortAudio.'
    libCsoundSources.append('InOut/rtpa.c')
    
if commonEnvironment['useFLTK'] and fltkFound:
    print 'Building with FLTK for graphs and widgets.'
    libCsoundSources.append('InOut/FL_graph.cpp')
    libCsoundSources.append('InOut/winFLTK.c')
    libCsoundSources.append('InOut/widgets.cpp')
    
staticLibrary = staticLibraryEnvironment.Library('csound', 
    libCsoundSources)
    
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

staticLibraryEnvironment.Library('ustub', 
    libUstubSources)

# Plugin opcodes.

pluginEnvironment.SharedLibrary('babo', 
    ['Opcodes/babo.c'])
pluginEnvironment.SharedLibrary('bbcut', 
    ['Opcodes/bbcut.c'])
pluginEnvironment.SharedLibrary('biquad', 
    ['Opcodes/biquad.c'])
pluginEnvironment.SharedLibrary('butter', 
    ['Opcodes/butter.c'])    
pluginEnvironment.SharedLibrary('clfilt', 
    ['Opcodes/clfilt.c'])
pluginEnvironment.SharedLibrary('cross2', 
    ['Opcodes/cross2.c'])
pluginEnvironment.SharedLibrary('dam', 
    ['Opcodes/dam.c'])
pluginEnvironment.SharedLibrary('dcblockr', 
    ['Opcodes/dcblockr.c'])
pluginEnvironment.SharedLibrary('filter', 
    ['Opcodes/filter.c'])
pluginEnvironment.SharedLibrary('flanger', 
    ['Opcodes/flanger.c'])
pluginEnvironment.SharedLibrary('follow', 
    ['Opcodes/follow.c'])
pluginEnvironment.SharedLibrary('grain', 
    ['Opcodes/grain.c'])
pluginEnvironment.SharedLibrary('grain4', 
    ['Opcodes/grain4.c'])
pluginEnvironment.SharedLibrary('hrtferX', 
    Split('''Opcodes/hrtferX.c 
    Top/natben.c'''))
pluginEnvironment.SharedLibrary('locsig', 
    ['Opcodes/locsig.c'])
pluginEnvironment.SharedLibrary('lowpassr', 
    ['Opcodes/lowpassr.c'])
pluginEnvironment.SharedLibrary('midiops2', 
    ['Opcodes/midiops2.c'])
pluginEnvironment.SharedLibrary('midiops3', 
    ['Opcodes/midiops3.c'])
pluginEnvironment.SharedLibrary('modal4', 
    Split('''Opcodes/modal4.c 
    Opcodes/physutil.c'''))
pluginEnvironment.SharedLibrary('nlfilt', 
    ['Opcodes/nlfilt.c'])
pluginEnvironment.SharedLibrary('oscbnk', 
    ['Opcodes/oscbnk.c'])
pluginEnvironment.SharedLibrary('phisem', 
    ['Opcodes/phisem.c'])
pluginEnvironment.SharedLibrary('physmod', 
    Split('''Opcodes/physmod.c 
    Opcodes/physutil.c 
    Opcodes/mandolin.c 
    Opcodes/singwave.c 
    Opcodes/fm4op.c 
    Opcodes/moog1.c 
    Opcodes/shaker.c 
    Opcodes/bowedbar.c'''))
pluginEnvironment.SharedLibrary('pitch', 
    Split('''Opcodes/pitch.c 
    Opcodes/pitch0.c 
    Opcodes/spectra.c'''))
pluginEnvironment.SharedLibrary('pluck', 
    ['Opcodes/pluck.c'])
pluginEnvironment.SharedLibrary('repluck', 
    ['Opcodes/repluck.c'])
pluginEnvironment.SharedLibrary('scansyn',  
    ['Opcodes/scansyn.c'])
pluginEnvironment.SharedLibrary('scansynx', 
    ['Opcodes/scansynx.c'])
pluginEnvironment.SharedLibrary('sfont', 
    ['Opcodes/sfont.c'])
pluginEnvironment.SharedLibrary('sndwarp', 
    ['Opcodes/sndwarp.c'])
pluginEnvironment.SharedLibrary('space', 
    ['Opcodes/space.c'])
pluginEnvironment.SharedLibrary('spat3d', 
    ['Opcodes/spat3d.c'])
pluginEnvironment.SharedLibrary('ugensa', 
    ['Opcodes/ugensa.c'])
pluginEnvironment.SharedLibrary('uggab', 
    ['Opcodes/uggab.c'])
pluginEnvironment.SharedLibrary('ugmoss', 
    ['Opcodes/ugmoss.c'])
pluginEnvironment.SharedLibrary('ugsc', 
    ['Opcodes/ugsc.c'])
pluginEnvironment.SharedLibrary('vdelayk', 
    ['Opcodes/vdelayk.c'])
pluginEnvironment.SharedLibrary('wave-terrain', 
    ['Opcodes/wave-terrain.c'])
   
    
# Plugins with External Dependencies

# FLUIDSYNTH OPCODES
if configure.CheckHeader("fluidsynth.h", language = "C"):        
       
    if getPlatform() == 'linux':
        fluidEnvironment = pluginEnvironment.Copy()
        fluidEnvironment.Append(LIBS = ['fluidsynth'])
        fluidEnvironment.SharedLibrary('fluidOpcodes', 
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp'])
    
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
        
        fluidEnvironment.SharedLibrary('fluidOpcodes', 
            ['Opcodes/fluidOpcodes/fluidOpcodes.cpp'])    
            
        fluidEnvironment.SharedLibrary('fluid', Split('''
            Opcodes/fluid/AudioEffect.cpp        
            Opcodes/fluid/audioeffectx.cpp        
            Opcodes/fluid/Soundfonts.cpp        
            Opcodes/fluid/SoundfontsMain.cpp        
            Opcodes/fluid/FluidsynthOpcode.cpp        
            '''))
        
# Utility programs.

csoundProgramEnvironment.Program('cscore', 
    ['util1/cscore/cscore_main.c'])
csoundProgramEnvironment.Program('cvanal', 
    ['anal/convol/cvl_main.c'])
csoundProgramEnvironment.Program('dnoise', 
    ['util2/dnoise.dir/dnoise_main.c'])
ustubProgramEnvironment.Program('envext', 
    ['util2/envext/envext.c'])
ustubProgramEnvironment.Program('extract', 
    ['util1/sortex/xmain.c'])
ustubProgramEnvironment.Program('extractor', 
    ['util2/mixer/xtrct.c'])
ustubProgramEnvironment.Program('het_export', 
    ['util2/exports/het_export.c'])
ustubProgramEnvironment.Program('het_import',  
    ['util2/exports/het_import.c'])
ustubProgramEnvironment.Program('hetro',  
    ['anal/adsyn/het_main.c'])
ustubProgramEnvironment.Program('lpanal',  
    ['anal/lpc/lpc_main.c'])
ustubProgramEnvironment.Program('lpc_export',  
    ['util2/exports/lpc_export.c'])
ustubProgramEnvironment.Program('lpc_import',  
    ['util2/exports/lpc_import.c'])
ustubProgramEnvironment.Program('mixer',  
    ['util2/mixer/mixer.c'])
ustubProgramEnvironment.Program('pv_export',   
    ['util2/exports/pv_export.c'])
ustubProgramEnvironment.Program('pv_import',  
    ['util2/exports/pv_import.c'])
csoundProgramEnvironment.Program('pvanal',  
    ['anal/pvoc/pvc_main.c'])
ustubProgramEnvironment.Program('pvlook',  
    ['util2/pvlook.dir/pvl_main.c'])
ustubProgramEnvironment.Program('scale',  
    ['util2/scale.dir/scale.c'])
ustubProgramEnvironment.Program('scot',  
    ['util1/scot/scot_main.c'])
ustubProgramEnvironment.Program('scsort',  
    ['util1/sortex/smain.c'])
sdif2ad = ustubProgramEnvironment.Program('sdif2ad',  
    Split('''SDIF/sdif2adsyn.c 
    SDIF/sdif.c 
    SDIF/sdif-mem.c'''))
ustubProgramEnvironment.Program('sndinfo', 
    ['util2/sndinfo/sndinfo_main.c'])
srconv = ustubProgramEnvironment.Program('srconv', 
    ['util2/dnoise.dir/srconv.c'])

# Front ends.

csoundProgramEnvironment.Program('csound', 
    ['frontends/csound/csound_main.c'])
    
if (commonEnvironment['buildCsoundVST'] == 1) and boostFound and fltkFound:    
    print 'Building CsoundVST plugin and standalone.'
    vstEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    vstEnvironment.Prepend(LIBS = ['csound', 'sndfile'])
    vstEnvironment.Append(SWIGFLAGS = Split('-python -c++ -includeall -verbose'))
    if getPlatform() == 'linux':
        vstEnvironment.Append(LIBS = ['swigpy', 'python2.3', 'util', 'dl', 'm'])
        vstEnvironment.Append(CPPPATH = ['/usr/local/include/python2.3'])
        vstEnvironment.Append(LIBPATH = ['/usr/local/lib/python2.3/config'])
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
    if getPlatform() == 'mingw' or getPlatform() == 'cygwin':
        Depends(csoundvst, pyrun)

    csoundvstGui = guiProgramEnvironment.Program('CsoundVST', ['frontends/CsoundVST/csoundvst_main.cpp']) 
    Depends(csoundvstGui, csoundvst)
        
    zip = commonEnvironment.Command(zipfilename, csoundvstGui, buildzip)
    Depends(zip, [csoundvstGui, srconv])
    
    copyPython = commonEnvironment.Install(['.'], ['frontends/CsoundVST/CsoundVST.py'])
    Depends(copyPython, csoundvst)
    Depends(copyPython, sdif2ad)

if commonEnvironment['generateTags'] == 1 and (getPlatform() == 'linux' or getPlatform() == 'cygwin'):
    print "Calling TAGS"
    allSources = string.join(glob.glob('*/*.h*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*.hpp'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.c*'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.h'))
    allSources = allSources + ' ' + string.join(glob.glob('*/*/*.hpp'))
    tags = commonEnvironment.Command('TAGS', Split(allSources), 'etags $SOURCES')
    Depends(tags, staticLibrary)

if commonEnvironment['generateXmg'] == 1:
    print "Calling makedb"
    if getPlatform() == 'mingw':
        xmgs = commonEnvironment.Command('American.xmg', ['strings/all_strings'], 'makedb strings/all_strings American')
        xmgs1 = commonEnvironment.Command('English.xmg', ['strings/english-strings'], 'makedb strings/english-strings English')
        xmgs2 = commonEnvironment.Command('csound.xmg', ['strings/english-strings'], 'makedb strings/english-strings csound')
    else:
        xmgs = commonEnvironment.Command('American.xmg', ['strings/all_strings'], './makedb strings/all_strings American')
        xmgs1 = commonEnvironment.Command('English.xmg', ['strings/english-strings'], './makedb strings/english-strings English')
        xmgs2 = commonEnvironment.Command('csound.xmg', ['strings/english-strings'], './makedb strings/english-strings csound')
    Depends(xmgs, makedb)
    Depends(xmgs1, makedb)
    Depends(xmgs2, makedb)
