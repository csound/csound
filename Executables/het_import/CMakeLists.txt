make_utility(het_import heti_main.c libcsound_internals)
target_include_directories(het_import PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)