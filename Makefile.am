# Csound uses this one master Makefile.am 
# to build and install all targets on all autotools platforms.
# All targets are specified below in order of increasing dependency.
# All configuration questions are asked and answered in configure.ac, 
# and all configuration results are implemented in config.h and in this file.
# Unless you are adding new library dependencies or new platforms to the Csound build,
# this is the only build system file that you should need to modify.

############################
# 
#	ALL TARGETS
#
############################

#	The makedb program for compiling xmg files:

bin_PROGRAMS=makedb

#	The Csound engine and API library:

lib_LTLIBRARIES=libcsound.la

#	Plugin opcodes 
#	(note that they are "modules" to dlopen, not "shared libraries" to link):

pkglib_LTLIBRARIES=babo.la
pkglib_LTLIBRARIES+=bbcut.la
pkglib_LTLIBRARIES+=biquad.la
pkglib_LTLIBRARIES+=butter.la
pkglib_LTLIBRARIES+=clfilt.la
pkglib_LTLIBRARIES+=cross2.la
pkglib_LTLIBRARIES+=dam.la
pkglib_LTLIBRARIES+=dcblock.la
pkglib_LTLIBRARIES+=filter.la
pkglib_LTLIBRARIES+=flanger.la
pkglib_LTLIBRARIES+=follow.la
pkglib_LTLIBRARIES+=grain.la
pkglib_LTLIBRARIES+=grain4.la
pkglib_LTLIBRARIES+=hrtferX.la
pkglib_LTLIBRARIES+=locsig.la
pkglib_LTLIBRARIES+=lowpassr.la
pkglib_LTLIBRARIES+=midiops2.la
pkglib_LTLIBRARIES+=midiops3.la
pkglib_LTLIBRARIES+=modal4.la
pkglib_LTLIBRARIES+=nlfilt.la
pkglib_LTLIBRARIES+=oscbnk.la
pkglib_LTLIBRARIES+=phisem.la
pkglib_LTLIBRARIES+=physmod.la
pkglib_LTLIBRARIES+=pitch.la
pkglib_LTLIBRARIES+=pluck.la
pkglib_LTLIBRARIES+=repluck.la
pkglib_LTLIBRARIES+=scansyn.la
pkglib_LTLIBRARIES+=scansynx.la
pkglib_LTLIBRARIES+=sfont.la
pkglib_LTLIBRARIES+=sndwarp.la
pkglib_LTLIBRARIES+=space.la
pkglib_LTLIBRARIES+=spat3d.la
pkglib_LTLIBRARIES+=ugensa.la
pkglib_LTLIBRARIES+=uggab.la
pkglib_LTLIBRARIES+=ugmoss.la
pkglib_LTLIBRARIES+=ugsc.la
pkglib_LTLIBRARIES+=wave_terrain.la

#	User interfaces for Csound:

bin_PROGRAMS+=csound

# bin_PROGRAMS+=csoundgui
# Note that _csoundvst is a "module" to dlopen, not a "shared library" to link...
# lib_LTLIBRARIES+=_csoundvst.la
# ...and the standalone csoundvst always uses dlopen to run _csoundvst.
# bin_PROGRAMS+=csoundvst

#	Convenience library for utility programs:

lib_LTLIBRARIES+=libustub.la

#	Utility programs:

bin_PROGRAMS+=cscore
bin_PROGRAMS+=cvanal
bin_PROGRAMS+=dnoise
bin_PROGRAMS+=envext
bin_PROGRAMS+=extract
bin_PROGRAMS+=extractor
bin_PROGRAMS+=het_export
bin_PROGRAMS+=het_import
bin_PROGRAMS+=hetro
bin_PROGRAMS+=lpanal
bin_PROGRAMS+=lpc_export
bin_PROGRAMS+=lpc_import
bin_PROGRAMS+=mixer
bin_PROGRAMS+=pv_export 
bin_PROGRAMS+=pv_import 
bin_PROGRAMS+=pvanal
bin_PROGRAMS+=pvlook
bin_PROGRAMS+=scale
bin_PROGRAMS+=scot
bin_PROGRAMS+=scsort
bin_PROGRAMS+=sdif2ad
bin_PROGRAMS+=sndinfo
bin_PROGRAMS+=srconv


