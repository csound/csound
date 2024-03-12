# UTILITY PLUGIN AND PROGRAMS

set(stdutil_SRCS
    ${Csound_SOURCE_DIR}/Executables/atsa/atsa.c
    ${Csound_SOURCE_DIR}/Executables/cvanal/cvanal.c
    ${Csound_SOURCE_DIR}/Executables/dnoise/dnoise.c
    ${Csound_SOURCE_DIR}/Executables/envext/envext.c
    ${Csound_SOURCE_DIR}/Executables/het_export/het_export.c
    ${Csound_SOURCE_DIR}/Executables/het_import/het_import.c
    ${Csound_SOURCE_DIR}/Executables/hetro/hetro.c
    ${Csound_SOURCE_DIR}/Executables/lpanal/lpanal.c
    ${Csound_SOURCE_DIR}/Executables/lpc_export/lpc_export.c
    ${Csound_SOURCE_DIR}/Executables/lpc_import/lpc_import.c
    ${Csound_SOURCE_DIR}/Executables/mixer-bin/mixer.c
    ${Csound_SOURCE_DIR}/Executables/pvanal/pvanal.c
    ${Csound_SOURCE_DIR}/Executables/pv_export/pv_export.c
    ${Csound_SOURCE_DIR}/Executables/pv_import/pv_import.c
    ${Csound_SOURCE_DIR}/Executables/pvlook/pvlook.c
    ${Csound_SOURCE_DIR}/Executables/scale/scale.c
    ${Csound_SOURCE_DIR}/Executables/sndinfo/sndinfo.c
    ${Csound_SOURCE_DIR}/Executables/srconv/srconv.c
    std_util.c
    ${Csound_SOURCE_DIR}/Executables/extractor/xtrct.c
    ${Csound_SOURCE_DIR}/Executables/sdif2ad/sdif.c
    ${Csound_SOURCE_DIR}/InOut/soundfile.c
)

make_plugin(stdutil "${stdutil_SRCS}" ${MATH_LIBRARY} SndFile::sndfile)
target_include_directories(stdutil PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Csound_SOURCE_DIR}/Executables/sdif2ad
)
