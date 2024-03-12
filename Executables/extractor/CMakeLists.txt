make_utility(extractor xtrc_main.c libcsound_internals)
target_include_directories(extractor PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)