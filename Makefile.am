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

#	Headers (incomplete for now).

pkginclude_HEADERS=H/csound.h \
H\cs.h \
H\csoundCore.h

#	Data

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

if CONDITIONAL_ENABLE_CSOUNDVST
if CONDITIONAL_USE_FLTK
if CONDITIONAL_USE_PYTHON
if CONDITIONAL_USE_SWIG

# Note that _CsoundVST is a "module" to dlopen, not a "shared library" to link...
lib_LTLIBRARIES+=_CsoundVST.la
# ...and the standalone CsoundVST always uses dlopen to run _CsoundVST.
bin_PROGRAMS+=CsoundVST

endif
endif
endif
endif

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

EXTRA_DIST=\
csound.spec \
trapped.csd \
trappedrt.csd \
xanadu.csd \
Doxyfile \
frontends/CsoundVST/index.htm 

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

#	Note that since some sources are compiled conditionally,
#	ALL sources should go into this list, which is used to compile the tar file.

EXTRA_libcsound_la_SOURCES=\
anal/adsyn/het_main.c \
anal/convol/cvl_main.c \
anal/lpc/lpc_main.c \
anal/pvoc/pvc_main.c \
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
Engine/otran.c \
Engine/rdorch.c \
Engine/rdscor.c \
Engine/scsort.c \
Engine/scxtract.c \
Engine/sort.c \
Engine/sread.c \
Engine/swrite.c \
Engine/twarp.c \
frontends/csound/csound_main.c \
frontends/CsoundVST/AEffect.h \
frontends/CsoundVST/aeffectx.h \
frontends/CsoundVST/AEffEditor.hpp \
frontends/CsoundVST/aeffguieditor.cpp \
frontends/CsoundVST/aeffguieditor.h \
frontends/CsoundVST/AudioEffect.cpp \
frontends/CsoundVST/AudioEffect.hpp \
frontends/CsoundVST/audioeffectx.cpp \
frontends/CsoundVST/audioeffectx.h \
frontends/CsoundVST/Cell.cpp \
frontends/CsoundVST/Cell.hpp \
frontends/CsoundVST/Composition.cpp \
frontends/CsoundVST/Composition.hpp \
frontends/CsoundVST/Conversions.cpp \
frontends/CsoundVST/Conversions.hpp \
frontends/CsoundVST/CppSound.cpp \
frontends/CsoundVST/CppSound.hpp \
frontends/CsoundVST/CsoundFile.cpp \
frontends/CsoundVST/CsoundFile.hpp \
frontends/CsoundVST/CsoundVST.cpp \
frontends/CsoundVST/CsoundVST.hpp \
frontends/CsoundVST/CsoundVST.i \
frontends/CsoundVST/csoundvst_main.cpp \
frontends/CsoundVST/CsoundVstFltk.cpp \
frontends/CsoundVST/CsoundVstFltk.hpp \
frontends/CsoundVST/CsoundVSTMain.cpp \
frontends/CsoundVST/CsoundVstUi.cpp \
frontends/CsoundVST/CsoundVstUi.h \
frontends/CsoundVST/Event.cpp \
frontends/CsoundVST/Event.hpp \
frontends/CsoundVST/Exception.hpp \
frontends/CsoundVST/Hocket.cpp \
frontends/CsoundVST/Hocket.hpp \
frontends/CsoundVST/ImageToScore.cpp \
frontends/CsoundVST/ImageToScore.hpp \
frontends/CsoundVST/Lindenmayer.cpp \
frontends/CsoundVST/Lindenmayer.hpp \
frontends/CsoundVST/MCRM.cpp \
frontends/CsoundVST/MCRM.hpp \
frontends/CsoundVST/Midifile.cpp \
frontends/CsoundVST/Midifile.hpp \
frontends/CsoundVST/MusicModel.cpp \
frontends/CsoundVST/MusicModel.hpp \
frontends/CsoundVST/Node.cpp \
frontends/CsoundVST/Node.hpp \
frontends/CsoundVST/Random.cpp \
frontends/CsoundVST/Random.hpp \
frontends/CsoundVST/Rescale.cpp \
frontends/CsoundVST/Rescale.hpp \
frontends/CsoundVST/Score.cpp \
frontends/CsoundVST/Score.hpp \
frontends/CsoundVST/ScoreNode.cpp \
frontends/CsoundVST/ScoreNode.hpp \
frontends/CsoundVST/Shell.cpp \
frontends/CsoundVST/Shell.hpp \
frontends/CsoundVST/Silence.hpp \
frontends/CsoundVST/StrangeAttractor.cpp \
frontends/CsoundVST/StrangeAttractor.hpp \
frontends/CsoundVST/System.cpp \
frontends/CsoundVST/System.hpp \
frontends/CsoundVST/vstcontrols.cpp \
frontends/CsoundVST/vstcontrols.h \
frontends/CsoundVST/vstfxstore.h \
frontends/CsoundVST/vstgui.cpp \
frontends/CsoundVST/vstgui.h \
frontends/CsoundVST/vstplugscarbon.h \
frontends/CsoundVST/vstplugsmac.h \
frontends/CsoundVST/vstplugsmacho.h \
H/3Dug.h \
H/aiff.h \
H/aops.h \
H/BeAudio.h \
H/cmath.h \
H/complex.h \
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
H/ugmoss.h \
H/ugrw1.h \
H/ugrw2.h \
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
InOut/FL_graph.cpp \
InOut/ieee80.c \
InOut/libsnd.c \
InOut/rtpa.c \
InOut/sfheader.c \
InOut/soundin.c \
InOut/soundio.c \
InOut/ulaw.c \
InOut/widgets.cpp \
InOut/win98.c \
InOut/winascii.c \
InOut/windin.c \
InOut/window.c \
InOut/winEPS.c \
InOut/winFLTK.c \
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
Opcodes/babo.c \
Opcodes/babo.h \
Opcodes/bbcut.c \
Opcodes/bbcut.h \
Opcodes/biquad.c \
Opcodes/biquad.h \
Opcodes/bowed.h \
Opcodes/bowedbar.c \
Opcodes/bowedbar.h \
Opcodes/brass.h \
Opcodes/butter.c \
Opcodes/butter.h \
Opcodes/clarinet.h \
Opcodes/clfilt.c \
Opcodes/clfilt.h \
Opcodes/cross2.c \
Opcodes/dam.c \
Opcodes/dam.h \
Opcodes/dcblockr.c \
Opcodes/dcblockr.h \
Opcodes/filter.c \
Opcodes/filter.h \
Opcodes/flanger.c \
Opcodes/flanger.h \
Opcodes/flute.h \
Opcodes/fm4op.c \
Opcodes/fm4op.h \
Opcodes/follow.c \
Opcodes/follow.h \
Opcodes/grain.c \
Opcodes/grain.h \
Opcodes/grain4.c \
Opcodes/grain4.h \
Opcodes/hrtferX.c \
Opcodes/hrtferx.h \
Opcodes/locsig.c \
Opcodes/locsig.h \
Opcodes/lowpassr.c \
Opcodes/lowpassr.h \
Opcodes/mandolin.c \
Opcodes/mandolin.h \
Opcodes/marimba.h \
Opcodes/midiops2.c \
Opcodes/midiops3.c \
Opcodes/midiops3.h \
Opcodes/modal4.c \
Opcodes/modal4.h \
Opcodes/moog1.c \
Opcodes/moog1.h \
Opcodes/nlfilt.c \
Opcodes/nlfilt.h \
Opcodes/oscbnk.c \
Opcodes/oscbnk.h \
Opcodes/phisem.c \
Opcodes/phisem.h \
Opcodes/physmod.c \
Opcodes/physutil.c \
Opcodes/physutil.h \
Opcodes/pitch.c \
Opcodes/pitch.h \
Opcodes/pitch0.c \
Opcodes/pluck.c \
Opcodes/pluck.h \
Opcodes/ptrigtbl.h \
Opcodes/repluck.c \
Opcodes/repluck.h \
Opcodes/scansyn.c \
Opcodes/scansyn.h \
Opcodes/scansynx.c \
Opcodes/scansynx.h \
Opcodes/sfenum.h \
Opcodes/sfont.c \
Opcodes/sfont.h \
Opcodes/shaker.c \
Opcodes/shaker.h \
Opcodes/singwave.c \
Opcodes/singwave.h \
Opcodes/sndwarp.c \
Opcodes/sndwarp.h \
Opcodes/space.c \
Opcodes/space.h \
Opcodes/spat3d.c \
Opcodes/spat3d.h \
Opcodes/spectra.c \
Opcodes/ugens6.c \
Opcodes/ugens7.c \
Opcodes/ugens9.c \
Opcodes/ugensa.c \
Opcodes/ugensa.h \
Opcodes/uggab.c \
Opcodes/uggab.h \
Opcodes/ugmoss.c \
Opcodes/ugsc.c \
Opcodes/vibraphn.h \
Opcodes/wavegde.h \
Opcodes/wave-terrain.c \
Opcodes/wave-terrain.h \
SDIF/sdif.c \
SDIF/sdif.h \
SDIF/sdif2adsyn.c \
SDIF/sdif-mem.c \
SDIF/sdif-mem.h \
strings/makedb.c \
Top/argdecode.c \
Top/cscore.c \
Top/cscore_internal.c \
Top/cscorfns.c \
Top/cscormai.c \
Top/csound.c \
Top/csound_main.c \
Top/cvanal.c \
Top/dl_opcodes.c \
Top/dnoise.c \
Top/dnoise_int.c \
Top/getstring.c \
Top/hetro.c \
Top/lpanal.c \
Top/main.c \
Top/natben.c \
Top/one_file.c \
Top/opcode.c \
Top/pvanal.c \
Top/pvl_main.c \
Top/pvlook.c \
Top/scot.c \
Top/sndinfo.c \
Top/ustub.c \
util1/cscore/cscore.c \
util1/cscore/cscore_main.c \
util1/scot/scot_main.c \
util1/sortex/MOD.c \
util1/sortex/smain.c \
util1/sortex/xmain.c \
util2/dnoise.dir/dnoise_main.c \
util2/dnoise.dir/srconv.c \
util2/envext/envext.c \
util2/exports/het_export.c \
util2/exports/het_import.c \
util2/exports/lpc_export.c \
util2/exports/lpc_import.c \
util2/exports/pv_export.c \
util2/exports/pv_import.c \
util2/mixer/extract.c \
util2/mixer/mixer.c \
util2/mixer/xtrct.c \
util2/pvlook.dir/pvl_main.c \
util2/pvlook.dir/pvlook.c \
util2/scale.dir/scale.c \
util2/sndinfo/sndinfo_main.c

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
InOut/ieee80.c \
InOut/libsnd.c \
InOut/sfheader.c \
InOut/soundin.c \
InOut/soundio.c \
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
Top/natben.c \
Top/one_file.c \
Top/opcode.c \
Top/pvanal.c \
Top/pvlook.c \
Top/scot.c \
Top/sndinfo.c \
$(RTAUDIO_SOURCES) \
$(FLTK_SOURCES)

