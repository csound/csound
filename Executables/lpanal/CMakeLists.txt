make_utility(lpanal lpc_main.c)
target_include_directories(lpanal PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)