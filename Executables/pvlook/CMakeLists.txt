make_utility(pvlook pvl_main.c libcsound_internals)
target_include_directories(pvlook PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)