libcsound_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@
libcsound_la_CFLAGS=@LIBCSOUND_CFLAGS@
libcsound_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@
libcsound_la_LIBADD=@LIBCSOUND_LIBADD@
libcsound_la_LDFLAGS=@LIBCSOUND_LDFLAGS@

if CONDITIONAL_USE_RTAUDIO
RTAUDIO_SOURCES=InOut/rtpa.c
endif

if CONDITIONAL_USE_FLTK
FLTK_SOURCES=InOut/winFLTK.c InOut/FL_graph.cpp InOut/widgets.cpp
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
InOut/FL_graph.cpp \
InOut/ieee80.c \
InOut/libsnd.c \
InOut/sfheader.c \
InOut/soundin.c \
InOut/soundio.c \
InOut/ulaw.c \
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
Top/natben.c \
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
SDIF/sdif-mem.h \
sdif2ad_CPPFLAGS=@UTILITY_CPPFLAGS@
sdif2ad_CFLAGS=@UTILITY_CFLAGS@
sdif2ad_LDADD=.libs/libustub.a @UTILITY_LDADD@
sdif2ad_LDFLAGS=@UTILITY_LDFLAGS@

if CONDITIONAL_ENABLE_CSOUNDVST
if CONDITIONAL_USE_FLTK
if CONDITIONAL_USE_PYTHON
if CONDITIONAL_USE_SWIG

