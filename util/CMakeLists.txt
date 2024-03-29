# UTILITY PLUGIN AND PROGRAMS

set(stdutil_SRCS
    atsa.c          cvanal.c        dnoise.c    envext.c
    het_export.c    het_import.c    hetro.c     lpanal.c
    lpc_export.c    lpc_import.c    mixer.c     pvanal.c
    pv_export.c     pv_import.c     pvlook.c    scale.c
    sndinfo.c       srconv.c        std_util.c  xtrct.c
    SDIF/sdif.c)

if(MSVC)
    set(LIBSNDFILE_LIBRARY SndFile::sndfile)
endif()

make_plugin(stdutil "${stdutil_SRCS}" ${MATH_LIBRARY} ${LIBSNDFILE_LIBRARY} ${LIBSNDFILE_SUPPORT_LIBS})

if(BUILD_UTILITIES)
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


    #find_library(LIBSNDFILE_LIBRARY sndfile libsndfile-1)
    find_library(LIBSAMPLERATE_LIBRARY NAMES samplerate libsamplerate-0)

    if(LIBSNDFILE_LIBRARY AND LIBSAMPLERATE_LIBRARY)
        make_executable(src_conv new_srconv.c "${LIBSNDFILE_LIBRARY};${LIBSAMPLERATE_LIBRARY}")
        if(MSVC)
            target_link_libraries(src_conv ${LIBSNDFILE_SUPPORT_LIBS})
        endif()
    else()
        message(STATUS "Not building src_conv (libsndfile or libsamplerate not found).")
    endif()

endif()
add_subdirectory(SDIF)
