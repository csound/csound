cmake_minimum_required(VERSION 2.6)

project (Csound)

message(STATUS "${CMAKE_HOME_DIRECTORY}")

# Project definitions
set(APIVERSION "5.2")

# Relative install paths
set(EXECUTABLE_INSTALL_DIR "bin")
set(LIBRARY_INSTALL_DIR "lib")
if(USE_DOUBLE)
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
else()
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
endif()
set(PYTHON_MODULE_INSTALL_DIR ${LIBRARY_INSTALL_DIR})
set(JAVA_MODULE_INSTALL_DIR ${LIBRARY_INSTALL_DIR})
set(LUA_MODULE_INSTALL_DIR ${LIBRARY_INSTALL_DIR})

# Include this after the install path definitions so we can override them here.
find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_HOME_DIRECTORY})
if(CUSTOM_CMAKE)
    message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
    include(${CUSTOM_CMAKE})
endif()

include(TestBigEndian)

# Utility function to make plugins. All plugin targets should use this as it
# sets up output directory set in top-level CmakeLists.txt
# and adds the appropriate install target
#
# libname - name of library to produce
# srcs - list of src files (must be quoted if a list)
# extralibs (OPTIONAL) - extra libraries to link the plugin to
#  
function(make_plugin libname srcs)
    if(APPLE)
        add_library(${libname} SHARED ${srcs})
    else()
        add_library(${libname} MODULE ${srcs})
    endif()

    set(i 2)
    while( ${i} LESS ${ARGC} )
        target_link_libraries(${libname} ${ARGV${i}})
        math(EXPR i "${i}+1")
    endwhile()
    
    set_target_properties(${libname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})

    install(TARGETS ${libname}
	LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" )
endfunction(make_plugin)
# Utility function to make executables. All plugin targets should use this as it
# sets up output directory set in top-level CmakeLists.txt
# and adds an appropriate install target
#
# name - name of executable to produce
# srcs - list of src files
# libs - list of library files to link to
# output_name (OPTIONAL) - overide the name of the generated executable
#
function(make_executable name srcs libs) 
    add_executable(${name} ${srcs})
    target_link_libraries (${name} ${libs})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR})

    if(${ARGC} EQUAL 4) 
        set_target_properties(${name} PROPERTIES            
            OUTPUT_NAME ${ARGV3})
    endif()
    install(TARGETS ${name}
	RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}" )
endfunction(make_executable)


# Utility function to make a utility executable
#
# name - name of executable to produce
# srcs - list of src files

function(make_utility name srcs)
    make_executable(${name} "${srcs}" "${CSOUNDLIB}")
    add_dependencies(${name} ${CSOUNDLIB}) 
endfunction()


# Expands a list into a space-separated string (outvar element1 ....)
# Why do I have to do this? Cmake, you just lost one point
function(expand_list out)
    set(i 1)
    set(tmp "")
    while( ${i} LESS ${ARGC} )
        set(tmp "${tmp} ${ARGV${i}}")
        math(EXPR i "${i}+1")
    endwhile()
    set(${out} "${tmp}" PARENT_SCOPE)
endfunction(expand_list)

# Checks if dependencies for an enabled target are fulfilled.
# If FAIL_MISSING is true and the dependencies are not fulfilled,
# it will abort the cmake run.
# If FAIL_MISSING is false, it will set the option to OFF.
# If the target is not enabled, it will do nothing.
# example: check_deps(BUILD_NEW_PARSER FLEX_EXECUTABLE BISON_EXECUTABLE)
function(check_deps option)
    if(${option})
        set(i 1)
        while( ${i} LESS ${ARGC} )
            set(dep ${ARGV${i}})
            if(NOT ${dep})
                if(FAIL_MISSING)
                    message(FATAL_ERROR
                        "${option} is enabled, but ${dep}=\"${${dep}}\"")
                else()
                    message(STATUS "${dep}=\"${${dep}}\", so disabling ${option}")
                    set(${option} OFF PARENT_SCOPE)
                    # Set it in the local scope too
                    set(${option} OFF)
                endif()
            endif()
            math(EXPR i "${i}+1")
        endwhile()
    endif()
    if(${option})
        message(STATUS "${option} is enabled")
    else()
        message(STATUS "${option} is disabled")
    endif()
