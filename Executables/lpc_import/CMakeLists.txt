make_utility(lpc_import lpci_main.c "${MATH_LIBRARY};libcsound_internals")
target_include_directories(lpc_import PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)