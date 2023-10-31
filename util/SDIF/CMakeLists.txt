# SDIF2AD Utility

if(BUILD_UTILITIES)
set(sdif2ad_SRCS sdif2adsyn.c sdif.c sdif-mem.c)
make_executable(sdif2ad "${sdif2ad_SRCS}" "")
target_link_libraries(sdif2ad PUBLIC ${CSOUNDLIB})
endif()