#	Extra files and scripts:

EXTRA_DIST=csound.spec Doxyfile

############################
# 
#	ALL SOURCES AND RULES
#
############################

#	Note that conditionals (AM_CONDITIONAL) must be used for source variables,
#	but substititions (AC_SUBST) can be used for all other variables.

#	The makedb program for compiling xmg files:

makedb_SOURCES=strings/makedb.c

xmg:	American.xmg English.xmg csound.xmg

American.xmg:	strings/all_strings makedb
	./makedb strings/all_strings American

English.xmg:	strings/english-strings makedb
	./makedb strings/english-strings English

csound.xmg:	strings/all_strings English.xmg
	cp English.xmg csound.xmg

#	The Csound engine and API library

EXTRA_libcsound_la_SOURCES=\
InOut/rtpa.c

libcsound_la_SOURCES=\
Engine/auxfd.c \
Engine/entry1.c \
Engine/entry2.c \
Engine/express.c \
Engine/extract.c \
Engine/fgens.c \
Engine/filopen.c \
Engine/insert.c \
Engine/linevent.c \
Engine/memalloc.c \
Engine/memfiles.c \
Engine/musmon.c \
Engine/namedins.c \
Engine/oload.c \
Engine/otran.c	\
Engine/rdorch.c \
Engine/rdscor.c \
Engine/scsort.c \
Engine/scxtract.c \
Engine/sort.c \
Engine/sread.c \
Engine/swrite.c \
Engine/twarp.c \
H/3Dug.h \
H/aiff.h \
H/aops.h \
H/BeAudio.h \
H/cmath.h \
H/complex.h \
H/config.h \
H/control.h \
H/convolve.h \
H/cs.h \
H/cscore.h \
H/csdl.h \
H/csgui.h \
H/CSliders.h \
H/csound.h \
H/csoundCore.h \
H/CsoundLib.h \
H/cwindow.h \
H/cwindows.h \
H/DECaudio.h \
H/DECplay.h \
H/diskin.h \
H/disprep.h \
H/dn_res.h \
H/dpwelib.h \
H/dsputil.h \
H/dumpf.h \
H/effects.h \
H/fft.h \
H/fhtfun.h \
H/foo.h \
H/fout.h \
H/ftgen.h \
H/fwguide.h \
H/ieee80.h \
H/insert.h \
H/lipfilt.h \
H/lpc.h \
H/midiinterop.h \
H/midioops.h \
H/midiops.h \
H/midiops2.h \
H/midiout.h \
H/mw_misc.h \
H/namedins.h \
H/noise.h \
H/nshakers.h \
H/oload.h \
H/onepole.h \
H/onezero.h \
H/OpcLoad.h \
H/opcode.h \
H/oscils.h \
H/OSC-sock.h \
H/pitch.h \
H/prototyp.h \
H/pstream.h \
H/pvadd.h \
H/pvdefs.h \
H/pvfileio.h \
H/pvinterp.h \
H/pvoc.h \
H/pvocext.h \
H/pvread.h \
H/pvxanal.h \
H/resource.h \
H/revsets.h \
H/rtNetBSD.h \
H/schedule.h \
H/scot.h \
H/sdif.h \
H/sdif-mem.h \
H/sf.h \
H/sfheader.h \
H/sftype.h \
H/slider.h \
H/sndinfUG.h \
H/solarisAudio.h \
H/sort.h \
H/soundio.h \
H/spectra.h \
H/sysdep.h \
H/text.h \
H/typetabl.h \
H/ugens1.h \
H/ugens2.h \
H/ugens3.h \
H/ugens4.h \
H/ugens5.h \
H/ugens6.h \
H/ugens7.h \
H/ugens8.h \
H/ugens9.h \
H/uggab.h \
H/ugmoss.h \
H/ugsc.h \
H/ustub.h \
H/vbap.h \
H/vdelay.h \
H/version.h \
H/vpvoc.h \
H/wave.h \
H/wguide.h \
H/widgets.h \
H/windin.h \
H/winEPS.h \
InOut/aifc.c \
InOut/aiff.c \
InOut/ieee80.c \
InOut/libsnd.c \
InOut/sfheader.c \
InOut/soundin.c \
InOut/soundio.c \
InOut/wave.c \
InOut/winascii.c \
InOut/windin.c \
InOut/window.c \
InOut/winEPS.c \
OOps/aops.c \
OOps/cmath.c \
OOps/control.c \
OOps/diskin.c \
OOps/disprep.c \
OOps/dnfft.c \
OOps/dsputil.c \
OOps/dumpf.c \
OOps/fft.c \
OOps/fout.c \
OOps/fprint.c \
OOps/lptrkfns.c \
OOps/mididevice.c \
OOps/midiinterop.c \
OOps/midiops.c \
OOps/midiout.c \
OOps/midirecv.c \
OOps/midisend.c \
OOps/mxfft.c \
OOps/oscils.c \
OOps/pstream.c \
OOps/pvadd.c \
OOps/pvfileio.c \
OOps/pvinterp.c \
OOps/pvoc.c \
OOps/pvocext.c \
OOps/pvread.c \
OOps/pvsanal.c \
OOps/pvxanal.c \
OOps/schedule.c \
OOps/sdif.c \
OOps/sdif-mem.c \
OOps/sndinfUG.c \
OOps/ugens1.c \
OOps/ugens2.c \
OOps/ugens3.c \
OOps/ugens4.c \
OOps/ugens5.c \
OOps/ugens6.c \
OOps/ugens7.c \
OOps/ugens8.c \
OOps/ugens9.c \
OOps/ugrw1.c \
OOps/ugrw2.c \
OOps/vbap.c \
OOps/vbap_eight.c \
OOps/vbap_four.c \
OOps/vbap_sixteen.c \
OOps/vbap_zak.c \
OOps/vdelay.c \
OOps/vpvoc.c \
Top/argdecode.c \
Top/cscore_internal.c \
Top/cscorfns.c \
Top/csound.c \
Top/cvanal.c \
Top/dl_opcodes.c \
Top/dnoise.c \
Top/getstring.c \
Top/hetro.c \
Top/load_opcodes.c \
Top/lpanal.c \
Top/main.c \
Top/one_file.c \
Top/opcode.c \
Top/pvanal.c \
Top/pvlook.c \
Top/scot.c \
Top/sndinfo.c	

