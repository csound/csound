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

noinst_PROGRAMS=makedb

# headers (incomplete for now).

pkginclude_HEADERS=H/csound.h

# data

pkgdata_DATA=csound.xmg

#	The Csound engine and API library:

lib_LTLIBRARIES=libcsound.la

#	Plugin opcodes 
#	(note that they are "modules" to dlopen, not "shared libraries" to link):

pkglib_LTLIBRARIES=libbabo.la
pkglib_LTLIBRARIES+=libbbcut.la
pkglib_LTLIBRARIES+=libbiquad.la
pkglib_LTLIBRARIES+=libbutter.la
pkglib_LTLIBRARIES+=libclfilt.la
pkglib_LTLIBRARIES+=libcross2.la
pkglib_LTLIBRARIES+=libdam.la
pkglib_LTLIBRARIES+=libdcblock.la
pkglib_LTLIBRARIES+=libfilter.la
pkglib_LTLIBRARIES+=libflanger.la
pkglib_LTLIBRARIES+=libfollow.la
pkglib_LTLIBRARIES+=libgrain.la
pkglib_LTLIBRARIES+=libgrain4.la
pkglib_LTLIBRARIES+=libhrtferX.la
pkglib_LTLIBRARIES+=liblocsig.la
pkglib_LTLIBRARIES+=liblowpassr.la
pkglib_LTLIBRARIES+=libmidiops2.la
pkglib_LTLIBRARIES+=libmidiops3.la
pkglib_LTLIBRARIES+=libmodal4.la
pkglib_LTLIBRARIES+=libnlfilt.la
pkglib_LTLIBRARIES+=liboscbnk.la
pkglib_LTLIBRARIES+=libphisem.la
pkglib_LTLIBRARIES+=libphysmod.la
pkglib_LTLIBRARIES+=libpitch.la
pkglib_LTLIBRARIES+=libpluck.la
pkglib_LTLIBRARIES+=librepluck.la
pkglib_LTLIBRARIES+=libscansyn.la
pkglib_LTLIBRARIES+=libscansynx.la
pkglib_LTLIBRARIES+=libsfont.la
pkglib_LTLIBRARIES+=libsndwarp.la
pkglib_LTLIBRARIES+=libspace.la
pkglib_LTLIBRARIES+=libspat3d.la
pkglib_LTLIBRARIES+=libugensa.la
pkglib_LTLIBRARIES+=libuggab.la
pkglib_LTLIBRARIES+=libugmoss.la
pkglib_LTLIBRARIES+=libugsc.la
pkglib_LTLIBRARIES+=libwave_terrain.la

#	User interfaces for Csound:

bin_PROGRAMS=csound

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
	./makedb $(srcdir)/strings/all_strings American

English.xmg:	strings/english-strings makedb
	./makedb $(srcdir)/strings/english-strings English

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
Top/lpanal.c \
Top/main.c \
Top/one_file.c \
Top/opcode.c \
Top/pvanal.c \
Top/pvlook.c \
Top/scot.c \
Top/sndinfo.c \
$(SGI_RTAUDIO) \
$(SUN_RTAUDIO) \
$(DEC_RTAUDIO)	\
$(HP_RTAUDIO) \
$(WIN32_RTAUDIO) \
$(LINUX_RTAUDIO) \
$(WIN_FLTK) \
$(WIN_FLTK_WIDGETS) \
$(WIN_X11)

libcsound_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@
libcsound_la_CFLAGS=@LIBCSOUND_CFLAGS@
libcsound_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@
libcsound_la_LIBADD=@LIBCSOUND_LIBADD@
libcsound_la_LDFLAGS=@LIBCSOUND_LDFLAGS@

if CONDITIONAL_SGI
if CONDITIONAL_USE_RTAUDIO
SGI_RTAUDIO=InOut/rtSGI.c InOut/SGIplay.c
endif
endif

if CONDITIONAL_sun
if CONDITIONAL_USE_RTAUDIO
SUN_RTAUDIO=InOut/rtSUN.c
endif
endif

if CONDITIONAL_DEC
if CONDITIONAL_USE_RTAUDIO
DEC_RTAUDIO=InOut/rtDEC.c InOut/DECaudio.c InOut/DECplay.c H/DECplay.h
endif
endif

if CONDITIONAL_HP
if CONDITIONAL_USE_RTAUDIO
HP_RTAUDIO=InOut/rtDEC.c InOut/DECaudio.c InOut/DECplay.c H/DECplay.h
endif
endif

if CONDITIONAL_WIN32
if CONDITIONAL_USE_RTAUDIO
WIN32_RTAUDIO=InOut/rtpa.c
endif
endif

if CONDITIONAL_LINUX
if CONDITIONAL_USE_RTAUDIO
LINUX_RTAUDIO=InOut/rtpa.c
endif
endif

if CONDITIONAL_WINDOWS
if CONDITIONAL_USE_FLTK
WIN_FLTK=InOut/winFLTK.c InOut/FL_graph.cpp 
if CONDITIONAL_USE_FLTK_WIDGETS
WIN_FLTK_WIDGETS=InOut/widgets.cpp
endif
endif
if CONDITIONAL_USE_X11
WIN_X11=InOut/winX11.c
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

