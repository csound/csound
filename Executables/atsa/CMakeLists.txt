make_utility(atsa atsa_main.c libcsound_internals)
target_include_directories(atsa PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)