libcsound_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@
libcsound_la_CFLAGS=@LIBCSOUND_CFLAGS@
libcsound_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@
libcsound_la_LIBADD=@LIBCSOUND_LIBADD@
libcsound_la_LDFLAGS=@LIBCSOUND_LDFLAGS@

if CONDITIONAL_SGI
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtSGI.c InOut/SGIplay.c
endif
endif

if CONDITIONAL_sun
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtSUN.c
endif
endif

if CONDITIONAL_DEC
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtDEC.c InOut/DECaudio.c InOut/DECplay.c H/DECplay.h
endif
endif

if CONDITIONAL_HP
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtDEC.c InOut/DECaudio.c InOut/DECplay.c H/DECplay.h
endif
endif

if CONDITIONAL_WIN32
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtpa.c
endif
endif

if CONDITIONAL_LINUX
if CONDITIONAL_USE_RTAUDIO
libcsound_la_SOURCES+=InOut/rtpa.c
endif
endif

if CONDITIONAL_WINDOWS
if CONDITIONAL_USE_FLTK
libcsound_la_SOURCES+=InOut/winFLTK.c InOut/FL_graph.cpp 
if CONDITIONAL_USE_FLTK_WIDGETS
libcsound_la_SOURCES+=InOut/widgets.cpp
endif
endif
if CONDITIONAL_USE_X11
libcsound_la_SOURCES+=InOut/winX11.c
endif
endif

