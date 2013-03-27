cmake_minimum_required(VERSION 2.8.3)

project (Csound)

ENABLE_TESTING()

message(STATUS "${CMAKE_HOME_DIRECTORY}")

# Project definitions
set(APIVERSION "6.0")

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
set(LOCALE_INSTALL_DIR "share/locale")
set(HEADER_INSTALL_DIR "include/csound")

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CMakeParseArguments)

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

# Shortcut to add compiler flags
# Mandatory arguments: the flags to add, should be passed before optional keywords, can be
#                      passed as lists or space-separated
# Optional keywords:
# LINKER: add to linker instead of compile flags (LDFLAGS vs CFLAGS)
# TARGETS <target1> <target2> ...
#           if specified, will add the flags to a target instead
#           of the global namespace
function(add_compiler_flags)
    set(bool_options LINKER)
    set(multi_val_args TARGETS)
    cmake_parse_arguments(FLAGS "${bool_options}" "" "${multi_val_args}" ${ARGN})

    expand_list(FLAGS ${FLAGS_UNPARSED_ARGUMENTS})

    if(FLAGS_TARGETS)
        foreach(target ${FLAGS_TARGETS})
            if(NOT FLAGS_LINKER)
                set(property "COMPILE_FLAGS")
            else()
                set(property "LINK_FLAGS")
            endif()
            get_target_property(propval ${target} ${property})
            if(NOT propval)
                set(propval "")
            endif()
            set_target_properties(${target} PROPERTIES
                ${property} "${propval} ${FLAGS}")
        endforeach()
    else()
        if(FLAGS_LINKER)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAGS}")
            set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAGS}")
        else()
            add_definitions("${FLAGS}")
        endif()
    endif()
endfunction(add_compiler_flags)


# Include this after the install path definitions so we can override them here.
# Also after function definitions so we can use them there
find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_HOME_DIRECTORY})
if(CUSTOM_CMAKE)
    message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
    include(${CUSTOM_CMAKE})
endif()


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

option(NEW_PARSER_DEBUG "Enable tracing of new parser" OFF)

option(BUILD_MULTI_CORE "Enable building for multicore system (requires BUILD_NEW_PARSER)" ON)

option(FAIL_MISSING "Fail when a required external dependency is not present (useful for packagers)" OFF)

option(USE_GETTEXT "Use the Gettext internationalization library" ON)

option(BUILD_STATIC_LIBRARY "Also build a static version of the csound library" OFF)

option(USE_OPEN_MP "Use OpenMP for Parallel Performance" ON)
option(USE_LRINT "Use lrint/lrintf for converting floating point values to integers." ON)
option(BUILD_RELEASE "Build for release" ON)
# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)


set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# This needs to be here since _everybody_ needs this flag
if(USE_DOUBLE) 
    add_definitions("-DUSE_DOUBLE")
endif(USE_DOUBLE)
if(${CMAKE_C_COMPILER} MATCHES "gcc" AND LINUX)
    add_compiler_flags("-fvisibility=hidden")
    add_compiler_flags("-fvisibility=hidden" LINKER)
endif()


if(BUILD_RELEASE)
    add_definitions("-D_CSOUND_RELEASE_")
    if(LINUX)
        set(DEFAULT_OPCODEDIR "${CMAKE_INSTALL_PREFIX}/${PLUGIN_INSTALL_DIR}")
        add_definitions("-DCS_DEFAULT_PLUGINDIR=\"${DEFAULT_OPCODEDIR}\"")
    elseif(APPLE)
        if(USE_DOUBLE)
            set(DEFAULT_OPCODEDIR "/Library/Frameworks/CsoundLib64.framework/Versions/${APIVERSION}/Resources/Opcodes64")
        else()
            set(DEFAULT_OPCODEDIR "/Library/Frameworks/CsoundLib.framework/Versions/${APIVERSION}/Resources/Opcodes")
        endif() 
        add_definitions(-DCS_DEFAULT_PLUGINDIR=\"${DEFAULT_OPCODEDIR}\")
    endif()
else()
    add_definitions("-DBETA")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX) 
    add_definitions("-Wno-format")
    add_compiler_flags("-g")
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

