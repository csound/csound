make_utility(dnoise dnoise_main.c)
target_include_directories(dnoise PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)