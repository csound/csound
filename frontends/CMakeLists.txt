option(BUILD_PD_CLASS "Build the puredata csoundapi~ class" ON)
option(BUILD_CSBEATS "Build the beats score frontend" ON)
option(BUILD_WINSOUND "Build the Winsound frontend. Requires FLTK headers and libs." OFF)
option(BUILD_CSOUND_VST "Build the CsoundVST frontend. Requires the Steinberg VST SDK 2.3." OFF)

find_package(FLEX)
find_package(BISON)

find_path(PD_HEADER_PATH m_pd.h)
find_library(PD_LIBRARY pd.dll)

## Csound Commandline Executable ##
set(CS_MAIN_SRCS csound/csound_main.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CS_MAIN_SRCS csound/sched.c)
    list(APPEND CSOUNDLIB -lpthread)
endif()

function(make_plugin_frontend libname srcs)
    if(APPLE)
        add_library(${libname} SHARED ${srcs})
    else()
        add_library(${libname} MODULE ${srcs})
    endif()

    set(i 2)
    while( ${i} LESS ${ARGC} )
        if(NOT MSVC OR NOT("${ARGV${i}}" MATCHES "m"))
            target_link_libraries(${libname} ${ARGV${i}})
        endif()
        math(EXPR i "${i}+1")
    endwhile()

    set_target_properties(${libname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})

    install(TARGETS ${libname}
	LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" )
endfunction(make_plugin_frontend)


# We need a different name to avoid clashes with float libcsound
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB}" csound)

message(STATUS "Building csLadspa")

make_plugin_frontend(csladspa "csladspa/csladspa.cpp" "${CSOUNDLIB}")
# We do not want the lib prefix
set_target_properties(csladspa PROPERTIES PREFIX "")

# CSBeats

check_deps(BUILD_CSBEATS FLEX_EXECUTABLE BISON_EXECUTABLE)

if(BUILD_CSBEATS)

    BISON_TARGET(csbeats_PARSER beats/beats.y ${CMAKE_CURRENT_BINARY_DIR}/beats.tab.c)
    FLEX_TARGET(csbeats_SCANNER beats/beats.l ${CMAKE_CURRENT_BINARY_DIR}/beatslex.yy.c)

    ADD_FLEX_BISON_DEPENDENCY(csbeats_SCANNER csbeats_PARSER)

    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/beats)

    set(csbeats_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/beats/main.c
        ${FLEX_csbeats_SCANNER_OUTPUTS}
        ${BISON_csbeats_PARSER_OUTPUTS})

    make_executable(csbeats "${csbeats_SRCS}" "m")

endif()

#PD Interface
if(WIN_32)
    check_deps(BUILD_PD_CLASS PD_HEADER_PATH PD_LIBRARY)
else()
    check_deps(BUILD_PD_CLASS PD_HEADER_PATH)
endif()
if(BUILD_PD_CLASS)
    include_directories(${PD_HEADER_PATH})
    if(LINUX)
        set(pdname "csound6~.pd_linux")
    elseif(APPLE)
        set(pdname "csound6~.pd_darwin")
    elseif(WIN32)
        set(pdname "csound6~.dll")
    endif()
    add_library(pdcsound MODULE csoundapi_tilde/csoundapi_tilde.c)
    if(WIN32)
        target_link_libraries (pdcsound ${CSOUNDLIB} ${PD_LIBRARY})
        set_target_properties(pdcsound  PROPERTIES "LINK_FLAGS"
            "-Wl,--allow-multiple-definition")
    else()
        target_link_libraries (pdcsound ${CSOUNDLIB})
    endif()
    set_target_properties(pdcsound PROPERTIES
        PREFIX ""
        SUFFIX ""
        OUTPUT_NAME ${pdname}
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})
    if(APPLE)
        set_target_properties(pdcsound  PROPERTIES "LINK_FLAGS"
            "-flat_namespace -undefined suppress")
    endif(APPLE)
    install(TARGETS pdcsound
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR})
endif()


#WinSound
check_deps(BUILD_WINSOUND FLTK_FOUND)
if(BUILD_WINSOUND)
    set(WINSOUND_FL ${CMAKE_CURRENT_SOURCE_DIR}/winsound/winsound.fl)
    set(WINSOUND_CXX ${CMAKE_CURRENT_BINARY_DIR}/winsound.cxx)
    set(WINSOUND_H ${CMAKE_CURRENT_BINARY_DIR}/winsound.h)
    set_source_files_properties(${WINSOUND_CXX} GENERATED)
    set_source_files_properties(${WINSOUND_H} GENERATED)
    add_custom_target(winsound_fl COMMAND fluid -c -o ${WINSOUND_CXX} -h  ${WINSOUND_H} ${WINSOUND_FL})

    set(WINSOUND_SRCS ${WINSOUND_CXX} winsound/main.cxx)

    make_executable(winsound "${WINSOUND_SRCS}" "${FLTK_LIBRARIES}")
    target_link_libraries (winsound ${CSOUNDLIB})
    add_dependencies(winsound winsound_fl)
    include_directories(${FLTK_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/winsound)


endif()

# debugger command line app
make_executable(csdebugger debugger/csdebugger.cpp "${CSOUNDLIB}")

# icsound interactive python API
if(PYTHON_MODULE_INSTALL_DIR)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icsound/icsound.py
            DESTINATION ${PYTHON_MODULE_INSTALL_DIR})
endif()

# Others
add_subdirectory(CsoundAC)
add_subdirectory(CsoundVST)
add_subdirectory(max_csound_tilde)