find_package(OpenMP)

# First, required stuff
find_library(LIBSNDFILE_LIBRARY sndfile)

if(NOT LIBSNDFILE_LIBRARY AND WIN32)
find_library(LIBSNDFILE_LIBRARY sndfile-1)
endif()

if(NOT LIBSNDFILE_LIBRARY)
    message(FATAL_ERROR "Csound requires the sndfile library")
endif()

find_path(SNDFILE_H_PATH sndfile.h)
if(SNDFILE_H_PATH)
    include_directories(${SNDFILE_H_PATH})
else()
    message(FATAL_ERROR "Could not find sndfile.h") 
endif()

find_library(PTHREAD_LIBRARY pthread)

if(NOT PTHREAD_LIBRARY AND WIN32)   
    find_library(PTHREAD_LIBRARY pthreadGC2)
endif()

if(NOT PTHREAD_LIBRARY)
    message(FATAL_ERROR "Csound requires the pthread library")
endif()

set(CMAKE_REQUIRED_INCLUDES pthread.h)
set(CMAKE_REQUIRED_LIBRARIES pthread)

# Now, non required library searches #

find_library(VORBISFILE_LIBRARY vorbisfile)
check_include_file(libintl.h LIBINTL_HEADER)
check_include_file_cxx(eigen3/Eigen/Dense EIGEN3_HEADER)
find_library(LIBINTL_LIBRARY intl)
find_package(Gettext)
check_library_exists(m lrint "" HAVE_LRINT)

set(HEADERS_TO_CHECK
    unistd.h io.h fcntl.h stdint.h
    sys/time.h sys/types.h termios.h
    values.h winsock.h sys/socket.h
    dirent.h )

foreach(header ${HEADERS_TO_CHECK})
    # Convert to uppercase and replace [./] with _
    string(TOUPPER ${header} tmp)

    string(REGEX REPLACE [./] "_" upper_header ${tmp})
    check_include_file(${header} HAVE_${upper_header})
endforeach()

check_deps(USE_LRINT HAVE_LRINT)
if(USE_LRINT)
    add_definitions("-DUSE_LRINT")
endif()

# Flex/Bison for the new parser
find_package(FLEX)
find_package(BISON)

if(NOT FLEX_EXECUTABLE)
    message(FATAL_ERROR "Csound requires the flex executable")
endif()

if(NOT BISON_EXECUTABLE)
    message(FATAL_ERROR "Csound requires the bison executable")
endif()

## MAIN TARGETS ##

set(libcsound_CFLAGS -D__BUILDING_LIBCSOUND)

include_directories(./H)
include_directories(./include)
include_directories(./Engine)
 
#adding this for files that #include SDIF/sdif*
include_directories(./)

#checking pthread functions
check_function_exists(pthread_spin_lock PTHREAD_SPIN_LOCK_EXISTS)
check_function_exists(pthread_barrier_init PTHREAD_BARRIER_INIT_EXISTS)

if(PTHREAD_SPIN_LOCK_EXISTS)
   list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_SPIN_LOCK)
endif()

if(PTHREAD_BARRIER_INIT_EXISTS)
   list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_BARRIER_INIT)
endif()


check_deps(USE_OPEN_MP OPENMP_FOUND PTHREAD_BARRIER_INIT_EXISTS) 
if(USE_OPEN_MP)
   list(APPEND libcsound_CFLAGS -DUSE_OPENMP)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

   set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")
   set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")

endif()


#if(WIN32)
    include_directories(${LIBSNDFILE_INCLUDE_DIRECTORY})
#endif(WIN32)


