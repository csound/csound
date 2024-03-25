make_utility(cvanal cvl_main.c libcsound_internals)
target_include_directories(cvanal PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)