#	User interfaces for Csound:

csound_SOURCES=frontends/csound/csound_main.c
csound_DEPENDENCIES=xmg libcsound.la
csound_CPPFLAGS=@CSOUND_CPPFLAGS@
csound_CFLAGS=@CSOUND_CFLAGS@
csound_CXXFLAGS=@CSOUND_CXXFLAGS@
csound_LDFLAGS=@CSOUND_LDFLAGS@
csound_LDADD=@CSOUND_LDADD@

#	Plugin opcodes 
#	(note that they are "modules" to dlopen, not "shared libraries" to link):

babo_la_SOURCES=Opcodes/babo.c
babo_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
babo_la_CFLAGS=@PLUGIN_CFLAGS@
babo_la_LDFLAGS=@PLUGIN_LDFLAGS@
babo_la_LIBADD=@PLUGIN_LIBADD@

bbcut_la_SOURCES=Opcodes/bbcut.c
bbcut_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
bbcut_la_CFLAGS=@PLUGIN_CFLAGS@
bbcut_la_LDFLAGS=@PLUGIN_LDFLAGS@
bbcut_la_LIBADD=@PLUGIN_LIBADD@

biquad_la_SOURCES=Opcodes/biquad.c
biquad_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
biquad_la_CFLAGS=@PLUGIN_CFLAGS@
biquad_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
biquad_la_LIBADD=@PLUGIN_LIBADD@

butter_la_SOURCES=Opcodes/butter.c
butter_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
butter_la_CFLAGS=@PLUGIN_CFLAGS@
butter_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
butter_la_LIBADD=@PLUGIN_LIBADD@

clfilt_la_SOURCES=Opcodes/clfilt.c
clfilt_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
clfilt_la_CFLAGS=@PLUGIN_CFLAGS@
clfilt_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
clfilt_la_LIBADD=@PLUGIN_LIBADD@

cross2_la_SOURCES=Opcodes/cross2.c
cross2_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
cross2_la_CFLAGS=@PLUGIN_CFLAGS@
cross2_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
cross2_la_LIBADD=@PLUGIN_LIBADD@

dam_la_SOURCES=Opcodes/dam.c
dam_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
dam_la_CFLAGS=@PLUGIN_CFLAGS@
dam_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
dam_la_LIBADD=@PLUGIN_LIBADD@

dcblock_la_SOURCES=Opcodes/dcblockr.c
dcblock_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
dcblock_la_CFLAGS=@PLUGIN_CFLAGS@
dcblock_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
dcblock_la_LIBADD=@PLUGIN_LIBADD@

filter_la_SOURCES=Opcodes/filter.c
filter_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
filter_la_CFLAGS=@PLUGIN_CFLAGS@
filter_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
filter_la_LIBADD=@PLUGIN_LIBADD@

flanger_la_SOURCES=Opcodes/flanger.c
flanger_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
flanger_la_CFLAGS=@PLUGIN_CFLAGS@
flanger_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
flanger_la_LIBADD=@PLUGIN_LIBADD@

follow_la_SOURCES=Opcodes/follow.c
follow_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
follow_la_CFLAGS=@PLUGIN_CFLAGS@
follow_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
follow_la_LIBADD=@PLUGIN_LIBADD@

grain_la_SOURCES=Opcodes/grain.c
grain_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
grain_la_CFLAGS=@PLUGIN_CFLAGS@
grain_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
grain_la_LIBADD=@PLUGIN_LIBADD@

grain4_la_SOURCES=Opcodes/grain4.c
grain4_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
grain4_la_CFLAGS=@PLUGIN_CFLAGS@
grain4_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
grain4_la_LIBADD=@PLUGIN_LIBADD@

hrtferX_la_SOURCES=Opcodes/hrtferX.c \
Top/natben.c
hrtferX_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
hrtferX_la_CFLAGS=@PLUGIN_CFLAGS@
hrtferX_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
hrtferX_la_LIBADD=@PLUGIN_LIBADD@