# The csound library
set(libcsound_SRCS
    Engine/auxfd.c
    Engine/cfgvar.c
    Engine/corfiles.c
    Engine/entry1.c
    Engine/envvar.c
    Engine/extract.c
    Engine/fgens.c
    Engine/insert.c
    Engine/linevent.c
    Engine/memalloc.c
    Engine/memfiles.c
    Engine/musmon.c
    Engine/namedins.c
    Engine/rdscor.c
    Engine/scsort.c
    Engine/scxtract.c
    Engine/sort.c
    Engine/sread.c
    Engine/swritestr.c
    Engine/twarp.c
    Engine/csound_type_system.c
    Engine/csound_standard_types.c
    Engine/pools.c
    InOut/libsnd.c
    InOut/libsnd_u.c
    InOut/midifile.c
    InOut/midirecv.c
    InOut/midisend.c
    InOut/winascii.c
    InOut/windin.c
    InOut/window.c
    InOut/winEPS.c
    InOut/circularbuffer.c
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
    OOps/ugens2a.c
    OOps/ugens3.c
    OOps/ugens4.c
    OOps/ugens5.c
    OOps/ugens6.c
    OOps/ugtabs.c
    OOps/ugrw1.c
    OOps/ugrw2.c
    OOps/vdelay.c
    OOps/compile_ops.c
    Opcodes/babo.c
    Opcodes/bilbar.c
    Opcodes/compress.c
    Opcodes/eqfil.c
    Opcodes/Vosim.c
    Opcodes/pitch.c 
    Opcodes/pitch0.c  
    Opcodes/spectra.c 
    Opcodes/ambicode1.c
    Opcodes/sfont.c 
    Opcodes/grain4.c 
    Opcodes/hrtferX.c 
    Opcodes/loscilx.c
    Opcodes/minmax.c 
    Opcodes/pan2.c 
    Opcodes/arrays.c
    Opcodes/tabvars.c 
    Opcodes/phisem.c
    Opcodes/hrtfopcodes.c 
    Opcodes/stackops.c 
    Opcodes/vbap.c 
    Opcodes/vbap1.c 
    Opcodes/vbap_n.c 
    Opcodes/vbap_zak.c  
    Opcodes/vaops.c 
    Opcodes/ugakbari.c 
    Opcodes/harmon.c 
    Opcodes/pitchtrack.c 
    Opcodes/partikkel.c 
    Opcodes/shape.c 
    Opcodes/tabsum.c
    Opcodes/crossfm.c 
    Opcodes/pvlock.c 
    Opcodes/fareyseq.c 
    Opcodes/modmatrix.c 
    Opcodes/scoreline.c 
    Opcodes/modal4.c
    Opcodes/physutil.c 
    Opcodes/physmod.c 
    Opcodes/mandolin.c 
    Opcodes/singwave.c
    Opcodes/fm4op.c 
    Opcodes/moog1.c 
    Opcodes/shaker.c 
    Opcodes/bowedbar.c
    Opcodes/gab/tabmorph.c
    Opcodes/gab/hvs.c
    Opcodes/gab/sliderTable.c
    Opcodes/gab/newgabopc.c
    Opcodes/ftest.c
    Opcodes/hrtfearly.c
    Opcodes/hrtfreverb.c
    Opcodes/cpumeter.c
    Opcodes/gendy.c
    Opcodes/tl/sc_noise.c
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
    Top/utility.c
    Top/threadsafe.c)

