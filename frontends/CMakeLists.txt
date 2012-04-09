option(BUILD_PD_CLASS "Build the puredata csoundapi~ class" ON)
option(BUILD_TCLCSOUND "Build the tcl interface and clients" ON)
option(BUILD_CSBEATS "Build the beats score frontend" ON)
option(BUILD_WINSOUND "Build the Winsound Frontent. REquies FLTK headers and libs." OFF)

find_package(FLEX)
find_package(BISON)
find_package(FLTK)


check_include_file(m_pd.h PD_HEADER)

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

check_deps(BUILD_CSBEATS FLEX_EXECUTABLE BISON_EXECUTABLE)

if(BUILD_CSBEATS)
    
    BISON_TARGET(csbeats_PARSER beats/beats.y ${CMAKE_CURRENT_BINARY_DIR}/beats.tab.c)
    FLEX_TARGET(csbeats_SCANNER beats/beats.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

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
check_deps(BUILD_PD_CLASS PD_HEADER)
if(BUILD_PD_CLASS)
    if(LINUX)
        set(pdname "csoundapi~.pd_linux")
    elseif(APPLE)
        set(pdname "csoundapi~.pd_darwin")
    elseif(WIN32)
        set(pdname "csoundapi~.dll")
    endif()

    add_library(${pdname} MODULE csoundapi_tilde/csoundapi_tilde.c)
    target_link_libraries (${pdname} ${CSOUNDLIB})
    set_target_properties(${pdname} PROPERTIES
        PREFIX ""
        SUFFIX "")

    install(TARGETS ${pdname}
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
    add_custom_target(winsound_fl COMMAND fluid -c -o ${WINSOUND_CXX} -h ${WINSOUND_H} ${WINSOUND_FL}) 
    
    set(WINSOUND_SRCS ${WINSOUND_CXX} winsound/main.cxx)

    make_executable(winsound "${WINSOUND_SRCS}" "${FLTK_LIBRARIES}")
    add_dependencies(winsound winsound_fl)    
    include_directories(${FLTK_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/winsound)


endif()

add_subdirectory(CsoundAC)
