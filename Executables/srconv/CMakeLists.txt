make_utility(srconv srconv_main.c libcsound_internals)
target_include_directories(srconv PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)
