# SDIF2AD Utility

if(NOT BUILD_OLPC)

    set(sdif2ad_SRCS sdif2adsyn.c sdif.c sdif-mem.c)
    
    make_executable(sdif2ad "${sdif2ad_SRCS}" "")

endif()