locsig_la_SOURCES=Opcodes/locsig.c
locsig_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
locsig_la_CFLAGS=@PLUGIN_CFLAGS@
locsig_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
locsig_la_LIBADD=@PLUGIN_LIBADD@

lowpassr_la_SOURCES=Opcodes/lowpassr.c
lowpassr_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
lowpassr_la_CFLAGS=@PLUGIN_CFLAGS@
lowpassr_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
lowpassr_la_LIBADD=@PLUGIN_LIBADD@

midiops2_la_SOURCES=Opcodes/midiops2.c
midiops2_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
midiops2_la_CFLAGS=@PLUGIN_CFLAGS@
midiops2_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
midiops2_la_LIBADD=@PLUGIN_LIBADD@

midiops3_la_SOURCES=Opcodes/midiops3.c
midiops3_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
midiops3_la_CFLAGS=@PLUGIN_CFLAGS@
midiops3_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
midiops3_la_LIBADD=@PLUGIN_LIBADD@

modal4_la_SOURCES=Opcodes/modal4.c \
Opcodes/physutil.c 
modal4_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
modal4_la_CFLAGS=@PLUGIN_CFLAGS@
modal4_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
modal4_la_LIBADD=@PLUGIN_LIBADD@

nlfilt_la_SOURCES=Opcodes/nlfilt.c
nlfilt_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
nlfilt_la_CFLAGS=@PLUGIN_CFLAGS@
nlfilt_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
nlfilt_la_LIBADD=@PLUGIN_LIBADD@

oscbnk_la_SOURCES=Opcodes/oscbnk.c
oscbnk_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
oscbnk_la_CFLAGS=@PLUGIN_CFLAGS@
oscbnk_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
oscbnk_la_LIBADD=@PLUGIN_LIBADD@

phisem_la_SOURCES=Opcodes/phisem.c
phisem_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
phisem_la_CFLAGS=@PLUGIN_CFLAGS@
phisem_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
phisem_la_LIBADD=@PLUGIN_LIBADD@

physmod_la_SOURCES=Opcodes/physmod.c \
Opcodes/physutil.c \
Opcodes/mandolin.c \
Opcodes/singwave.c \
Opcodes/fm4op.c \
Opcodes/moog1.c \
Opcodes/shaker.c \
Opcodes/bowedbar.c
physmod_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
physmod_la_CFLAGS=@PLUGIN_CFLAGS@
physmod_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
physmod_la_LIBADD=@PLUGIN_LIBADD@

pitch_la_SOURCES=Opcodes/pitch.c \
Opcodes/pitch0.c \
Opcodes/spectra.c
pitch_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
pitch_la_CFLAGS=@PLUGIN_CFLAGS@
pitch_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
pitch_la_LIBADD=@PLUGIN_LIBADD@

pluck_la_SOURCES=Opcodes/pluck.c
pluck_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
pluck_la_CFLAGS=@PLUGIN_CFLAGS@
pluck_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
pluck_la_LIBADD=@PLUGIN_LIBADD@

repluck_la_SOURCES=Opcodes/repluck.c
repluck_CPPFLAGS=@PLUGIN_CPPFLAGS@
repluck_CFLAGS=@PLUGIN_CFLAGS@
repluck_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
repluck_la_LIBADD=@PLUGIN_LIBADD@

scansyn_la_SOURCES=Opcodes/scansyn.c
scansyn_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
scansyn_la_CFLAGS=@PLUGIN_CFLAGS@
scansyn_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
scansyn_la_LIBADD=@PLUGIN_LIBADD@

scansynx_la_SOURCES=Opcodes/scansynx.c
scansynx_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
scansynx_la_CFLAGS=@PLUGIN_CFLAGS@
scansynx_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
scansynx_la_LIBADD=@PLUGIN_LIBADD@

sfont_la_SOURCES=Opcodes/sfont.c
sfont_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
sfont_la_CFLAGS=@PLUGIN_CFLAGS@
sfont_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
sfont_la_LIBADD=@PLUGIN_LIBADD@

