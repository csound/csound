make_utility(dnoise dnoise_main.c libcsound_internals)
target_include_directories(dnoise PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)