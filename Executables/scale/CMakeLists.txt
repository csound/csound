make_utility(scale scale_main.c)
target_include_directories(scale PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)