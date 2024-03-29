option(BUILD_CSBEATS "Build the beats score frontend" ON)

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
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB};${MATH_LIBRARY}" csound)

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
    endif()
    make_executable(csbeats "${csbeats_SRCS}" "${MATH_LIBRARY}")

    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/beats)
endif()

# debugger command line app
make_executable(csdebugger debugger/csdebugger.cpp "${CSOUNDLIB}")