sndwarp_la_SOURCES=Opcodes/sndwarp.c
sndwarp_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
sndwarp_la_CFLAGS=@PLUGIN_CFLAGS@
sndwarp_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
sndwarp_la_LIBADD=@PLUGIN_LIBADD@

space_la_SOURCES=Opcodes/space.c
space_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
space_la_CFLAGS=@PLUGIN_CFLAGS@
space_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
space_la_LIBADD=@PLUGIN_LIBADD@

spat3d_la_SOURCES=Opcodes/spat3d.c
spat3d_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
spat3d_la_CFLAGS=@PLUGIN_CFLAGS@
spat3d_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
spat3d_la_LIBADD=@PLUGIN_LIBADD@

ugensa_la_SOURCES=Opcodes/ugensa.c
ugensa_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
ugensa_la_CFLAGS=@PLUGIN_CFLAGS@
ugensa_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
ugensa_la_LIBADD=@PLUGIN_LIBADD@

uggab_la_SOURCES=Opcodes/uggab.c
uggab_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
uggab_la_CFLAGS=@PLUGIN_CFLAGS@
uggab_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
uggab_la_LIBADD=@PLUGIN_LIBADD@

ugmoss_la_SOURCES=Opcodes/ugmoss.c
ugmoss_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
ugmoss_la_CFLAGS=@PLUGIN_CFLAGS@
ugmoss_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
ugmoss_la_LIBADD=@PLUGIN_LIBADD@

ugsc_la_SOURCES=Opcodes/ugsc.c
ugsc_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
ugsc_la_CFLAGS=@PLUGIN_CFLAGS@
ugsc_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
ugsc_la_LIBADD=@PLUGIN_LIBADD@

wave_terrain_la_SOURCES=Opcodes/wave-terrain.c
wave_terrain_CPPFLAGS=@PLUGIN_CPPFLAGS@
wave_terrain_CFLAGS=@PLUGIN_CFLAGS@
wave_terrain_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
wave_terrain_la_LIBADD=@PLUGIN_LIBADD@

#	Convenience library for utility programs:

libustub_la_SOURCES=\
Engine/extract.c \
Engine/filopen.c \
Engine/memalloc.c \
Engine/memfiles.c \
Engine/scsort.c \
Engine/scxtract.c \
Engine/sort.c \
Engine/sread.c \
Engine/swrite.c \
Engine/twarp.c \
InOut/aifc.c \
InOut/aiff.c \
InOut/FL_graph.cpp \
InOut/ieee80.c \
InOut/libsnd.c \
InOut/sfheader.c \
InOut/soundin.c \
InOut/soundio.c \
InOut/ulaw.c \
InOut/wave.c \
InOut/winascii.c \
InOut/window.c \
InOut/winEPS.c \
InOut/winFLTK.c \
OOps/dsputil.c \
OOps/fft.c \
OOps/mxfft.c \
OOps/lptrkfns.c \
OOps/pvfileio.c \
OOps/pvoc.c \
OOps/pvocext.c \
OOps/pvread.c \
OOps/pvsanal.c \
OOps/pvxanal.c \
OOps/sdif.c \
Top/cvanal.c \
Top/getstring.c \
Top/hetro.c \
Top/lpanal.c \
Top/pvanal.c \
Top/pvlook.c \
Top/scot.c \
Top/sndinfo.c \
Top/ustub.c	

libustub_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@
libustub_la_CFLAGS=@LIBCSOUND_CFLAGS@
libustub_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@
libustub_la_LIBADD=@LIBCSOUND_LIBADD@
libustub_la_LDFLAGS=@LIBCSOUND_LDFLAGS@

#	Utility programs:

# To use Cscore for algorithmic composition, provide your own cscore_main.c and rebuild cscore.
cscore_SOURCES=util1/cscore/cscore_main.c
cscore_CPPFLAGS=@UTILITY_CPPFLAGS@
cscore_CFLAGS=@UTILITY_CFLAGS@
cscore_LDADD=.libs/libcsound.a @UTILITY_LDADD@
cscore_LDFLAGS=@UTILITY_LDFLAGS@

