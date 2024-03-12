make_executable(mixer-bin mixer_main.c "${CSOUNDLIB};${MATH_LIBRARY}" mixer)
target_include_directories(mixer-bin PRIVATE 
    ${libcsound_private_include_dirs}
    ${Csound_SOURCE_DIR}/Plugins/stdutil
)