make_utility(pv_import pvi_main.c)
target_include_directories(pv_import PRIVATE
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)