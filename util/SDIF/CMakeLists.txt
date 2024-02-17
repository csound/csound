# SDIF2AD Utility

if(BUILD_UTILITIES)
set(sdif2ad_SRCS sdif2adsyn.c sdif.c sdif-mem.c)
make_utility(sdif2ad "${sdif2ad_SRCS}" "")
endif()