endfunction(check_deps)

if(WIN32 AND NOT MSVC)
    if(EXISTS "C:/MinGW/include")
        include_directories(C:/MinGW/include)
    else()
        MESSAGE(STATUS "MinGW include dir not found")
    endif()
endif()

if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX "")
    
    set(CSOUND_WINDOWS_LIBRARIES 
        advapi32 
        comctl32 
        comdlg32 
        glu32 
        kernel32 
        msvcrt
        odbc32 
        odbccp32 
        ole32 
        oleaut32 
        shell32             
        user32 
        uuid             
        winmm 
        winspool 
        ws2_32 
        wsock32 
        advapi32 
        comctl32 
        comdlg32 
        glu32 
        kernel32 
        odbc32 
        odbccp32 
        ole32 
        oleaut32 
        shell32             
        user32 
        uuid             
        winmm 
        winspool 
        ws2_32 
        wsock32
        pthread)
    
endif(WIN32)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX YES)
else()
    set(LINUX NO)
endif()

## USER OPTIONS ##

option(USE_DOUBLE "Set to use double-precision floating point for audio samples." ON)
option(BUILD_UTILITIES "Build stand-alone executables for utilities that can also be used with -U" ON)

option(BUILD_NEW_PARSER "Enable building new parser (requires Flex/Bison)" ON)
option(NEW_PARSER_DEBUG "Enable tracing of new parser" OFF)

option(BUILD_MULTI_CORE "Enable building for multicore system (requires BUILD_NEW_PARSER)" OFF)

option(FAIL_MISSING "Fail when a required external dependency is not present (useful for packagers)" OFF)

 # Optional targets, they should all default to ON (check_deps will disable them if not possible to build)


set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# This needs to be here since _everybody_ needs this flag
if(USE_DOUBLE) 
    add_definitions("-DUSE_DOUBLE")
endif(USE_DOUBLE)
if(${CMAKE_C_COMPILER} MATCHES "gcc" AND LINUX)
    add_definitions("-fvisibility=hidden")
endif()

#if(USE_DOUBLE)
#    set(BUILD_PLUGINS_DIR ${BUILD_DIR}/plugins64)
#else()
#    set(BUILD_PLUGINS_DIR ${BUILD_DIR}/plugins)
#endif()

set(BUILD_PLUGINS_DIR ${BUILD_DIR})
set(BUILD_BIN_DIR ${BUILD_DIR})
set(BUILD_LIB_DIR ${BUILD_DIR})

message(STATUS "BUILD_BIN_DIR set to ${BUILD_BIN_DIR}")
message(STATUS "BUILD_LIB_DIR set to ${BUILD_LIB_DIR}")
message(STATUS "BUILD_PLUGINS_DIR set to ${BUILD_PLUGINS_DIR}")

# OS specific checks

TEST_BIG_ENDIAN(BIG_ENDIAN)

## CONFIGURATION ##

SET(BUILD_SHARED_LIBS ON)

## HEADER/LIBRARY/OTHER CHECKS ##

# First, required stuff
find_library(LIBSNDFILE_LIBRARY sndfile)
if(NOT LIBSNDFILE_LIBRARY)
    message(FATAL_ERROR "Csound requires the sndfile library")
endif()

find_path(SNDFILE_H_PATH sndfile.h)
if(SNDFILE_H_PATH)
    include_directories(${SNDFILE_H_PATH})
else()
    message(FATAL_ERROR "Could not find sndfile.h") 
endif()

if(WIN32)   
    find_library(PTHREAD_LIBRARY pthreadGC2)
else()
    find_library(PTHREAD_LIBRARY pthread)
endif(WIN32)
if(NOT PTHREAD_LIBRARY)
    message(FATAL_ERROR "Csound requires the pthread library")
endif()

# Now, non required library searches #

find_library(VORBISFILE_LIBRARY vorbisfile)
find_file(USE_GETTEXT libintl.h)
find_library(LIBINTL_LIBRARY intl)

set(HEADERS_TO_CHECK 
    unistd.h io.h fcntl.h stdint.h
    sys/time.h sys/types.h termios.h
    values.h winsock.h sys/socket.h
    dirent.h)

