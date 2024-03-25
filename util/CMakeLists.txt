# UTILITY PLUGIN AND PROGRAMS

set(stdutil_SRCS
    atsa.c          cvanal.c        dnoise.c    envext.c
    het_export.c    het_import.c    hetro.c     lpanal.c
    lpc_export.c    lpc_import.c    mixer.c     pvanal.c
    pv_export.c     pv_import.c     pvlook.c    scale.c
    sndinfo.c       srconv.c        std_util.c  xtrct.c
    SDIF/sdif.c   ../InOut/soundfile.c)

if(BUILD_UTILITIES)
    make_plugin(stdutil "${stdutil_SRCS}" ${MATH_LIBRARY} libcsound_internals)
    if (USE_LIBSNDFILE)
        target_link_libraries(stdutil PRIVATE SndFile::sndfile)
    endif()
    make_utility(atsa        atsa_main.c libcsound_internals)
    make_utility(csanalyze   csanalyze.c)
    make_utility(cvanal      cvl_main.c libcsound_internals)
    make_utility(dnoise      dnoise_main.c libcsound_internals)
    make_utility(envext      env_main.c libcsound_internals)
    make_utility(extractor   xtrc_main.c libcsound_internals)
    make_utility(het_export  hetx_main.c libcsound_internals)
    make_utility(het_import  heti_main.c libcsound_internals)
    make_utility(hetro       het_main.c libcsound_internals)
    make_utility(lpanal      lpc_main.c libcsound_internals)
    make_utility(lpc_export  lpcx_main.c ${MATH_LIBRARY} libcsound_internals)
    make_utility(lpc_import  lpci_main.c ${MATH_LIBRARY} libcsound_internals)
    make_executable(mixer-bin   mixer_main.c   "${CSOUNDLIB};${MATH_LIBRARY};libcsound_internals" mixer)
    make_utility(pvanal      pvc_main.c libcsound_internals)
    make_utility(pvlook      pvl_main.c libcsound_internals)
    make_utility(pv_export   pvx_main.c libcsound_internals)
    make_utility(pv_import   pvi_main.c libcsound_internals)
    make_utility(scale       scale_main.c libcsound_internals)
    make_utility(sndinfo     sndinfo_main.c libcsound_internals)
    make_utility(srconv      srconv_main.c libcsound_internals)

    if (BUILD_SRC_CONV)
        make_executable(src_conv new_srconv.c "SndFile::sndfile;SampleRate::samplerate")
        if(USE_GETTEXT)
            target_compile_definitions(src_conv PRIVATE "GNU_GETTEXT")
            target_link_libraries(src_conv PRIVATE ${CSOUNDLIB})
        endif()
    endif()

endif()
add_subdirectory(SDIF)
