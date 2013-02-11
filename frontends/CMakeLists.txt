option(BUILD_PD_CLASS "Build the puredata csoundapi~ class" ON)
option(BUILD_CSBEATS "Build the beats score frontend" ON)
option(BUILD_WINSOUND "Build the Winsound frontend. Requires FLTK headers and libs." OFF)


  # first, try to use fltk-config for fltk >= 1.3 (FindFLTK is buggy
  # on Unix, where e.g. xft and xinerama options are not dealt with)
  find_program(FLTK_CONFIG_SCRIPT fltk-config)
  if(FLTK_CONFIG_SCRIPT)
    execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --api-version
                    OUTPUT_VARIABLE FLTK_VERSION)
    string(STRIP ${FLTK_VERSION} FLTK_VERSION)
    if(FLTK_VERSION GREATER 1.1)
      set(HAVE_FLTK "Fltk")
      set(FLTK_FOUND 1)
      message(STATUS "Using fltk-config script for Fltk " ${FLTK_VERSION})
      execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --includedir
                      OUTPUT_VARIABLE FLTK_INCLUDE_DIR)
      string(STRIP ${FLTK_INCLUDE_DIR} FLTK_INCLUDE_DIR)
      execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --ldflags
                      OUTPUT_VARIABLE FLTK_LIBRARIES)
      string(STRIP ${FLTK_LIBRARIES} FLTK_LIBRARIES)
      string(REGEX MATCH "fltk[_ ]jpeg" FLTK_JPEG ${FLTK_LIBRARIES})
      string(REGEX MATCH "fltk[_ ]z" FLTK_Z ${FLTK_LIBRARIES})
      string(REGEX MATCH "fltk[_ ]png" FLTK_PNG ${FLTK_LIBRARIES})
    endif(FLTK_VERSION GREATER 1.1)
  endif(FLTK_CONFIG_SCRIPT)
  if(NOT HAVE_FLTK) 

    set(FLTK_SKIP_OPENGL true)
    find_package(FLTK)
  endif()

find_package(FLEX)
find_package(BISON)

check_include_file(m_pd.h PD_HEADER)

## Csound Commandline Executable ##
set(CS_MAIN_SRCS csound/csound_main.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CS_MAIN_SRCS csound/sched.c)
    list(APPEND CSOUNDLIB -lpthread)
endif()

# We need a different name to avoid clashes with float libcsound
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB}" csound)

message(STATUS "Building csLadspa")
make_plugin(csladspa "csladspa/csladspa.cpp" "${CSOUNDLIB}")
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
    if(APPLE)
    set_target_properties(${pdname}  PROPERTIES "LINK_FLAGS"
    "-flat_namespace -undefined suppress")
    ENDIF(APPLE)
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
    target_link_libraries (winsound ${CSOUNDLIB})
    add_dependencies(winsound winsound_fl)    
    include_directories(${FLTK_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/winsound)


endif()

add_subdirectory(CsoundAC)
