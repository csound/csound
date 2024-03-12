make_utility(het_export hetx_main.c libcsound_internals)
target_include_directories(het_export PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)