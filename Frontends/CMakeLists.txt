option(BUILD_CSBEATS "Build the beats score frontend" ON)
option(BUILD_WINSOUND "Build the Winsound frontend. Requires FLTK headers and libs." OFF)

if (NOT MSVC)
    find_package(FLEX)
    find_package(BISON)
endif()

find_path(PD_HEADER_PATH m_pd.h)
find_library(PD_LIBRARY pd.dll)

## Csound Commandline Executable ##
set(CS_MAIN_SRCS csound/csound_main.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CS_MAIN_SRCS csound/sched.c)
    list(APPEND CSOUNDLIB -lpthread)
endif()

# We need a different name to avoid clashes with float libcsound
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB}" csound)
if(LINUX)
  target_link_libraries(csound-bin m)
endif()

# Post build dependencies copy for MSVC
if(MSVC)
	add_custom_command(TARGET csound-bin
					   POST_BUILD
					   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/msvc/deps/bin
						$<TARGET_FILE_DIR:csound-bin>
					   COMMENT "Running copy dependencies script...")
endif()


# CsBeats
check_deps(BUILD_CSBEATS FLEX_EXECUTABLE BISON_EXECUTABLE)

if(BUILD_CSBEATS)
    BISON_TARGET(csbeats_PARSER beats/beats.y ${CMAKE_CURRENT_BINARY_DIR}/beats.tab.c)
    FLEX_TARGET(csbeats_SCANNER beats/beats.l ${CMAKE_CURRENT_BINARY_DIR}/beatslex.yy.c)

    ADD_FLEX_BISON_DEPENDENCY(csbeats_SCANNER csbeats_PARSER)

    set(csbeats_SRCS
    	${CMAKE_CURRENT_SOURCE_DIR}/beats/main.c
        ${FLEX_csbeats_SCANNER_OUTPUTS}
        ${BISON_csbeats_PARSER_OUTPUTS})


    if(MSVC)
        set_source_files_properties(${FLEX_csbeats_SCANNER_OUTPUTS} PROPERTIES
        COMPILE_FLAGS -DYY_NO_UNISTD_H)
      make_executable(csbeats "${csbeats_SRCS}" "")
    else()
      make_executable(csbeats "${csbeats_SRCS}" "m")
    endif()

    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/beats)
endif()

#WinSound
check_deps(BUILD_WINSOUND FLTK_FOUND)
if(BUILD_WINSOUND)
    set(WINSOUND_FL ${CMAKE_CURRENT_SOURCE_DIR}/winsound/winsound.fl)
    set(WINSOUND_CXX ${CMAKE_CURRENT_BINARY_DIR}/winsound.cxx)
    set(WINSOUND_H ${CMAKE_CURRENT_BINARY_DIR}/winsound.h)
    set_source_files_properties(${WINSOUND_CXX} GENERATED)
    set_source_files_properties(${WINSOUND_H} GENERATED)

	find_program(FLUID_BIN NAMES fluid)
	if(FLUID_BIN)
		add_custom_target(winsound_fl COMMAND ${FLUID_BIN} -c -o ${WINSOUND_CXX} -h  ${WINSOUND_H} ${WINSOUND_FL})
	else()
		message(WARNING "Could not find fluid executable")
	endif()

    set(WINSOUND_SRCS ${WINSOUND_CXX} winsound/main.cxx)

    make_executable(winsound "${WINSOUND_SRCS}" "${FLTK_LIBRARIES}")
    if(NOT MSVC)
       set_target_properties(winsound 
            PROPERTIES COMPILE_FLAGS "-Wno-nonportable-include-path")
       target_link_libraries(winsound -ldl)
    endif()
    target_link_libraries (winsound ${CSOUNDLIB})
    add_dependencies(winsound winsound_fl)
    target_include_directories(winsound PRIVATE ${FLTK_INCLUDE_DIR})
    target_include_directories(winsound PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(winsound PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/winsound)
endif()

# debugger command line app
make_executable(csdebugger debugger/csdebugger.cpp "${CSOUNDLIB}")


