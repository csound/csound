make_utility(het_import heti_main.c)
target_include_directories(het_import PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)