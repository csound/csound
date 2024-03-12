make_utility(sndinfo sndinfo_main.c)
target_include_directories(sndinfo PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)