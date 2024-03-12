make_utility(pvanal pvc_main.c)
target_include_directories(pvanal PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)