make_utility(pv_import pvi_main.c libcsound_internals)
target_include_directories(pv_import PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)