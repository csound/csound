# UTILITY PLUGIN AND PROGRAMS

set(stdutil_SRCS
    cvanal.c    dnoise.c    envext.c    lpanal.c 
    mixer.c     pvanal.c    pvlook.c    scale.c
    sndinfo.c   srconv.c    std_util.c)
    
if(NOT buildOLPC ) 
    list(APPEND stdutil_SRCS
        atsa.c      xtrct.c         het_export.c    het_import.c
        hetro.c     lpc_export.c    lpc_import.c
        pv_export.c pv_import.c     ../SDIF/sdif.c)
endif()

make_plugin(stdutil "${stdutil_SRCS}" m ${LIBSNDFILE_LIBRARY})

if(NOT BUILD_OLPC AND BUILD_UTILITIES) 
    make_utility(atsa        atsa_main.c)
    make_utility(cvanal      cvl_main.c)
    make_utility(dnoise      dnoise_main.c)
    make_utility(envext      env_main.c)
    make_utility(extractor   xtrc_main.c)
    make_utility(het_export  hetx_main.c)
    make_utility(het_import  heti_main.c)
    make_utility(hetro       het_main.c)
    make_utility(lpanal      lpc_main.c)
    make_utility(lpc_export  lpcx_main.c)
    make_utility(lpc_import  lpci_main.c)
    make_executable(mixer-bin   mixer_main.c   "${CSOUNDLIB}" mixer)
    make_utility(pvanal      pvc_main.c)
    make_utility(pvlook      pvl_main.c)
    make_utility(pv_export   pvx_main.c)
    make_utility(pv_import   pvi_main.c)
    make_utility(scale       scale_main.c)
    make_utility(sndinfo     sndinfo_main.c)
    make_utility(srconv      srconv_main.c)
endif()
