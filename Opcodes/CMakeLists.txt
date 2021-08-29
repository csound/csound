option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" ON)
option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)
option(BUILD_JACK_OPCODES "Build the JACK related opcodes" ON)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
option(BUILD_PADSYNTH_OPCODES "Build the Padsynth opcode" ON)
option(BUILD_SCANSYN_OPCODES "Build the scansyn opcodes" ON)
option(BUILD_DEPRECATED_OPCODES "Build deprecated opcodes" ON)

##########################################
## Plugin opcodes are in the following categories ##
# 1) opcodes in C++ with no external dependencies
# 2) opcodes with special licensing conditions
# 3) platform-specific opcodes
# 4) opcodes with external dependencies
# 5) deprecated opcodes
#
#  if an opcode is not in one of the categories above,
#  it should be added with BUILTIN linkage
#  (i.e. not as plugins)
##########################################

message(STATUS "## Building Plugin Opcodes ##")
## CPP OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##
make_plugin(doppler doppler.cpp)
make_plugin(fractalnoise tl/fractalnoise.cpp)
make_plugin(ftsamplebank ftsamplebank.cpp)
make_plugin(lfsr lfsr.cpp)
make_plugin(bformdec2 bformdec2.cpp)
make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)
make_plugin(ampmidid ampmidid.cpp)

if(APPLE)
   make_plugin(arrayops arrayops.cpp)
   make_plugin(pvsops pvsops.cpp)
   make_plugin(trigenvsegs trigEnvSegs.cpp)
elseif(LINUX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
     if(COMPILER_SUPPORTS_CXX11)
       make_plugin(arrayops arrayops.cpp)
       set_target_properties(arrayops PROPERTIES COMPILE_FLAGS "-std=gnu++11")
       make_plugin(pvsops pvsops.cpp)
      set_target_properties(pvsops PROPERTIES COMPILE_FLAGS "-std=gnu++11")
       make_plugin(trigenvsegs trigEnvSegs.cpp)
      set_target_properties(trigenvsegs PROPERTIES COMPILE_FLAGS "-std=gnu++11")
     else()
        message(STATUS "Not building array operators as no C++11 support found.")
     endif()
else()
    make_plugin(arrayops arrayops.cpp)
    make_plugin(pvsops pvsops.cpp)
    make_plugin(trigenvsegs trigEnvSegs.cpp)
    if(WIN32 AND NOT MSVC)
      set_target_properties(arrayops PROPERTIES COMPILE_FLAGS
      "-std=gnu++11")
      set_target_properties(pvsops PROPERTIES COMPILE_FLAGS "-std=gnu++11")
      set_target_properties(trigenvsegs PROPERTIES COMPILE_FLAGS "-std=gnu++11")
      endif()
endif()

if(BUILD_PADSYNTH_OPCODES)
if(APPLE)
  make_plugin(padsynth padsynth_gen.cpp)
  set_target_properties(padsynth PROPERTIES COMPILE_FLAGS "-std=gnu++11 -stdlib=libc++"
    LINK_FLAGS "-std=gnu++11 -stdlib=libc++")
elseif(LINUX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=gnu++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        make_plugin(padsynth padsynth_gen.cpp)
        set_target_properties(padsynth PROPERTIES COMPILE_FLAGS "-std=gnu++11")
    elseif(COMPILER_SUPPORTS_CXX0X)
        make_plugin(padsynth padsynth_gen.cpp)
        set_target_properties(padsynth PROPERTIES COMPILE_FLAGS "-std=gnu++0x")
    else()
        message(STATUS "Not building padsynth as no C++11 support found.")
    endif()
else()
    make_plugin(padsynth padsynth_gen.cpp)

    if(WIN32 AND NOT MSVC)
      set_target_properties(padsynth PROPERTIES COMPILE_FLAGS "-std=gnu++11"
        LINK_FLAGS "-std=gnu++11")
    endif()
endif()
endif()

## opcodes with special licence conditions ##
if(BUILD_SCANSYN_OPCODES)
    set(scansyn_SRCS
        scansyn.c scansynx.c)
    make_plugin(scansyn "${scansyn_SRCS}" ${MATH_LIBRARY})
endif()

## platform-dependent opcodes ##
if(UNIX)
    make_plugin(control control.c)
    make_plugin(urandom urandom.c)
endif()

if(LINUX)
    make_plugin(joystick linuxjoystick.c)
endif()

check_deps(BUILD_DSSI_OPCODES LINUX)
if(BUILD_DSSI_OPCODES)
    check_include_file(ladspa.h LADSPA_H)
    set(dssi_SRC dssi4cs/src/load.c dssi4cs/src/dssi4cs.c)
    make_plugin(dssi4cs "${dssi_SRC}" dl)
endif()

## opcodes contributed by third-party ##
add_subdirectory(emugens)

## OPCODES WITH EXTERNAL DEPENDENCIES ##
include(CheckIncludeFiles)

find_package(FLUIDSYNTH)
if(BUILD_FLUID_OPCODES AND FLUIDSYNTH_FOUND)
    make_plugin(fluidOpcodes fluidOpcodes/fluidOpcodes.cpp)
    target_include_directories(fluidOpcodes PRIVATE ${FLUIDSYNTH_INCLUDE_DIR})
    target_link_libraries(fluidOpcodes ${FLUIDSYNTH_LIBRARIES})
endif()

find_library(JACK_LIBRARY jack)
find_library(JACKDMP_LIBRARY jackdmp)
check_include_file(jack/jack.h JACK_HEADER)
set(JACK_LIB ${JACK_LIBRARY})
if(NOT JACK_LIB)
  set(JACK_LIB ${JACKDMP_LIBRARY})
endif()

check_deps(BUILD_JACK_OPCODES JACK_HEADER JACK_LIB)
if(BUILD_JACK_OPCODES)
    set(jack_LIBS ${PTHREAD_LIBRARY})
    if(JACKDMP_LIBRARY)
        list(APPEND jack_LIBS ${JACKDMP_LIBRARY})
    else()
        list(APPEND jack_LIBS ${JACK_LIBRARY})
    endif()
    make_plugin(jacko jacko.cpp "${jack_LIBS}")
    set_target_properties(jacko PROPERTIES COMPILE_FLAGS
   "-std=c++11")
    make_plugin(jackTransport jackTransport.c "${jack_LIBS}")
endif()

find_package(LIBLO)
if(BUILD_OSC_OPCODES AND LIBLO_FOUND)
    make_plugin(osc OSC.c)
    if(WIN32)
      target_link_libraries(osc ${LIBLO_LIBRARIES})
	  # FIXME how to build a static version of this?
      if(BUILD_STATIC_LIBRARY AND NOT MSVC)
        add_library(pthread_static STATIC IMPORTED)
        set_target_properties(pthread_static PROPERTIES IMPORTED_LOCATION ${PTHREAD_LIBRARY})
        target_link_libraries(osc pthread_static)
      elseif(NOT MSVC)
        target_link_libraries(osc ${PTHREAD_LIBRARY})
      endif()
      target_link_libraries(osc wsock32 ws2_32 iphlpapi)
    elseif(HAIKU)
      target_link_libraries(osc ${LIBLO_LIBRARIES})
    else()
      target_link_libraries(osc ${LIBLO_LIBRARIES} pthread)
    endif()
endif()

## deprecated opcodes ##
if(BUILD_DEPRECATED_OPCODES)
    make_plugin(deprecated deprecated.c)
endif()
