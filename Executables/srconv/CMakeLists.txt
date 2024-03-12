make_utility(srconv srconv_main.c)
target_include_directories(srconv PRIVATE
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)