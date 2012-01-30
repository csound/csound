# Build CsoundAC

option(BUILD_CSOUND_AC "Build CsoundAC extension module for Csound with algorithmic composition (needs C++ interface)" ON)
option(BUILD_CSOUND_AC_PYTHON_INTERFACE "Build the Csound AC Python interface" ON)
option(BUILD_CSOUND_AC_LUA_INTERFACE "Build the Csound AC Lua interface" ON)


find_package(Boost)
find_library(MUSICXML_LIBRARY musicxml2)
find_package(FLTK)
find_package(SWIG)
find_package(PythonLibs)
find_package(Lua51)

check_deps(BUILD_CSOUND_AC Boost_FOUND BUILD_CXX_INTERFACE FLTK_FOUND LIBSNDFILE_LIBRARY)
check_deps(BUILD_CSOUND_AC_PYTHON_INTERFACE SWIG_FOUND PYTHONLIBS_FOUND)
check_deps(BUILD_CSOUND_AC_LUA_INTERFACE SWIG_FOUND LUA51_FOUND)

if(BUILD_CSOUND_AC)
    include_directories(${CMAKE_HOME_DIRECTORY}/interfaces)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${FLTK_INCLUDE_DIR})
    set(libcsoundac_SRCS 
        Cell.cpp
        ChordLindenmayer.cpp
        Composition.cpp
        Conversions.cpp
        Counterpoint.cpp
        CounterpointNode.cpp
        Event.cpp
        Hocket.cpp
        ImageToScore.cpp
        Lindenmayer.cpp
        MCRM.cpp
        Midifile.cpp
        MusicModel.cpp
        Node.cpp
        Random.cpp
        Rescale.cpp
        Score.cpp
        ScoreModel.cpp
        ScoreNode.cpp
        Sequence.cpp
        Shell.cpp
        Soundfile.cpp
        StrangeAttractor.cpp
        System.cpp
        Voicelead.cpp
        VoiceleadingNode.cpp)
    
    add_library(libCsoundAC SHARED ${libcsoundac_SRCS})
    set_target_properties(libCsoundAC PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        SOVERSION ${APIVERSION}
        OUTPUT_NAME CsoundAC
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(libCsoundAC libcsnd ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})
    
    if(MUSICXML_LIBRARY) 
        target_link_libraries(CsoundAC MUSICXML_LIBRARY)
    endif()

    install(TARGETS libCsoundAC
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR})

    if(SWIG_FOUND)
        include(${SWIG_USE_FILE})
        set(CMAKE_SWIG_OUTDIR ${BUILD_LIB_DIR})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(BUILD_CSOUND_AC_PYTHON_INTERFACE)
        message(STATUS "Building CsoundAC Python interface")
        find_package(PythonLibs)
        include_directories(${PYTHON_INCLUDE_PATH})
        set_source_files_properties(CsoundAC.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME CsoundAC)
        swig_add_module(CsoundAC python CsoundAC.i)
        swig_link_libraries(CsoundAC libCsoundAC ${PYTHON_LIBRARIES})

        set_target_properties(${SWIG_MODULE_CsoundAC_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

        install(TARGETS ${SWIG_MODULE_CsoundAC_REAL_NAME}
            LIBRARY DESTINATION "${PYTHON_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${PYTHON_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/CsoundAC.py
            DESTINATION ${PYTHON_MODULE_INSTALL_DIR})
    endif()

    if(BUILD_CSOUND_AC_LUA_INTERFACE)
        message(STATUS "Building CsoundAC Lua interface")
        find_package(Lua51)
        include_directories(${LUA_INCLUDE_DIR})

        SET_SOURCE_FILES_PROPERTIES(luaCsoundAC.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME luaCsoundAC)
        SWIG_ADD_MODULE(luaCsoundAC lua luaCsoundAC.i)
        SWIG_LINK_LIBRARIES(luaCsoundAC libCsoundAC ${LUA_LIBRARIES})

        set_target_properties(${SWIG_MODULE_luaCsoundAC_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            PREFIX "")

        install(TARGETS luaCsoundAC
            LIBRARY DESTINATION "${LUA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${LUA_MODULE_INSTALL_DIR}")
    endif()

endif()