cvanal_SOURCES=anal/convol/cvl_main.c
cvanal_CPPFLAGS=@UTILITY_CPPFLAGS@
cvanal_CFLAGS=@UTILITY_CFLAGS@
cvanal_LDADD=.libs/libustub.a @UTILITY_LDADD@
cvanal_LDFLAGS=@UTILITY_LDFLAGS@

dnoise_SOURCES=util2/dnoise.dir/dnoise_main.c
dnoise_CPPFLAGS=@UTILITY_CPPFLAGS@
dnoise_CFLAGS=@UTILITY_CFLAGS@
dnoise_LDADD=.libs/libcsound.a @UTILITY_LDADD@
dnoise_LDFLAGS=@UTILITY_LDFLAGS@

envext_SOURCES=util2/envext/envext.c
envext_CPPFLAGS=@UTILITY_CPPFLAGS@
envext_CFLAGS=@UTILITY_CFLAGS@
envext_LDADD=.libs/libustub.a @UTILITY_LDADD@
envext_LDFLAGS=@UTILITY_LDFLAGS@

extract_SOURCES=util1/sortex/xmain.c
extract_CPPFLAGS=@UTILITY_CPPFLAGS@
extract_CFLAGS=@UTILITY_CFLAGS@
extract_LDADD=.libs/libustub.a @UTILITY_LDADD@
extract_LDFLAGS=@UTILITY_LDFLAGS@

extractor_SOURCES=util2/mixer/xtrct.c
extractor_CPPFLAGS=@UTILITY_CPPFLAGS@
extractor_CFLAGS=@UTILITY_CFLAGS@
extractor_LDADD=.libs/libustub.a @UTILITY_LDADD@
extractor_LDFLAGS=@UTILITY_LDFLAGS@

het_export_SOURCES=util2/exports/het_export.c
het_export_CPPFLAGS=@UTILITY_CPPFLAGS@
het_export_CFLAGS=@UTILITY_CFLAGS@
het_export_LDADD=.libs/libustub.a @UTILITY_LDADD@
het_export_LDFLAGS=@UTILITY_LDFLAGS@

het_import_SOURCES=util2/exports/het_import.c
het_import_CPPFLAGS=@UTILITY_CPPFLAGS@
het_import_CFLAGS=@UTILITY_CFLAGS@
het_import_LDADD=.libs/libustub.a @UTILITY_LDADD@
het_import_LDFLAGS=@UTILITY_LDFLAGS@

hetro_SOURCES=anal/adsyn/het_main.c
hetro_CPPFLAGS=@UTILITY_CPPFLAGS@
hetro_CFLAGS=@UTILITY_CFLAGS@
hetro_LDADD=.libs/libustub.a @UTILITY_LDADD@
hetro_LDFLAGS=@UTILITY_LDFLAGS@

lpanal_SOURCES=anal/lpc/lpc_main.c
lpanal_CPPFLAGS=@UTILITY_CPPFLAGS@
lpanal_CFLAGS=@UTILITY_CFLAGS@
lpanal_LDADD=.libs/libustub.a @UTILITY_LDADD@
lpanal_LDFLAGS=@UTILITY_LDFLAGS@

lpc_export_SOURCES=util2/exports/lpc_export.c
lpc_export_CPPFLAGS=@UTILITY_CPPFLAGS@
lpc_export_CFLAGS=@UTILITY_CFLAGS@
lpc_export_LDADD=.libs/libustub.a @UTILITY_LDADD@
lpc_export_LDFLAGS=@UTILITY_LDFLAGS@

lpc_import_SOURCES=util2/exports/lpc_import.c
lpc_import_CPPFLAGS=@UTILITY_CPPFLAGS@
lpc_import_CFLAGS=@UTILITY_CFLAGS@
lpc_import_LDADD=.libs/libustub.a @UTILITY_LDADD@
lpc_import_LDFLAGS=@UTILITY_LDFLAGS@