foreach(header ${HEADERS_TO_CHECK})
    # Convert to uppercase and replace [./] with _
    string(TOUPPER ${header} tmp)
    string(REGEX REPLACE [./] "_" upper_header ${tmp})
    find_file(HAVE_${upper_header} ${header})
endforeach()

# Flex/Bison for the new parser
if(BUILD_NEW_PARSER)
    find_package(FLEX)
    find_package(BISON)
endif()

## MAIN TARGETS ##

set(libcsound_CFLAGS -D__BUILDING_LIBCSOUND)

include_directories(./H)

 
#adding this for files that #include SDIF/sdif*
include_directories(./)

#if(WIN32)
    include_directories(${LIBSNDFILE_INCLUDE_DIRECTORY})
#endif(WIN32)


# The csound library
set(libcsound_SRCS
    Engine/auxfd.c
    Engine/cfgvar.c
    Engine/entry1.c
    Engine/envvar.c
    Engine/express.c
    Engine/extract.c
    Engine/fgens.c
    Engine/insert.c
    Engine/linevent.c
    Engine/memalloc.c
    Engine/memfiles.c
    Engine/musmon.c
    Engine/namedins.c
    Engine/otran.c
    Engine/rdorch.c
    Engine/rdscor.c
    Engine/scsort.c
    Engine/scxtract.c
    Engine/sort.c
    Engine/sread.c
    Engine/swrite.c
    Engine/twarp.c
    InOut/libsnd.c
    InOut/libsnd_u.c
    InOut/midifile.c
    InOut/midirecv.c
    InOut/midisend.c
    InOut/winascii.c
    InOut/windin.c
    InOut/window.c
    InOut/winEPS.c
    OOps/aops.c
    OOps/bus.c
    OOps/cmath.c
    OOps/diskin.c
    OOps/diskin2.c
    OOps/disprep.c
    OOps/dumpf.c
    OOps/fftlib.c
    OOps/goto_ops.c
    OOps/midiinterop.c
    OOps/midiops.c
    OOps/midiout.c
    OOps/mxfft.c
    OOps/oscils.c
    OOps/pstream.c
    OOps/pvfileio.c
    OOps/pvsanal.c
    OOps/random.c
    OOps/remote.c
    OOps/schedule.c
    OOps/sndinfUG.c
    OOps/str_ops.c
    OOps/ugens1.c
    OOps/ugens2.c
    OOps/ugens3.c
    OOps/ugens4.c
    OOps/ugens5.c
    OOps/ugens6.c
    OOps/ugrw1.c
    OOps/ugrw2.c
    OOps/vdelay.c
    Top/argdecode.c
    Top/cscore_internal.c
    Top/cscorfns.c
    Top/csmodule.c
    Top/csound.c
    Top/getstring.c
    Top/main.c
    Top/new_opts.c
    Top/one_file.c
    Top/opcode.c
    Top/threads.c
    Top/utility.c)

# Handling New Parser

check_deps(BUILD_NEW_PARSER FLEX_EXECUTABLE BISON_EXECUTABLE)
if(BUILD_NEW_PARSER)
    add_custom_target(NewParser echo "Creating parser.c")

    set(YACC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.y)
    set(YACC_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.c)

    set(LEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.l)
    set(LEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c)

    add_custom_command(
        SOURCE ${LEX_SRC}
        COMMAND ${FLEX_EXECUTABLE} ARGS -Pcsound_orc -o${LEX_OUT} ${LEX_SRC}
        TARGET NewParser
        OUTPUTS ${LEX_OUT})

    add_custom_command(
        SOURCE ${YACC_SRC}
        COMMAND ${BISON_EXECUTABLE}
        ARGS -pcsound_orc -d --report=itemset -o ${YACC_OUT} ${YACC_SRC} 
        TARGET NewParser
        DEPENDS ${LEX_OUT}
        OUTPUTS ${YACC_OUT})
    
    list(APPEND libcsound_SRCS 
                ${LEX_OUT} ${YACC_OUT}
                Engine/csound_orc_semantics.c
                Engine/csound_orc_expressions.c
                Engine/csound_orc_optimize.c
                Engine/csound_orc_compile.c
                Engine/new_orc_parser.c
                Engine/symbtab.c)

    set_source_files_properties(${YACC_OUT} GENERATED)
    set_source_files_properties(${LEX_OUT} GENERATED)

    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    
    list(APPEND libcsound_CFLAGS -DENABLE_NEW_PARSER)
    
    if(NEW_PARSER_DEBUG)
        message(STATUS "Building with new parser debugging")
        list(APPEND libcsound_CFLAGS -DPARSER_DEBUG=1)
    else()
        message(STATUS "Not building with new parser debugging")
    endif()
    
    if(BUILD_MULTI_CORE)
        message(STATUS "Building with multicore")
        
        list(APPEND libcsound_SRCS
            Engine/cs_par_base.c
            Engine/cs_par_orc_semantic_analysis.c
            Engine/cs_par_dispatch.c
            Engine/new_orc_parser.c)
        
        list(APPEND libcsound_CFLAGS -DPARCS)
        
    else()
        message(STATUS "Not building with multicore")
    endif()
    
