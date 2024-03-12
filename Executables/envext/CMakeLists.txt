make_utility(envext env_main.c libcsound_internals)
target_include_directories(envext PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)