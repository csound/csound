make_utility(pvanal pvc_main.c libcsound_internals)
target_include_directories(pvanal PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)