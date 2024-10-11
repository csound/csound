# UTILITY PLUGIN AND PROGRAMS
option(BUILD_SRC_CONV "Build src_conv utility" ON)

set(stdutil_SRCS
    atsa.c          cvanal.c        dnoise.c    envext.c
    het_export.c    het_import.c    hetro.c     lpanal.c
    lpc_export.c    lpc_import.c    mixer.c     pvanal.c
    pv_export.c     pv_import.c     pvlook.c    scale.c
    sndinfo.c       srconv.c        std_util.c  xtrct.c
    SDIF/sdif.c )

if(BUILD_UTILITIES)
   make_plugin(stdutil "${stdutil_SRCS}" ${MATH_LIBRARY} SndFile::sndfile)
    make_utility(atsa        atsa_main.c)
    make_utility(csanalyze   csanalyze.c)
    make_utility(cvanal      cvl_main.c)
    make_utility(dnoise      dnoise_main.c)
    make_utility(envext      env_main.c)
    make_utility(extractor   xtrc_main.c)
    make_utility(het_export  hetx_main.c)
    make_utility(het_import  heti_main.c)
    make_utility(hetro       het_main.c)
    make_utility(lpanal      lpc_main.c)
    make_utility(lpc_export  lpcx_main.c ${MATH_LIBRARY})
    make_utility(lpc_import  lpci_main.c ${MATH_LIBRARY})
    make_executable(mixer-bin   mixer_main.c   "${CSOUNDLIB};${MATH_LIBRARY}" mixer)
    make_utility(pvanal      pvc_main.c)
    make_utility(pvlook      pvl_main.c)
    make_utility(pv_export   pvx_main.c)
    make_utility(pv_import   pvi_main.c)
    make_utility(scale       scale_main.c)
    make_utility(sndinfo     sndinfo_main.c)
    make_utility(srconv      srconv_main.c)

    if (BUILD_SRC_CONV)
        if (USE_VCPKG)
            find_package(SampleRate CONFIG REQUIRED)
            assign_bool(SampleRate_FOUND TARGET SampleRate::samplerate)
        else()
            find_package(SampleRate MODULE)
        endif()
    endif()

    check_deps(BUILD_SRC_CONV USE_LIBSNDFILE SampleRate_FOUND)
    if (BUILD_SRC_CONV)
        make_executable(src_conv new_srconv.c "SndFile::sndfile;SampleRate::samplerate")
    endif()

endif()
add_subdirectory(SDIF)
