make_utility(pvlook pvl_main.c)
target_include_directories(pvlook PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)