set(stdopcod_SRCS 
    Opcodes/ambicode.c      
    Opcodes/bbcut.c         
    Opcodes/biquad.c
    Opcodes/butter.c        
    Opcodes/clfilt.c        
    Opcodes/cross2.c
    Opcodes/dam.c           
    Opcodes/dcblockr.c      
    Opcodes/filter.c
    Opcodes/flanger.c       
    Opcodes/follow.c        
    Opcodes/fout.c
    Opcodes/freeverb.c      
    Opcodes/ftconv.c        
    Opcodes/ftgen.c
    Opcodes/gab/gab.c       
    Opcodes/gab/vectorial.c 
    Opcodes/grain.c
    Opcodes/locsig.c        
    Opcodes/lowpassr.c      
    Opcodes/metro.c
    Opcodes/midiops2.c      
    Opcodes/midiops3.c      
    Opcodes/newfils.c
    Opcodes/nlfilt.c        
    Opcodes/oscbnk.c        
    Opcodes/pluck.c
    Opcodes/repluck.c       
    Opcodes/reverbsc.c      
    Opcodes/seqtime.c
    Opcodes/sndloop.c       
    Opcodes/sndwarp.c       
    Opcodes/space.c
    Opcodes/spat3d.c        
    Opcodes/syncgrain.c     
    Opcodes/ugens7.c
    Opcodes/ugens9.c        
    Opcodes/ugensa.c        
    Opcodes/uggab.c
    Opcodes/ugmoss.c        
    Opcodes/ugnorman.c      
    Opcodes/ugsc.c
    Opcodes/wave-terrain.c  
    Opcodes/stdopcod.c
    Opcodes/socksend.c
    Opcodes/sockrecv.c)

set(cs_pvs_ops_SRCS
    Opcodes/ifd.c 
    Opcodes/partials.c 
    Opcodes/psynth.c 
    Opcodes/pvsbasic.c
    Opcodes/pvscent.c 
    Opcodes/pvsdemix.c 
    Opcodes/pvs_ops.c 
    Opcodes/pvsband.c
    Opcodes/pvsbuffer.c
    Opcodes/pvsgendy.c)

set(oldpvoc_SRCS
    Opcodes/dsputil.c 
    Opcodes/pvadd.c 
    Opcodes/pvinterp.c 
    Opcodes/pvocext.c
    Opcodes/pvread.c 
    Opcodes/ugens8.c 
    Opcodes/vpvoc.c 
    Opcodes/pvoc.c)

set(mp3in_SRCS
    Opcodes/mp3in.c
    InOut/libmpadec/layer1.c
    InOut/libmpadec/layer2.c
    InOut/libmpadec/layer3.c
    InOut/libmpadec/synth.c
    InOut/libmpadec/tables.c
    InOut/libmpadec/mpadec.c
    InOut/libmpadec/mp3dec.c)


list(APPEND libcsound_SRCS ${stdopcod_SRCS} ${cs_pvs_ops_SRCS} ${oldpvoc_SRCS} ${mp3in_SRCS})

# Handling New Parser

add_custom_target(NewParser echo "Creating parser.c")

set(YACC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.y)
set(YACC_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.c)

set(LEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.l)
set(LEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c)

set(PRELEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_pre.lex)
set(PRELEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_prelex.c)

