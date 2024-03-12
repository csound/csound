make_utility(envext env_main.c)
target_include_directories(envext PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)