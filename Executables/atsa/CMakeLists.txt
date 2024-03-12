make_utility(atsa atsa_main.c)
target_include_directories(atsa PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)