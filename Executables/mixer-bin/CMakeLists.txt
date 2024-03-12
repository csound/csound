make_executable(mixer-bin mixer_main.c "${CSOUNDLIB};${MATH_LIBRARY};libcsound_internals" mixer)
target_include_directories(mixer-bin PRIVATE ${Csound_SOURCE_DIR}/Plugins/stdutil)