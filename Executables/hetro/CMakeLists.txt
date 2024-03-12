make_utility(hetro het_main.c libcsound_internals)
target_include_directories(hetro PRIVATE
    ${Csound_SOURCE_DIR}/Executables/sdif2ad
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)