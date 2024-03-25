make_utility(lpc_export lpcx_main.c "${MATH_LIBRARY};libcsound_internals")
target_include_directories(lpc_export PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)