## OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##

set(stdopcod_SRCS 
    ambicode.c      bbcut.c         biquad.c
    butter.c        clfilt.c        cross2.c
    dam.c           dcblockr.c      filter.c
    flanger.c       follow.c        fout.c
    freeverb.c      ftconv.c        ftgen.c
    gab/gab.c       gab/vectorial.c grain.c
    locsig.c        lowpassr.c      metro.c
    midiops2.c      midiops3.c      newfils.c
    nlfilt.c        oscbnk.c        pluck.c
    repluck.c       reverbsc.c      seqtime.c
    sndloop.c       sndwarp.c       space.c
    spat3d.c        syncgrain.c     ugens7.c
    ugens9.c        ugensa.c        uggab.c
    ugmoss.c        ugnorman.c      ugsc.c
    wave-terrain.c  stdopcod.c)

make_plugin(stdopcod "${stdopcod_SRCS}" ${LIBSNDFILE_LIBRARY} m)
if(HAVE_VALUES_H)
    set_target_properties(stdopcod PROPERTIES COMPILE_FLAGS "-DHAVE_VALUES_H")
endif()

make_plugin(modmatrix modmatrix.c)
make_plugin(eqfil eqfil.c m)
make_plugin(pvsbuffer pvsbuffer.c)
make_plugin(scoreline scoreline.c)
make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)

set(modal4_SRCS 
    modal4.c physutil.c)
make_plugin(modal4 "${modal4_SRCS}" m)

set(physmod_SRCS 
    physmod.c physutil.c 
    mandolin.c singwave.c
    fm4op.c moog1.c shaker.c bowedbar.c)
make_plugin(physmod "${physmod_SRCS}" m)


set(pitch_SRCS 
    pitch.c pitch0.c spectra.c)    
make_plugin(pitch "${pitch_SRCS}" m)


make_plugin(barmodel bilbar.c m)
make_plugin(compress compress.c m)
make_plugin(grain4 grain4.c)
make_plugin(loscilx loscilx.c m)
make_plugin(minmax minmax.c)
make_plugin(cs_pan2 pan2.c m)
make_plugin(phisem phisem.c m)

set(pvoc_SRCS 
    dsputil.c pvadd.c pvinterp.c pvocext.c
    pvread.c ugens8.c vpvoc.c pvoc.c)
make_plugin(pvoc "${pvoc_SRCS}" m)

set(cs_pvs_ops_SRCS
    ifd.c partials.c psynth.c pvsbasic.c
    pvscent.c pvsdemix.c pvs_ops.c pvsband.c)
make_plugin(cs_pvs_ops "${cs_pvs_ops_SRCS}" m)

make_plugin(stackops stackops.c)
make_plugin(vaops vaops.c)
make_plugin(ugakbari ugakbari.c m)
make_plugin(harmon harmon.c m)
make_plugin(ampmidid ampmidid.cpp)
make_plugin(cs_date date.c)
make_plugin(system_call system_call.c)
make_plugin(ptrack pitchtrack.c m)
make_plugin(mutexops mutexops.cpp)
make_plugin(partikkel partikkel.c m)
make_plugin(shape shape.c m)
make_plugin(doppler doppler.cpp)
make_plugin(tabsum tabsum.c)
make_plugin(crossfm crossfm.c m)
make_plugin(pvlock pvlock.c m)
make_plugin(vosim Vosim.c)
make_plugin(tabfns tabvars.c)

set(gabnew_SRCS
    gab/tabmorph.c      gab/hvs.c
    gab/sliderTable.c   gab/newgabopc.c)
make_plugin(gabnew "${gabnew_SRCS}" m)

make_plugin(hrtfnew hrtfopcodes.c m)
if(BIG_ENDIAN)
    set_target_properties(hrtfnew PROPERTIES
        COMPILE_DEFINITIONS "WORDS_BIGENDIAN")
endif()

# temporarily do not build on Windows until these opcodes are fixed
# to compile
if(NOT WIN32) 
    make_plugin(udprecv sockrecv.c)
    make_plugin(udpsend socksend.c)
endif()


make_plugin(ftest ftest.c m)
make_plugin(ambicode1 ambicode1.c m)
make_plugin(babo babo.c m)
make_plugin(hrtferX hrtferX.c m)

set(vbap_SRCS 
    vbap.c vbap_eight.c vbap_four.c
    vbap_sixteen.c vbap_zak.c)
make_plugin(vbap "${vbap_SRCS}" m)

set(scansyn_SRCS
    scansyn.c scansynx.c)
make_plugin(scansyn "${scansyn_SRCS}" m)

if(UNIX)
    make_plugin(control control.c)
endif()

if(LINUX)
    make_plugin(cpumeter cpumeter.c)
endif()

## OPCODES WITH EXTERNAL DEPENDENCIES ##

# OGG Opcode previously was built only for OLPC
#if(VORBISFILE_LIBRARY)
#    set(vorbis_LIBS ${libcsound_LIBS} ${VORBISFILE_LIBRARY})
#    make_plugin(ogg ogg.c "${vorbis_LIBS}")
#endif()



set(sfont_FLAGS "")

if(CMAKE_C_COMPILER MATCHES "gcc")
    set(sfont_FLAGS ${sfont_FLAGS} -fno-strict-aliasing)
endif()
if(BIG_ENDIAN)
    set(sfont_FLAGS ${sfont_FLAGS} -DWORDS_BIGENDIAN)
endif()
message(STATUS "Compiler flags used for sfont lib: ${sfont_FLAGS}")
make_plugin(sfont sfont.c m)
set_target_properties(sfont
    PROPERTIES COMPILER_FLAGS "${sfont_FLAGS}")

    
          

#if mpafound==1:
#  makePlugin(pluginEnvironment, 'mp3in', ['mp3in.c'])
#if wiifound==1:
#  WiiEnvironment = pluginEnvironment.Clone()
#  makePlugin(WiiEnvironment, 'wiimote', ['wiimote.c'])
#if p5gfound==1:
#  P5GEnvironment = pluginEnvironment.Clone()
#  makePlugin(P5GEnvironment, 'p5g', ['p5glove.c'])

add_subdirectory(stk)
