# Build CsoundAC

include(CheckIncludeFiles)

option(BUILD_CSOUND_AC "Build CsoundAC extension module for Csound with algorithmic composition (needs C++ interface)" ON)
option(BUILD_CSOUND_AC_PYTHON_INTERFACE "Build the Csound AC Python interface" ON)
option(BUILD_CSOUND_AC_LUA_INTERFACE "Build the Csound AC Lua interface" ON)
option(USE_SYSTEM_PORTSMF ON)

find_package(Boost)
find_library(MUSICXML_LIBRARY musicxml2)
find_package(SWIG)
find_package(PythonLibs)
find_library(LUAJIT_LIBRARY luajit)
if(NOT LUAJIT_LIBRARY)
    find_library(LUAJIT_LIBRARY luajit-5.1)
endif()
if(NOT LUAJIT_LIBRARY AND WIN32)
    find_library(LUAJIT_LIBRARY lua51)
endif()
find_path(LUA_H_PATH lua.h PATH_SUFFIXES luajit-2.0)

find_library(PORTSMF_LIBRARY portSMF)

#include_directories(${EIGEN3_INCLUDE_DIR})

find_file(EIGEN_DENSE eigen3/Eigen/Dense)

check_deps(BUILD_CSOUND_AC Boost_INCLUDE_DIRS BUILD_CXX_INTERFACE FLTK_FOUND LIBSNDFILE_LIBRARY EIGEN_DENSE)
check_deps(BUILD_CSOUND_AC_PYTHON_INTERFACE SWIG_FOUND PYTHONLIBS_FOUND EIGEN_DENSE)
check_deps(BUILD_CSOUND_AC_LUA_INTERFACE SWIG_FOUND LUAJIT_LIBRARY LUA_H_PATH EIGEN_DENSE)

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
	CounterpointMain.cpp
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

    check_deps(USE_SYSTEM_PORTSMF PORTSMF_LIBRARY)
    if(NOT USE_SYSTEM_PORTSMF)
        list(APPEND libcsoundac_SRCS
        allegro.cpp
        allegrord.cpp
        allegroserial.cpp
        allegrosmfrd.cpp
        allegrosmfwr.cpp
        allegrowr.cpp
        mfmidi.cpp
        strparse.cpp
        trace.cpp
        )
    endif()

    add_library(libCsoundAC SHARED ${libcsoundac_SRCS})
    set_target_properties(libCsoundAC PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        SOVERSION ${APIVERSION}
        OUTPUT_NAME CsoundAC
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY})
    if(USE_SYSTEM_PORTSMF)
        target_link_libraries(libCsoundAC ${PORTSMF_LIBRARY})
    endif()
    if(WIN32 AND NOT MSVC) # for MSYS2/mingw64
        target_link_libraries(libCsoundAC jpeg png z)
    endif()
    if(HAVE_UNISTD_H)
      set_target_properties(libCsoundAC PROPERTIES
        COMPILE_FLAGS "-DHAVE_UNISTD_H")
    endif()

    if(MUSICXML_LIBRARY)
        target_link_libraries(libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY})
    endif()

    install(TARGETS libCsoundAC
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR})

    if(SWIG_FOUND)
        include(${SWIG_USE_FILE})
        set(CMAKE_SWIG_OUTDIR ${BUILD_LIB_DIR})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	list(APPEND CMAKE_SWIG_FLAGS "-includeall" "-verbose")
    endif()

    if(BUILD_CSOUND_AC_PYTHON_INTERFACE)
        message(STATUS "Building CsoundAC Python interface.")
        find_package(PythonLibs)
        include_directories(${PYTHON_INCLUDE_PATH})
        # Had: 	    SWIG_FLAGS "-threads"
        set_source_files_properties(CsoundAC.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME CsoundAC)
        swig_add_module(CsoundAC python CsoundAC.i)
        swig_link_libraries(CsoundAC libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${PYTHON_LIBRARIES})

        if(MUSICXML_LIBRARY)
            swig_link_libraries(CsoundAC libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY} ${PYTHON_LIBRARIES})
        endif()


  	    if(APPLE)
            set_target_properties(${SWIG_MODULE_CsoundAC_REAL_NAME} PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup")
       	endif()

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
        message(STATUS "Building CsoundAC Lua interface.")
        include_directories(${LUA_H_PATH})

        SET_SOURCE_FILES_PROPERTIES(luaCsoundAC.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME luaCsoundAC)
        SWIG_ADD_MODULE(luaCsoundAC lua luaCsoundAC.i)
        SWIG_LINK_LIBRARIES(luaCsoundAC libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${LUAJIT_LIBRARY})

        if(MUSICXML_LIBRARY)
          SWIG_LINK_LIBRARIES(luaCsoundAC libCsoundAC libcsnd6 ${CSOUNDLIB} ${FLTK_LIBRARIES} ${LIBSNDFILE_LIBRARY} ${MUSICXML_LIBRARY} ${LUAJIT_LIBRARY})
        endif()


  	    if(APPLE)
            set_target_properties(${SWIG_MODULE_luaCsoundAC_REAL_NAME} PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup")
       	endif()

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
