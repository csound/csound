# Build CsoundVST

include(CheckIncludeFiles)

option(BUILD_CSOUNDVST "Build CsoundVST plugin (needs C++ interface, FLTK, VST SDK)" OFF)
IF(APPLE)
find_library(FLTK libfltk)
ENDIF(APPLE)

find_file(EIGEN_DENSE Dense ${EIGEN3_INCLUDE_DIR}/eigen3/Eigen)


check_deps(BUILD_CSOUNDVST Boost_FOUND BUILD_CXX_INTERFACE FLTK_FOUND LIBSNDFILE_LIBRARY EIGEN_DENSE VSTSDK2X_FOUND)
message(STATUS "BUILD_CSOUNDVST: ${BUILD_CSOUNDVST}.")
if(BUILD_CSOUNDVST)
    include_directories(${CMAKE_HOME_DIRECTORY}/interfaces)
    include_directories(${CMAKE_HOME_DIRECTORY}/frontends/CsoundVST)
    include_directories(${EIGEN3_INCLUDE_DIR})
    include_directories(${VSTSDK2X_INCLUDE_DIR})
    include_directories(${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x)
    include_directories(${VSTSDK2X_INCLUDE_DIR}/pluginterfaces/source/vst2.x)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${FLTK_INCLUDE_DIR})
    add_definitions(-DVST_FORCE_DEPRECATED=0)
    set(CSOUNDVST_SRCS
        CsoundVst.cpp
        CsoundVstFltk.cpp
        CsoundVstUi.cpp
        ${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
        ${VSTSDK2X_INCLUDE_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp)
    add_compile_options("-O2")
    add_library(CsoundVST SHARED ${CSOUNDVST_SRCS})
    set_target_properties(CsoundVST PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        OUTPUT_NAME CsoundVST
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(CsoundVST libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})

    if(MUSICXML_LIBRARY)
        target_link_libraries(CsoundVST libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY})
    endif()

    set(CSOUNDVST_MAIN_SRCS
        csoundvst_main.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp)
    add_executable(csoundvstmain ${CSOUNDVST_MAIN_SRCS})
    set_target_properties(csoundvstmain PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        OUTPUT_NAME csoundvstmain
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(csoundvstmain ${CsoundVST} libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})

    if(MUSICXML_LIBRARY)
        target_link_libraries(csoundvstmain ${CsoundVST} libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY})
    endif()


    install(TARGETS CsoundVST
        RUNTIME DESTINATION ${LIBRARY_INSTALL_DIR}
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR})

    install(TARGETS csoundvstmain
        RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}")

endif()
