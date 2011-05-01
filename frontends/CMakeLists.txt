find_package(FLEX)
find_package(BISON)

## Csound Commandline Executable ##
set(CS_MAIN_SRCS csound/csound_main.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CS_MAIN_SRCS csound/sched.c)
endif()


# We need a different name to avoid clashes with float libcsound
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB}" csound)

message(STATUS "Building csLadspa")
make_plugin(csladspa "csladspa/csladspa.cpp" "${CSOUNDLIB}")
# We do not want the lib prefix
set_target_properties(csladspa PROPERTIES PREFIX "")

# TCL interface

option(BUILD_TCLCSOUND "Build the tcl interface and clients" ON)

if(BUILD_TCLCSOUND)
    set(TCL_VERSION "" CACHE STRING "Try a specific tcl/tk version first")
    set(TCL_LIBRARY "tcl${TCL_VERSION}")
    set(TK_LIBRARY "tk${TCL_VERSION}")
    find_package(TCL)
endif()
check_deps(BUILD_TCLCSOUND TCL_FOUND TK_FOUND)
if(BUILD_TCLCSOUND)
    include_directories(${TCL_INCLUDE_PATH} ${TK_INCLUDE_PATH})
    make_executable(cstclsh "tclcsound/main_tclsh.c;tclcsound/commands.c" "${CSOUNDLIB};${TCL_LIBRARY}")
    make_executable(cswish "tclcsound/main_wish.c;tclcsound/commands.c" "${CSOUNDLIB};${TCL_LIBRARY};${TK_LIBRARY}")
    add_library(tclcsound SHARED tclcsound/tclcsound.c tclcsound/commands.c)
    target_link_libraries(tclcsound ${CSOUNDLIB} ${TCL_LIBRARY})
    set_target_properties(tclcsound PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        PREFIX ""
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

    install(TARGETS tclcsound cswish cstclsh
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
        RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}")

endif()

# CSBeats

option(BUILD_CSBEATS "Build the beats score frontend" ON)
check_deps(BUILD_CSBEATS FLEX_EXECUTABLE BISON_EXECUTABLE)

if(BUILD_CSBEATS)
    
    BISON_TARGET(csbeats_PARSER beats/beats.y ${CMAKE_CURRENT_BINARY_DIR}/beats.tab.c)
    FLEX_TARGET(csbeats_SCANNER beats/beats.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

    ADD_FLEX_BISON_DEPENDENCY(csbeats_SCANNER csbeats_PARSER)

    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(csbeats_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/beats/main.c
        ${FLEX_csbeats_SCANNER_OUTPUTS}
        ${BISON_csbeats_PARSER_OUTPUTS})

    make_executable(csbeats "${csbeats_SRCS}" "m")
    
endif()

add_subdirectory(CsoundAC)
