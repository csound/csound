make_utility(pv_export pvx_main.c libcsound_internals)
target_include_directories(pv_export PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)