add_custom_command(
    SOURCE ${LEX_SRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B -t ${LEX_SRC} > ${LEX_OUT} 
    TARGET NewParser
    OUTPUTS ${LEX_OUT})

add_custom_command(
    SOURCE ${PRELEX_SRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B ${PRELEX_SRC} > ${PRELEX_OUT} 
    TARGET NewParser
    OUTPUTS ${PRELEX_OUT})

add_custom_command(
    SOURCE ${YACC_SRC}
    COMMAND ${BISON_EXECUTABLE}
    ARGS -pcsound_orc -d --report=itemset -o ${YACC_OUT} ${YACC_SRC} 
    TARGET NewParser
    DEPENDS ${LEX_OUT}
    OUTPUTS ${YACC_OUT})

list(APPEND libcsound_SRCS 
    ${LEX_OUT} ${YACC_OUT} ${PRELEX_OUT}
    Engine/csound_orc_semantics.c
    Engine/csound_orc_expressions.c
    Engine/csound_orc_optimize.c
    Engine/csound_orc_compile.c
    Engine/new_orc_parser.c
    Engine/symbtab.c)

set_source_files_properties(${YACC_OUT} GENERATED)
set_source_files_properties(${LEX_OUT} GENERATED)

# Handling New Score Parser

add_custom_target(NewScoParser echo "Creating sco_parser.c")
set(NewScoParser OFF)

set(YACC_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_sco.y)
set(YACC_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_scoparse.c)

set(LEX_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_sco.lex)
set(LEX_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_scolex.c)

set(PRELEX_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_prs.lex)
set(PRELEX_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_prslex.c)

add_custom_command(
    SOURCE ${LEX_SCOSRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B -t ${LEX_SCOSRC} > ${LEX_SCOOUT} 
    TARGET NewScoParser
    OUTPUTS ${LEX_SCOOUT})

add_custom_command(
    SOURCE ${PRELEX_SCOSRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B ${PRELEX_SCOSRC} > ${PRELEX_SCOOUT} 
    TARGET NewScoParser
    OUTPUTS ${PRELEXSCO_OUT})

add_custom_command(
    SOURCE ${YACC_SCOSRC}
    COMMAND ${BISON_EXECUTABLE}
    ARGS -pcsound_sco -d --report=itemset -o ${YACC_SCOOUT} ${YACC_SCOSRC} 
    TARGET NewParser
    DEPENDS ${LEX_SCOOUT}
    OUTPUTS ${YACC_SCOOUT})

list(APPEND libcsound_SRCS 
    ${LEX_SCOOUT} ${YACC_SCOOUT} ${PRELEX_SCOOUT})

set_source_files_properties(${YACC_SCOOUT} GENERATED)
set_source_files_properties(${LEX_SCOOUT} GENERATED)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
    
if(NEW_PARSER_DEBUG)
    message(STATUS "Building with new parser debugging")
    list(APPEND libcsound_CFLAGS -DPARSER_DEBUG=1)
else()
    message(STATUS "Not building with new parser debugging")
endif()

if(BUILD_MULTI_CORE)
    message(STATUS "Building with multicore")
    
    list(APPEND libcsound_SRCS
        Engine/cs_new_dispatch.c
        Engine/cs_par_base.c
        Engine/cs_par_orc_semantic_analysis.c
        Engine/cs_par_dispatch.c)
    
    list(APPEND libcsound_CFLAGS -DPARCS)
    
else()
    message(STATUS "Not building with multicore")
endif()
    


if(USE_DOUBLE)
    if(APPLE)
        set(CSOUNDLIB "CsoundLib64")
    else()
        set(CSOUNDLIB "csound64")
    endif()
else()
    if(APPLE)
        set(CSOUNDLIB "CsoundLib")
    else()
        set(CSOUNDLIB "csound")
    endif()
endif()

set(CSOUNDLIB_STATIC "${CSOUNDLIB}-static")

# ADDING HERE TO GRAB LIST OF HEADERS IN CASE OF BUILDING OSX FRAMEWORK
add_subdirectory(include)

if(APPLE)
    string(REGEX REPLACE "([^;]+)(;|$)" "include/\\1\\2" csheaders "${csheaders}")
    set(libcsound_SRCS "${libcsound_SRCS};${csheaders}")
endif()

add_library(${CSOUNDLIB} SHARED ${libcsound_SRCS})
set_target_properties(${CSOUNDLIB} PROPERTIES
    # Do not pull extra libs when linking against shared libcsound
    # The shared library loader will do that for us
    LINK_INTERFACE_LIBRARIES ""
    SOVERSION ${APIVERSION})

if(APPLE)
    set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK YES)
    set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK_VERSION "${APIVERSION}")
    set_target_properties(${CSOUNDLIB} PROPERTIES PUBLIC_HEADER "${csheaders}")
endif()

# Add the install target
install(TARGETS ${CSOUNDLIB}
    LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
    ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
    FRAMEWORK DESTINATION "~/Library/Frameworks")

set(libcsound_LIBS 
    ${LIBSNDFILE_LIBRARY}  
    ${PTHREAD_LIBRARY})

if(WIN32) 
    list(APPEND libcsound_LIBS "${CSOUND_WINDOWS_LIBRARIES}")
endif()

# Linux does not have a separate libintl, it is part of libc
set(LIBINTL_AVAIL (LIBINTL_LIBRARY OR LINUX))
check_deps(USE_GETTEXT LIBINTL_HEADER LIBINTL_AVAIL GETTEXT_MSGFMT_EXECUTABLE)
if(USE_GETTEXT)
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
    add_definitions(-DLINUX -DPIPES)
    list(APPEND libcsound_LIBS m dl) 
endif()

if(APPLE)
    message(STATUS "Building on OSX")
    add_definitions(-DMACOSX -DPIPES -DNO_FLTK_THREADS -DHAVE_SOCKETS)
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
if(HAVE_VALUES_H)
    list(APPEND libcsound_CFLAGS -DHAVE_VALUES_H)
endif()
#if(CMAKE_C_COMPILER MATCHES "gcc")
#    list(APPEND libcsound_CFLAGS -fno-strict-aliasing)
#endif()

if(BIG_ENDIAN)
    list(APPEND libcsound_CFLAGS -DWORDS_BIGENDIAN)
endif()


# VL 13.02.2013: I have added this code to make the PUBLIC attribute mean
# something on gcc (OSX, LINUX), since we are using it.
# see http://gcc.gnu.org/wiki/Visibility

if(NOT WIN32)
if(CMAKE_C_COMPILER MATCHES "gcc")
    add_compiler_flags(${libcsound_CFLAGS} -fvisibility=hidden
    -ffast-math -mfpmath=sse -fomit-frame-pointer TARGETS ${CSOUNDLIB})
else()
   add_compiler_flags(${libcsound_CFLAGS} TARGETS ${CSOUNDLIB})
endif()
else()
  add_compiler_flags(${libcsound_CFLAGS} TARGETS ${CSOUNDLIB})
endif()

target_link_libraries(${CSOUNDLIB} ${libcsound_LIBS})

set_target_properties(${CSOUNDLIB} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

if(BUILD_STATIC_LIBRARY)
    add_library(${CSOUNDLIB_STATIC} STATIC ${libcsound_SRCS})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES OUTPUT_NAME ${CSOUNDLIB})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES PREFIX "lib")
    add_compiler_flags(${libcsound_CFLAGS} TARGETS ${CSOUNDLIB_STATIC})

    target_link_libraries(${CSOUNDLIB_STATIC} ${libcsound_LIBS})

    set_target_properties(${CSOUNDLIB_STATIC} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

    # Add the install target
    install(TARGETS ${CSOUNDLIB_STATIC}
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")
endif()

## option(BUILD_CATALOG "Build the opcode/library catalog" OFF)
## check_deps(BUILD_CATALOG)
## if(BUILD_CATALOG)
## make_executable(mkdb "mkdb" "dl")
## set_target_properties(mkdb PROPERTIES LINKER_LANGUAGE C)
## endif()


add_subdirectory(Opcodes)
add_subdirectory(InOut)
add_subdirectory(interfaces)
add_subdirectory(frontends)
add_subdirectory(util)
add_subdirectory(util1)
add_subdirectory(SDIF)
add_subdirectory(po)
add_subdirectory(tests/c)
add_subdirectory(tests/commandline)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# target etags/tags
if(UNIX)
  add_custom_target(tags COMMAND etags `find ${CMAKE_CURRENT_SOURCE_DIR} -name \\*.cc -or -name \\*.hh -or -name \\*.cpp -or -name \\*.h -or -name \\*.c | grep -v " "`)
  add_custom_target(etags DEPENDS tags)
endif()

