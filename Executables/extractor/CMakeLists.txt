make_utility(extractor xtrc_main.c)
target_include_directories(extractor PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)