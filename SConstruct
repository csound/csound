print '''
C S O U N D   5

SCons build file for Csound 5: 
API library, opcodes, utilities, and front ends.

By Michael Gogins <gogins at pipeline dot com>

For custom options, run 'scons -h'.
For default options, run 'scons -H'.
If headers or libraries are not found, edit 'custom.py'.
For MinGW, make sure its bin directory is in %PATH%.
For Microsoft Visual C++, run 'scons' in the Visual Studio .NET command prompt
    and use www.python.org WIN32 Python.
For Cygwin, run SCons.bat in the Python directory 
    and use Cygwin Python.
'''
import os
import sys
import string

#############################################################################
#
#   DEFINE CONFIGURATION
#
#############################################################################

# Detect platform.

print "Platform is '" + sys.platform + "'."
print

# Define configuration options.

opts = Options('custom.py')
opts.Add('useDouble', 
    'Set to 1 to use double-precision floating point for audio samples.', 
    0)
opts.Add('usePortAudio', 
    'Set to 1 to use PortAudio for real-time audio input and output.', 
    1)
opts.Add('useFLTK', 
    'Set to 1 to use FLTK for graphs and widget opcodes.', 
    1)
opts.Add('buildCsoundVST', 
    'Set to 1 to build CsoundVST (needs FLTK, boost, Python 2.3, SWIG).', 
    1)
opts.Add('useMingw', 
    'Set to 1 to use mingw on Windows.', 
    0)

# Define the common part of the build environment.

commonEnvironment = Environment(options = opts)
if commonEnvironment['useMingw'] and (sys.platform == 'win32' or sys.platform == 'cygwin'):
    print 'Using mingw.'
    commonEnvironment.Append(tools = ['mingw'])
    
Help(opts.GenerateHelpText(commonEnvironment))

commonEnvironment.Append(CPPPATH  = ['.', './H'])
commonEnvironment.Append(CCFLAGS = '-DCSOUND_WITH_API')
# Define options for different platforms.

if sys.platform == 'linux1':
	commonEnvironment.Append(CCFLAGS = "-DLINUX")
	commonEnvironment.Append(CPPPATH = '/usr/local/include')
	commonEnvironment.Append(CPPPATH = '/usr/include')
	commonEnvironment.Append(CCFLAGS = "-g")
	commonEnvironment.Append(CCFLAGS = "-O2")
	commonEnvironment.Append(CCFLAGS = "-Wall")
	commonEnvironment.Append(CCFLAGS = "-DPIPES")
	commonEnvironment.Append(LIBPATH = ['.', '#.', '/usr/include/lib', '/usr/local/lib'])
if sys.platform == 'cygwin' or sys.platform == 'mingw':
	commonEnvironment.Append(CPPPATH = '/usr/local/include')
	commonEnvironment.Append(CPPPATH = '/usr/include')
	commonEnvironment.Append(CCFLAGS = "-g")
	commonEnvironment.Append(CCFLAGS = "-O2")
	commonEnvironment.Append(CCFLAGS = "-Wall")
	commonEnvironment.Append(CCFLAGS = "-D_WIN32")
	commonEnvironment.Append(CCFLAGS = "-DWIN32")
	commonEnvironment.Append(CCFLAGS = "-DHAVE_STRING_H")
	commonEnvironment.Append(CCFLAGS = "-DPIPES")
	commonEnvironment.Append(LIBPATH = ['.', '#.', '/usr/include/lib', '/usr/local/lib'])
if sys.platform == 'win32':
	commonEnvironment.Append(CCFLAGS = "-DDEBUG")
	commonEnvironment.Append(CCFLAGS = "-D_WIN32")
	commonEnvironment.Append(CCFLAGS = "-DWIN32")
	commonEnvironment.Append(CCFLAGS = "-D_MSCVER")
	commonEnvironment.Append(CCFLAGS = "-DHAVE_STRING_H")
	commonEnvironment.Append(CCFLAGS = "-D__STDC__")

# Pick up some things from the local shell's environment.
# This is used for Microsoft Visual Studio.