frontends/CsoundVST/CsoundVST_wrap.cpp: frontends/CsoundVST/CsoundVST.i
	swig @PLUGIN_CPPFLAGS@ -Ifrontends/CsoundVST -python -c++ -c -includeall -verbose -o frontends/CsoundVST/CsoundVST_wrap.cpp frontends/CsoundVST/CsoundVST.i 

_CsoundVST_la_SOURCES=\
frontends/CsoundVST/AudioEffect.cpp \
frontends/CsoundVST/audioeffectx.cpp \
frontends/CsoundVST/Composition.cpp \
frontends/CsoundVST/Conversions.cpp \
frontends/CsoundVST/CppSound.cpp \
frontends/CsoundVST/CsoundFile.cpp \
frontends/CsoundVST/Cell.cpp \
frontends/CsoundVST/CsoundVST.cpp \
frontends/CsoundVST/CsoundVST_wrap.cpp \
frontends/CsoundVST/CsoundVstFltk.cpp \
frontends/CsoundVST/CsoundVSTMain.cpp \
frontends/CsoundVST/CsoundVstUi.cpp \
frontends/CsoundVST/Event.cpp \
frontends/CsoundVST/Hocket.cpp \
frontends/CsoundVST/ImageToScore.cpp \
frontends/CsoundVST/Lindenmayer.cpp \
frontends/CsoundVST/MCRM.cpp \
frontends/CsoundVST/Midifile.cpp \
frontends/CsoundVST/MusicModel.cpp \
frontends/CsoundVST/Node.cpp \
frontends/CsoundVST/Random.cpp \
frontends/CsoundVST/Rescale.cpp \
frontends/CsoundVST/Score.cpp \
frontends/CsoundVST/ScoreNode.cpp \
frontends/CsoundVST/Shell.cpp \
frontends/CsoundVST/StrangeAttractor.cpp \
frontends/CsoundVST/System.cpp

