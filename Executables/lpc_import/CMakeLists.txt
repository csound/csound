make_utility(lpc_import lpci_main.c ${MATH_LIBRARY})
target_include_directories(lpc_import PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)