commonEnvironment['ENV']['TMP'] = os.environ['TMP']
commonEnvironment['ENV']['PATH'] = os.environ['PATH']
commonEnvironment['ENV']['INCLUDE'] = os.environ['INCLUDE']
commonEnvironment['ENV']['LIB'] = os.environ['LIB']
commonEnvironment.Append(LIBPATH = '#.')
commonEnvironment.Append(CPPFLAGS = "-DBETA")
if commonEnvironment['useDouble']:
    print 'Using double-precision floating point for audio samples.'
    commonEnvironment.Append(CPPFLAGS = "-DUSE_DOUBLE")
else:
    print 'Using single-precision floating point for audio samples.'

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

# Define macros that configure and config.h used to define.

if configure.CheckHeader("io.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_IO_H')
if configure.CheckHeader("fcntl.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_FCNTL_H')
if configure.CheckHeader("unistd.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_UNISTED_H')
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
if configure.CheckHeader("term/ios.h", language = "C"):
    commonEnvironment.Append(CCFLAGS = '-DHAVE_TERM_IOS_H')

# Define different build environments for different types of targets.

staticLibraryEnvironment = commonEnvironment.Copy()

pluginEnvironment = commonEnvironment.Copy()

csoundProgramEnvironment = commonEnvironment.Copy()
csoundProgramEnvironment.Append(LIBS = ['csound', 'sndfile'])

ustubProgramEnvironment = commonEnvironment.Copy()
ustubProgramEnvironment.Append(LIBS = ['ustub', 'sndfile'])

vstEnvironment = commonEnvironment.Copy()

guiProgramEnvironment = commonEnvironment.Copy()

if commonEnvironment['usePortAudio'] and portaudioFound:
    staticLibraryEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    pluginEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    csoundProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    ustubProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    vstEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    guiProgramEnvironment.Append(CCFLAGS = '-DRTAUDIO')
    csoundProgramEnvironment.Append(LIBS = ['portaudio'])
    vstEnvironment.Append(LIBS = ['portaudio'])
    if sys.platform == 'linux1': 
        csoundProgramEnvironment.Append(LIBS = ['asound'])
        vstEnvironment.Append(LIBS = ['asound'])
    if sys.platform == 'cygwin' or sys.platform == 'mingw' or sys.platform == 'win32': 
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
        vstEnvironment.Append(LIBS = ['fltk'])
        if sys.platform == 'linux1' or sys.platform == 'cygwin' or sys.platform == 'mingw':
            csoundProgramEnvironment.Append(LIBS = ['stdc++', 'pthread'])
            ustubProgramEnvironment.Append(LIBS = ['stdc++', 'pthread'])
            vstEnvironment.Append(LIBS = ['stdc++', 'pthread'])
            guiProgramEnvironment.Append(LIBS = ['stdc++', 'pthread'])
 	
#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

ustubProgramEnvironment.Program('makedb', 
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
InOut/ieee80.c
InOut/libsnd.c
InOut/libsnd_u.c
InOut/sfheader.c
InOut/soundin.c
InOut/soundio.c
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
    libCsoundSources.append('InOut/winFLTK.c')
    libCsoundSources.append('InOut/FL_graph.cpp')
    libCsoundSources.append('InOut/widgets.cpp')
    
staticLibraryEnvironment.Library('csound', 
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
InOut/ieee80.c 
InOut/libsnd.c 
InOut/libsnd_u.c
InOut/sfheader.c 
InOut/soundin.c 
InOut/soundio.c 
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
pluginEnvironment.SharedLibrary('wave-terrain', 
    ['Opcodes/wave-terrain.c'])

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
ustubProgramEnvironment.Program('sdif2ad',  
    Split('''SDIF/sdif2adsyn.c 
    SDIF/sdif.c 
    SDIF/sdif-mem.c'''))
ustubProgramEnvironment.Program('sndinfo', 
    ['util2/sndinfo/sndinfo_main.c'])
ustubProgramEnvironment.Program('srconv', 
    ['util2/dnoise.dir/srconv.c'])

# Front ends.

csoundProgramEnvironment.Program('csound', 
    ['frontends/csound/csound_main.c'])
    
if commonEnvironment['buildCsoundVST'] and boostFound and fltkFound:
    print 'Building CsoundVST plugin and standalone.'
    vstEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    guiProgramEnvironment.Append(CPPPATH = ['frontends/CsoundVST'])
    vstEnvironment.Prepend(LIBS = ['csound', 'sndfile'])
    guiProgramEnvironment.Prepend(LIBS = ['_CsoundVST'])
    if sys.platform == 'linux1':
    	vstEnvironment.Append(LIBS = ['python2.3'])
    if sys.platform == 'cygwin':
        # vstEnvironment.Append(CPPPATH = ['/usr/include/python2.3'])
        vstEnvironment.Prepend(CPPPATH = ['c:/Python23/include'])
    	vstEnvironment.Append(CCFLAGS = '-D_MSC_VER')
    	# vstEnvironment.Append(LIBS = ['python2.3'])
    	vstEnvironment.Append(LIBS = ['python23'])
    	# vstEnvironment.Prepend(LIBPATH = '/lib/python2.3/config')
    	vstEnvironment.Prepend(LIBPATH = 'c:/Python23/libs')
    	pyrunEnvironment = vstEnvironment.Copy()
    	pyrunEnvironment.Append(CCFLAGS = '-DSWIG_GLOBAL')
    	pyrun = pyrunEnvironment.SharedLibrary('pyrun', ['frontends/CsoundVST/pyrun.c'])
    	vstEnvironment.Append(SWIGFLAGS = Split('-python -c++ -c -includeall -verbose'))
    	if sys.platform == 'cygwin' or sys.platform == 'mingw':
    	     vstEnvironment.Append(LIBS = ['fltk_images'])
    	     vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
    	     vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
    	     guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    	for option in vstEnvironment['CPPPATH']:
    	     option = '-I' + option
    	     vstEnvironment.Append(SWIGFLAGS = [option])
     	for option in vstEnvironment['CCFLAGS']:
    	     if string.find(option, '-D') == 0:
                 vstEnvironment.Append(SWIGFLAGS = [option])
    	vstEnvironment.Append(LIBS = ['pyrun'])
    if sys.platform == 'mingw' or sys.platform == 'win32':
        vstEnvironment.Prepend(CPPPATH = ['c:/Python23/include'])
    	vstEnvironment.Append(CCFLAGS = '-D_MSC_VER')
    	vstEnvironment.Append(LIBS = ['python23'])
    	vstEnvironment.Prepend(LIBPATH = 'c:/Python23/libs')
    	pyrunEnvironment = vstEnvironment.Copy()
    	pyrunEnvironment.Append(CCFLAGS = '-DSWIG_GLOBAL')
    	pyrun = pyrunEnvironment.SharedLibrary('pyrun', ['frontends/CsoundVST/pyrun.c'])
    	vstEnvironment.Append(SWIGFLAGS = Split('-python -c++ -c -includeall -verbose'))
    	if sys.platform == 'cygwin' or sys.platform == 'mingw':
    	     vstEnvironment.Append(LIBS = ['fltk_images'])
    	     vstEnvironment.Append(SHLINKFLAGS = '--no-export-all-symbols')
    	     vstEnvironment.Append(SHLINKFLAGS = '--add-stdcall-alias')
    	     guiProgramEnvironment.Append(LINKFLAGS = '-mwindows')
    	for option in vstEnvironment['CPPPATH']:
    	     option = '-I' + option
    	     vstEnvironment.Append(SWIGFLAGS = [option])
     	for option in vstEnvironment['CCFLAGS']:
    	     if string.find(option, '-D') == 0:
                 vstEnvironment.Append(SWIGFLAGS = [option])
    	vstEnvironment.Append(LIBS = ['pyrun'])

    csoundVstSources = Split('''
    frontends/CsoundVST/AudioEffect.cpp 
    frontends/CsoundVST/audioeffectx.cpp 
    frontends/CsoundVST/Composition.cpp 
    frontends/CsoundVST/Conversions.cpp 
    frontends/CsoundVST/CppSound.cpp 
    frontends/CsoundVST/CsoundFile.cpp 
    frontends/CsoundVST/Cell.cpp 
    frontends/CsoundVST/CsoundVST.cpp 
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
    csoundvst = vstEnvironment.SharedLibrary('CsoundVST', csoundVstSources, SHLIBPREFIX = '_')
    Depends(csoundvst, pyrun)

    guiProgramEnvironment.Program('CsoundVST', ['frontends/CsoundVST/csoundvst_main.cpp']) 