mixer_SOURCES=util2/mixer/mixer.c
mixer_CPPFLAGS=@UTILITY_CPPFLAGS@
mixer_CFLAGS=@UTILITY_CFLAGS@
mixer_LDADD=.libs/libustub.a @UTILITY_LDADD@
mixer_LDFLAGS=@UTILITY_LDFLAGS@

pv_export_SOURCES=util2/exports/pv_export.c
pv_export_CPPFLAGS=@UTILITY_CPPFLAGS@
pv_export_CFLAGS=@UTILITY_CFLAGS@
pv_export_LDADD=.libs/libustub.a @UTILITY_LDADD@
pv_export_LDFLAGS=@UTILITY_LDFLAGS@

pv_import_SOURCES=util2/exports/pv_import.c
pv_import_CPPFLAGS=@UTILITY_CPPFLAGS@
pv_import_CFLAGS=@UTILITY_CFLAGS@
pv_import_LDADD=.libs/libustub.a @UTILITY_LDADD@
pv_import_LDFLAGS=@UTILITY_LDFLAGS@

pvanal_SOURCES=anal/pvoc/pvc_main.c
pvanal_CPPFLAGS=@UTILITY_CPPFLAGS@
pvanal_CFLAGS=@UTILITY_CFLAGS@
pvanal_LDADD=.libs/libustub.a @UTILITY_LDADD@
pvanal_LDFLAGS=@UTILITY_LDFLAGS@

pvlook_SOURCES=util2/pvlook.dir/pvl_main.c
pvlook_CPPFLAGS=@UTILITY_CPPFLAGS@
pvlook_CFLAGS=@UTILITY_CFLAGS@
pvlook_LDADD=.libs/libustub.a @UTILITY_LDADD@
pvlook_LDFLAGS=@UTILITY_LDFLAGS@

scale_SOURCES=util2/scale.dir/scale.c
scale_CPPFLAGS=@UTILITY_CPPFLAGS@
scale_CFLAGS=@UTILITY_CFLAGS@
scale_LDADD=.libs/libustub.a @UTILITY_LDADD@
scale_LDFLAGS=@UTILITY_LDFLAGS@

scot_SOURCES=util1/scot/scot_main.c
scot_CPPFLAGS=@UTILITY_CPPFLAGS@
scot_CFLAGS=@UTILITY_CFLAGS@
scot_LDADD=.libs/libustub.a @UTILITY_LDADD@
scot_LDFLAGS=@UTILITY_LDFLAGS@

scsort_SOURCES=util1/sortex/smain.c
scsort_CPPFLAGS=@UTILITY_CPPFLAGS@
scsort_CFLAGS=@UTILITY_CFLAGS@
scsort_LDADD=.libs/libustub.a @UTILITY_LDADD@
scsort_LDFLAGS=@UTILITY_LDFLAGS@

sndinfo_SOURCES=util2/sndinfo/sndinfo_main.c
sndinfo_CPPFLAGS=@UTILITY_CPPFLAGS@
sndinfo_CFLAGS=@UTILITY_CFLAGS@
sndinfo_LDADD=.libs/libustub.a @UTILITY_LDADD@
sndinfo_LDFLAGS=@UTILITY_LDFLAGS@

srconv_SOURCES=util2/dnoise.dir/srconv.c
srconv_CPPFLAGS=@UTILITY_CPPFLAGS@
srconv_CFLAGS=@UTILITY_CFLAGS@
srconv_LDADD=.libs/libustub.a @UTILITY_LDADD@
srconv_LDFLAGS=@UTILITY_LDFLAGS@

sdif2ad_SOURCES=SDIF/sdif2adsyn.c \
SDIF/sdif.c \
SDIF/sdif.h \
SDIF/sdif-mem.c \
SDIF/sdif-mem.h
sdif2ad_CPPFLAGS=@UTILITY_CPPFLAGS@
sdif2ad_CFLAGS=@UTILITY_CFLAGS@
sdif2ad_LDADD=.libs/libustub.a @UTILITY_LDADD@
sdif2ad_LDFLAGS=@UTILITY_LDFLAGS@


	