endif()


if(USE_DOUBLE)
    set(CSOUNDLIB "csound64")
else()
    set(CSOUNDLIB "csound")
endif()

add_library(${CSOUNDLIB} SHARED ${libcsound_SRCS})
set_target_properties(${CSOUNDLIB} PROPERTIES
    # Do not pull extra libs when linking against shared libcsound
    # The shared library loader will do that for us
    LINK_INTERFACE_LIBRARIES ""
    SOVERSION ${APIVERSION})

# Add the install target
install(TARGETS ${CSOUNDLIB}
    LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
    ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")

set(libcsound_LIBS 
    ${LIBSNDFILE_LIBRARY}  
    ${PTHREAD_LIBRARY})

if(WIN32) 
    list(APPEND libcsound_LIBS "${CSOUND_WINDOWS_LIBRARIES}")
endif()

# Linux does not have a separate libintl, it is part of libc
if(USE_GETTEXT AND (LIBINTL_LIBRARY OR LINUX))
    message(STATUS "Using GNU Gettext")
    if(NOT LINUX)
        list(APPEND libcsound_LIBS ${LIBINTL_LIBRARY})
    endif()
    list(APPEND libcsound_CFLAGS -DGNU_GETTEXT)
else()
    message(STATUS "Not using localization")
endif()


if(LINUX)
    message(STATUS "Building on Linux")
    add_definitions(-DLINUX)
    list(APPEND libcsound_LIBS m dl) 
endif()

if(APPLE)
    message(STATUS "Building on OSX")
    add_definitions(-DMACOSX -DPIPES)
    list(APPEND libcsound_LIBS m dl) 
endif()

if(WIN32)
    add_definitions(-DWIN32)
endif()
    


# Pass flags according to system capabilities

if(HAVE_WINSOCK_H OR HAVE_SYS_SOCKETS_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SOCKETS)
endif()
if(HAVE_DIRENT_H)
    list(APPEND libcsound_CFLAGS -DHAVE_DIRENT_H)
endif()
if(HAVE_FCNTL_H)
    list(APPEND libcsound_CFLAGS -DHAVE_FCNTL_H)
endif()
if(HAVE_UNISTD_H)
    list(APPEND libcsound_CFLAGS -DHAVE_UNISTD_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND libcsound_CFLAGS -DHAVE_STDINT_H)
endif()
if(HAVE_SYS_TIME_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SYS_TIME_H)
endif()
if(HAVE_SYS_TYPES_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_TERMIOS_H)
    list(APPEND libcsound_CFLAGS -DHAVE_TERMIOS_H)
endif()

expand_list(expanded_libcsound_CFLAGS ${libcsound_CFLAGS})
set_property(
    TARGET ${CSOUNDLIB}
    PROPERTY COMPILE_FLAGS "${expanded_libcsound_CFLAGS}")

target_link_libraries(${CSOUNDLIB} ${libcsound_LIBS})

set_target_properties(${CSOUNDLIB} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

add_subdirectory(Opcodes)
add_subdirectory(InOut)
add_subdirectory(interfaces)
add_subdirectory(frontends)
add_subdirectory(util)
add_subdirectory(util1)
add_subdirectory(SDIF)
