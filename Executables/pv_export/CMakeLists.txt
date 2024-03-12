make_utility(pv_export pvx_main.c)
target_include_directories(pv_export PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)