if CONDITIONAL_IS_WIN32

_CsoundVST_la_DEPENDENCIES=xmg libcsound.la
_CsoundVST_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@ -D_MSC_VER -I/usr/include/python2.3 -Ifrontends/CsoundVST
_CsoundVST_la_CFLAGS=@LIBCSOUND_CFLAGS@ -D_MSC_VER
_CsoundVST_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@ -D_MSC_VER
_CsoundVST_la_LIBADD=@LIBCSOUND_LIBADD@
_CsoundVST_la_LDFLAGS=@LIBCSOUND_LDFLAGS@

_CsoundVST.dll:	libcsound.la	
	g++ -g -O2 -Wall -L/usr/local/lib -L.libs -L/usr/lib/python2.3/config -o _CsoundVST.dll -Wl,--out-implib,_CsoundVST.dll.a -L/usr/local/lib -shared -module --no-undefined -mconsole -L/usr/local/lib _csoundvst_la-AudioEffect.o _csoundvst_la-audioeffectx.o _csoundvst_la-Composition.o _csoundvst_la-Conversions.o _csoundvst_la-CppSound.o _csoundvst_la-CsoundFile.o _csoundvst_la-Cell.o _csoundvst_la-CsoundVST.o _csoundvst_la-CsoundVST_wrap.o _csoundvst_la-CsoundVstFltk.o _csoundvst_la-CsoundVstMain.o _csoundvst_la-CsoundVstUi.o _csoundvst_la-Event.o _csoundvst_la-Hocket.o _csoundvst_la-ImageToScore.o _csoundvst_la-Lindenmayer.o _csoundvst_la-MCRM.o _csoundvst_la-Midifile.o _csoundvst_la-MusicModel.o _csoundvst_la-Node.o _csoundvst_la-Random.o _csoundvst_la-Rescale.o _csoundvst_la-Score.o _csoundvst_la-ScoreNode.o _csoundvst_la-Shell.o _csoundvst_la-StrangeAttractor.o _csoundvst_la-System.o -mwindows -lfltk -lole32 -luuid -lcomctl32 -lwsock32 -lsupc++ -lpthread -lstdc++ -lcsound -lportaudio -lwinmm -lm -lpthread -lfltk -lpthread -lfltk -lfltk_images -lsndfile -lpthread -lswigpy -lpython2.3 

