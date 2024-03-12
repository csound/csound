make_utility(sndinfo sndinfo_main.c libcsound_internals)
target_include_directories(sndinfo PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)