make_utility(lpc_export lpcx_main.c ${MATH_LIBRARY})
target_include_directories(lpc_export PRIVATE
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)