option(BUILD_CSBEATS "Build the beats score frontend" ON)

check_deps(BUILD_CSBEATS FLEX_EXECUTABLE BISON_EXECUTABLE)

if(BUILD_CSBEATS)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Executables/csbeats)
    BISON_TARGET(csbeats_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/beats.y ${CMAKE_CURRENT_BINARY_DIR}/Executables/csbeats/beats.tab.c)
    FLEX_TARGET(csbeats_SCANNER ${CMAKE_CURRENT_SOURCE_DIR}/beats.l ${CMAKE_CURRENT_BINARY_DIR}/Executables/csbeats/beatslex.yy.c)

    ADD_FLEX_BISON_DEPENDENCY(csbeats_SCANNER csbeats_PARSER)

    set(csbeats_SRCS 
        main.c
        ${FLEX_csbeats_SCANNER_OUTPUTS}
        ${BISON_csbeats_PARSER_OUTPUTS}
    )

    if(MSVC)
        set_source_files_properties(${FLEX_csbeats_SCANNER_OUTPUTS} PROPERTIES
        COMPILE_FLAGS -DYY_NO_UNISTD_H)
    endif()
    make_executable(csbeats "${csbeats_SRCS}" "${MATH_LIBRARY}")

    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(csbeats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/beats)
endif()