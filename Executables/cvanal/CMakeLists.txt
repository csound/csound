make_utility(cvanal cvl_main.c)
target_include_directories(cvanal PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)