libbabo_la_SOURCES=Opcodes/babo.c
libbabo_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libbabo_la_CFLAGS=@PLUGIN_CFLAGS@
libbabo_la_LDFLAGS=@PLUGIN_LDFLAGS@
libbabo_la_LIBADD=@PLUGIN_LIBADD@

libbbcut_la_SOURCES=Opcodes/bbcut.c
libbbcut_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libbbcut_la_CFLAGS=@PLUGIN_CFLAGS@
libbbcut_la_LDFLAGS=@PLUGIN_LDFLAGS@
libbbcut_la_LIBADD=@PLUGIN_LIBADD@

libbiquad_la_SOURCES=Opcodes/biquad.c
libbiquad_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libbiquad_la_CFLAGS=@PLUGIN_CFLAGS@
libbiquad_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libbiquad_la_LIBADD=@PLUGIN_LIBADD@

libbutter_la_SOURCES=Opcodes/butter.c
libbutter_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libbutter_la_CFLAGS=@PLUGIN_CFLAGS@
libbutter_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libbutter_la_LIBADD=@PLUGIN_LIBADD@

libclfilt_la_SOURCES=Opcodes/clfilt.c
libclfilt_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libclfilt_la_CFLAGS=@PLUGIN_CFLAGS@
libclfilt_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libclfilt_la_LIBADD=@PLUGIN_LIBADD@

libcross2_la_SOURCES=Opcodes/cross2.c
libcross2_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libcross2_la_CFLAGS=@PLUGIN_CFLAGS@
libcross2_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libcross2_la_LIBADD=@PLUGIN_LIBADD@

libdam_la_SOURCES=Opcodes/dam.c
libdam_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libdam_la_CFLAGS=@PLUGIN_CFLAGS@
libdam_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libdam_la_LIBADD=@PLUGIN_LIBADD@

libdcblock_la_SOURCES=Opcodes/dcblockr.c
libdcblock_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libdcblock_la_CFLAGS=@PLUGIN_CFLAGS@
libdcblock_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libdcblock_la_LIBADD=@PLUGIN_LIBADD@

libfilter_la_SOURCES=Opcodes/filter.c
libfilter_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libfilter_la_CFLAGS=@PLUGIN_CFLAGS@
libfilter_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libfilter_la_LIBADD=@PLUGIN_LIBADD@

libflanger_la_SOURCES=Opcodes/flanger.c
libflanger_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libflanger_la_CFLAGS=@PLUGIN_CFLAGS@
libflanger_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libflanger_la_LIBADD=@PLUGIN_LIBADD@

libfollow_la_SOURCES=Opcodes/follow.c
libfollow_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libfollow_la_CFLAGS=@PLUGIN_CFLAGS@
libfollow_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libfollow_la_LIBADD=@PLUGIN_LIBADD@

libgrain_la_SOURCES=Opcodes/grain.c
libgrain_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libgrain_la_CFLAGS=@PLUGIN_CFLAGS@
libgrain_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libgrain_la_LIBADD=@PLUGIN_LIBADD@

libgrain4_la_SOURCES=Opcodes/grain4.c
libgrain4_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libgrain4_la_CFLAGS=@PLUGIN_CFLAGS@
libgrain4_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libgrain4_la_LIBADD=@PLUGIN_LIBADD@

libhrtferX_la_SOURCES=Opcodes/hrtferX.c \
Top/natben.c
libhrtferX_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libhrtferX_la_CFLAGS=@PLUGIN_CFLAGS@
libhrtferX_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libhrtferX_la_LIBADD=@PLUGIN_LIBADD@

liblocsig_la_SOURCES=Opcodes/locsig.c
liblocsig_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
liblocsig_la_CFLAGS=@PLUGIN_CFLAGS@
liblocsig_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
liblocsig_la_LIBADD=@PLUGIN_LIBADD@

liblowpassr_la_SOURCES=Opcodes/lowpassr.c
liblowpassr_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
liblowpassr_la_CFLAGS=@PLUGIN_CFLAGS@
liblowpassr_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
liblowpassr_la_LIBADD=@PLUGIN_LIBADD@

libmidiops2_la_SOURCES=Opcodes/midiops2.c
libmidiops2_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libmidiops2_la_CFLAGS=@PLUGIN_CFLAGS@
libmidiops2_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libmidiops2_la_LIBADD=@PLUGIN_LIBADD@

libmidiops3_la_SOURCES=Opcodes/midiops3.c
libmidiops3_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libmidiops3_la_CFLAGS=@PLUGIN_CFLAGS@
libmidiops3_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libmidiops3_la_LIBADD=@PLUGIN_LIBADD@

libmodal4_la_SOURCES=Opcodes/modal4.c \
Opcodes/physutil.c 
libmodal4_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libmodal4_la_CFLAGS=@PLUGIN_CFLAGS@
libmodal4_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libmodal4_la_LIBADD=@PLUGIN_LIBADD@

