print '''
C S O U N D   5

SCons build file for Csound 5: API library, opcodes, utilities, and frontends.
Author: Michael Gogins <gogins@pipeline.com>

'''
import os
import sys

#############################################################################
#
#   DEFINE CONFIGURATION
#
#############################################################################

# Define options for package configurations.

opts = Options()
opts.Add('PORTAUDIO', 'Set to 1 to use PortAudio for real-time audio input and output.', 1)
opts.Add('FLTK', 'Set to 1 to use FLTK for graphs and widget opcodes.', 1)
opts.Add('CSOUNDVST', 'Set to 1 to build CsoundVST (needs FLTK, Python 2.3, SWIG, boost).', 1)

# Define the common part of the build environment.

commonEnvironment = Environment(options = opts)
commonEnvironment.Append(CPPPATH  = ['.', './H'])

# Pick up some things from the local shell environment.
# This is used for Microsoft Visual Studio.

commonEnvironment['ENV']['TMP'] = os.environ['TMP']
commonEnvironment['ENV']['PATH'] = os.environ['PATH']
commonEnvironment['ENV']['INCLUDE'] = os.environ['INCLUDE']
commonEnvironment['ENV']['LIB'] = os.environ['LIB']
commonEnvironment.Append(LIBPATH = '.')

# Define flags for different platforms.

print 'Platform:', sys.platform
print
if sys.platform == 'cygwin' or sys.platform == 'mingw':
	commonEnvironment.Append(CCFLAGS = "-g")
	commonEnvironment.Append(CCFLAGS = "-O2")
	commonEnvironment.Append(CCFLAGS = "-Wall")
	commonEnvironment.Append(CCFLAGS = "-D_WIN32")
	commonEnvironment.Append(CCFLAGS = "-DWIN32")
	commonEnvironment.Append(CCFLAGS = "-DHAVE_STRING_H")
if sys.platform == 'win32':
	commonEnvironment.Append(CCFLAGS = "-DDEBUG")
	commonEnvironment.Append(CCFLAGS = "-D_WIN32")
	commonEnvironment.Append(CCFLAGS = "-DWIN32")
	commonEnvironment.Append(CCFLAGS = "-D_MSCVER")
	commonEnvironment.Append(CCFLAGS = "-DHAVE_STRING_H")
	commonEnvironment.Append(CCFLAGS = "-D__STDC__")
if sys.platform == 'linux1':
	commonEnvironment.Append(CCFLAGS = "-g")
	commonEnvironment.Append(CCFLAGS = "-O2")
	commonEnvironment.Append(CCFLAGS = "-Wall")
	
# Define different build environments for different types of targets.

staticLibraryEnvironment = commonEnvironment.Copy()
consoleProgramEnvironment = commonEnvironment.Copy()
consoleProgramEnvironment.Append(LIBS = "csound")
guiProgramEnvironment = commonEnvironment.Copy()
guiProgramEnvironment.Append(LIBS = "csound")
pluginEnvironment = commonEnvironment.Copy()
vstEnvironment = commonEnvironment.Copy()
vstEnvironment.Append(LIBS = "csound")
	
#############################################################################
#
#   DEFINE TARGETS AND SOURCES
#
#############################################################################

consoleProgramEnvironment.Program('makedb', ['strings/makedb.c'])

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
Engine/otran.c	\
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

libCsound_LIBS=''
libCsound_LIBPATH=''

if commonEnvironment['PORTAUDIO']:
    print 'Building with PortAudio.'
    libCsoundSources.append('InOut/rtpa.c')
    consoleProgramEnvironment.Append(LIBS = "portaudio")
	
if commonEnvironment['FLTK']:
    print 'Building with FLTK for graphs and widgets.'
    libCsoundSources.append('InOut/winFLTK.c')
    libCsoundSources.append('InOut/FL_graph.cpp')
    libCsoundSources.append('InOut/widgets.cpp')
    consoleProgramEnvironment.Append(LIBS = "fltk")

if commonEnvironment['CSOUNDVST']:
    print 'Building CsoundVST plugin and standalone.'
    
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
InOut/FL_graph.cpp 
InOut/ieee80.c 
InOut/libsnd.c 
InOut/sfheader.c 
InOut/soundin.c 
InOut/soundio.c 
InOut/ulaw.c 
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

consoleProgramEnvironment.Program('cscore', 
    ['util1/cscore/cscore_main.c'])
consoleProgramEnvironment.Program('cvanal', 
    ['anal/convol/cvl_main.c'])
consoleProgramEnvironment.Program('dnoise', 
    ['util2/dnoise.dir/dnoise_main.c'])
consoleProgramEnvironment.Program('envext', 
    ['util2/envext/envext.c'])
consoleProgramEnvironment.Program('extract', 
    ['util1/sortex/xmain.c'])
consoleProgramEnvironment.Program('extractor', 
    ['util2/mixer/xtrct.c'])
consoleProgramEnvironment.Program('het_export', 
    ['util2/exports/het_export.c'])
consoleProgramEnvironment.Program('het_import',  
    ['util2/exports/het_import.c'])
consoleProgramEnvironment.Program('hetro',  
    ['anal/adsyn/het_main.c'])
consoleProgramEnvironment.Program('lpanal',  
    ['anal/lpc/lpc_main.c'])
consoleProgramEnvironment.Program('lpc_export',  
    ['util2/exports/lpc_export.c'])
consoleProgramEnvironment.Program('lpc_import',  
    ['util2/exports/lpc_import.c'])
consoleProgramEnvironment.Program('mixer',  
    ['util2/mixer/mixer.c'])
consoleProgramEnvironment.Program('pv_export',   
    ['util2/exports/pv_export.c'])
consoleProgramEnvironment.Program('pv_import',  
    ['util2/exports/pv_import.c'])
consoleProgramEnvironment.Program('pvanal',  
    ['anal/pvoc/pvc_main.c'])
consoleProgramEnvironment.Program('pvlook',  
    ['util2/pvlook.dir/pvl_main.c'])
consoleProgramEnvironment.Program('scale',  
    ['util2/scale.dir/scale.c'])
consoleProgramEnvironment.Program('scot',  
    ['util1/scot/scot_main.c'])
consoleProgramEnvironment.Program('scsort',  
    ['util1/sortex/smain.c'])
consoleProgramEnvironment.Program('sdif2ad',  
    Split('''SDIF/sdif2adsyn.c 
    SDIF/sdif.c 
    SDIF/sdif.h 
    SDIF/sdif-mem.c 
    SDIF/sdif-mem.h '''))
consoleProgramEnvironment.Program('sndinfo', 
    ['util2/sndinfo/sndinfo_main.c'])
consoleProgramEnvironment.Program('srconv', 
    ['util2/dnoise.dir/srconv.c'])

# Front ends.