CsoundVST_SOURCES=frontends/csoundvst/csoundvst_main.cpp
CsoundVST_DEPENDENCIES=_CsoundVST.dll
CsoundVST_CPPFLAGS=@CSOUND_CPPFLAGS@ -I/usr/include/python2.3 -Ifrontends/CsoundVST
CsoundVST_CFLAGS=@CSOUND_CFLAGS@
CsoundVST_CXXFLAGS=-mwindows @CSOUND_CXXFLAGS@
CsoundVST_LDFLAGS=-mwindows @CSOUND_LDFLAGS@ -mwindows _CsoundVST.la -L/usr/lib/python2.3/config -lpython2.3
CsoundVST_LDADD=@CSOUND_LDADD@

CsoundVST.exe:	_CsoundVST.dll
	g++ -g -O2 -Wall -D_WIN32 CsoundVST-csoundvst_main.o -mwindows -o CsoundVST.exe _CsoundVST.dll.a -L/GCC/gcc-3.3.1-3/.build/i686-pc-cygwin/libstdc++-v3/src -L/GCC/gcc-3.3.1-3/.build/i686-pc-cygwin/libstdc++-v3/src/.libs -L/GCC/gcc-3.3.1-3/.build/gcc -L/usr/i686-pc-cygwin/bin -L/usr/lib/gcc-lib/i686-pc-cygwin/3.3.1 -L/usr/lib/gcc-lib/i686-pc-cygwin/3.3.1/../../.. -lcygwin -luser32 -lkernel32 -ladvapi32 -lshell32 -lgcc -lstdc++
    
else

_CsoundVST_la_DEPENDENCIES=xmg libcsound.la
_CsoundVST_la_CPPFLAGS=@LIBCSOUND_CPPFLAGS@ -I/usr/include/python2.3 -Ifrontends/CsoundVST
_CsoundVST_la_CFLAGS=@LIBCSOUND_CFLAGS@
_CsoundVST_la_CXXFLAGS=@LIBCSOUND_CXXFLAGS@
_CsoundVST_la_LIBADD=@LIBCSOUND_LIBADD@
_CsoundVST_la_LDFLAGS=-module @LIBCSOUND_LDFLAGS@

CsoundVST_SOURCES=frontends/csoundvst/csoundvst_main.cpp
CsoundVST_DEPENDENCIES=_CsoundVST.la
CsoundVST_CPPFLAGS=@CSOUND_CPPFLAGS@ -I/usr/include/python2.3 -Ifrontends/CsoundVST
CsoundVST_CFLAGS=@CSOUND_CFLAGS@
CsoundVST_CXXFLAGS=@CSOUND_CXXFLAGS@
CsoundVST_LDFLAGS=@CSOUND_LDFLAGS@ -mwindows _CsoundVST.la -L/usr/lib/python2.3/config -lpython2.3
CsoundVST_LDADD=@CSOUND_LDADD@

endif


endif
endif
endif
endif

