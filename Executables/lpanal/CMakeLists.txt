make_utility(lpanal lpc_main.c libcsound_internals)
target_include_directories(lpanal PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)