libnlfilt_la_SOURCES=Opcodes/nlfilt.c
libnlfilt_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libnlfilt_la_CFLAGS=@PLUGIN_CFLAGS@
libnlfilt_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libnlfilt_la_LIBADD=@PLUGIN_LIBADD@

liboscbnk_la_SOURCES=Opcodes/oscbnk.c
liboscbnk_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
liboscbnk_la_CFLAGS=@PLUGIN_CFLAGS@
liboscbnk_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
liboscbnk_la_LIBADD=@PLUGIN_LIBADD@

libphisem_la_SOURCES=Opcodes/phisem.c
libphisem_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libphisem_la_CFLAGS=@PLUGIN_CFLAGS@
libphisem_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libphisem_la_LIBADD=@PLUGIN_LIBADD@

libphysmod_la_SOURCES=Opcodes/physmod.c \
Opcodes/physutil.c \
Opcodes/mandolin.c \
Opcodes/singwave.c \
Opcodes/fm4op.c \
Opcodes/moog1.c \
Opcodes/shaker.c \
Opcodes/bowedbar.c
libphysmod_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libphysmod_la_CFLAGS=@PLUGIN_CFLAGS@
libphysmod_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libphysmod_la_LIBADD=@PLUGIN_LIBADD@

libpitch_la_SOURCES=Opcodes/pitch.c \
Opcodes/pitch0.c \
Opcodes/spectra.c
libpitch_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libpitch_la_CFLAGS=@PLUGIN_CFLAGS@
libpitch_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libpitch_la_LIBADD=@PLUGIN_LIBADD@

libpluck_la_SOURCES=Opcodes/pluck.c
libpluck_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libpluck_la_CFLAGS=@PLUGIN_CFLAGS@
libpluck_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libpluck_la_LIBADD=@PLUGIN_LIBADD@

librepluck_la_SOURCES=Opcodes/repluck.c
librepluck_CPPFLAGS=@PLUGIN_CPPFLAGS@
librepluck_CFLAGS=@PLUGIN_CFLAGS@
librepluck_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
librepluck_la_LIBADD=@PLUGIN_LIBADD@

libscansyn_la_SOURCES=Opcodes/scansyn.c
libscansyn_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libscansyn_la_CFLAGS=@PLUGIN_CFLAGS@
libscansyn_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libscansyn_la_LIBADD=@PLUGIN_LIBADD@

libscansynx_la_SOURCES=Opcodes/scansynx.c
libscansynx_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libscansynx_la_CFLAGS=@PLUGIN_CFLAGS@
libscansynx_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libscansynx_la_LIBADD=@PLUGIN_LIBADD@

libsfont_la_SOURCES=Opcodes/sfont.c
libsfont_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libsfont_la_CFLAGS=@PLUGIN_CFLAGS@
libsfont_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libsfont_la_LIBADD=@PLUGIN_LIBADD@

libsndwarp_la_SOURCES=Opcodes/sndwarp.c
libsndwarp_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libsndwarp_la_CFLAGS=@PLUGIN_CFLAGS@
libsndwarp_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libsndwarp_la_LIBADD=@PLUGIN_LIBADD@

libspace_la_SOURCES=Opcodes/space.c
libspace_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libspace_la_CFLAGS=@PLUGIN_CFLAGS@
libspace_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libspace_la_LIBADD=@PLUGIN_LIBADD@

libspat3d_la_SOURCES=Opcodes/spat3d.c
libspat3d_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libspat3d_la_CFLAGS=@PLUGIN_CFLAGS@
libspat3d_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libspat3d_la_LIBADD=@PLUGIN_LIBADD@

libugensa_la_SOURCES=Opcodes/ugensa.c
libugensa_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libugensa_la_CFLAGS=@PLUGIN_CFLAGS@
libugensa_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libugensa_la_LIBADD=@PLUGIN_LIBADD@

libuggab_la_SOURCES=Opcodes/uggab.c
libuggab_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libuggab_la_CFLAGS=@PLUGIN_CFLAGS@
libuggab_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libuggab_la_LIBADD=@PLUGIN_LIBADD@

libugmoss_la_SOURCES=Opcodes/ugmoss.c
libugmoss_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libugmoss_la_CFLAGS=@PLUGIN_CFLAGS@
libugmoss_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libugmoss_la_LIBADD=@PLUGIN_LIBADD@

libugsc_la_SOURCES=Opcodes/ugsc.c
libugsc_la_CPPFLAGS=@PLUGIN_CPPFLAGS@
libugsc_la_CFLAGS=@PLUGIN_CFLAGS@
libugsc_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libugsc_la_LIBADD=@PLUGIN_LIBADD@

libwave_terrain_la_SOURCES=Opcodes/wave-terrain.c
libwave_terrain_CPPFLAGS=@PLUGIN_CPPFLAGS@
libwave_terrain_CFLAGS=@PLUGIN_CFLAGS@
libwave_terrain_la_LDFLAGS=@PLUGIN_LDFLAGS@ 
libwave_terrain_la_LIBADD=@PLUGIN_LIBADD@

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
