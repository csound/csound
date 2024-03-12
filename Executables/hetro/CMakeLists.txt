make_utility(hetro het_main.c)
target_include_directories(hetro PRIVATE
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Executables/sdif2ad
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)