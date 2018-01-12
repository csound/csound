# Build CsoundVST

include(CheckIncludeFiles)

option(BUILD_CSOUND_VST "Build CsoundVST plugin (needs C++ interface, FLTK, VST SDK)" OFF)
IF(APPLE)
find_library(FLTK libfltk)
ENDIF(APPLE)

check_deps(BUILD_CSOUND_VST BUILD_CXX_INTERFACE FLTK_FOUND LIBSNDFILE_LIBRARY EIGEN3_INCLUDE_PATH VSTSDK2X_FOUND)
message(STATUS "BUILD_CSOUND_VST: ${BUILD_CSOUND_VST}.")
if(BUILD_CSOUND_VST)
    include_directories(${CMAKE_HOME_DIRECTORY}/interfaces)
    include_directories(${CMAKE_HOME_DIRECTORY}/frontends/CsoundVST)
    include_directories(${EIGEN3_INCLUDE_PATH})
    include_directories(${VSTSDK2X_INCLUDE_DIR})
    include_directories(${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x)
    include_directories(${VSTSDK2X_INCLUDE_DIR}/pluginterfaces/source/vst2.x)
    include_directories(${FLTK_INCLUDE_DIR})
    set(CSOUNDVST_SRCS
        CsoundVST.cpp
        CsoundVstFltk.cpp
        CsoundVstUi.cpp
        ${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
        ${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp)
    add_library(CsoundVST SHARED ${CSOUNDVST_SRCS})
    target_compile_options(CsoundVST PRIVATE -DVST_FORCE_DEPRECATED=0 -O2)
    set_target_properties(CsoundVST PROPERTIES
        OUTPUT_NAME CsoundVST
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(CsoundVST PRIVATE libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})

    if(MUSICXML_LIBRARY)
        target_link_libraries(CsoundVST PRIVATE libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY})
    endif()

    set(CSOUNDVST_MAIN_SRCS
        csoundvst_main.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp)
    add_executable(csoundvstmain ${CSOUNDVST_MAIN_SRCS})
    set_target_properties(csoundvstmain PROPERTIES
        OUTPUT_NAME csoundvstmain
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(csoundvstmain PRIVATE CsoundVST libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})

    if(MUSICXML_LIBRARY)
        target_link_libraries(csoundvstmain PRIVATE CsoundVST libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY})
    endif()

    install(TARGETS CsoundVST
        RUNTIME DESTINATION ${LIBRARY_INSTALL_DIR}
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR})

    install(TARGETS csoundvstmain
        RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}")

endif()


