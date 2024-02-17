option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" OFF)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
option(BUILD_PADSYNTH_OPCODES "Build the Padsynth opcode" ON)
option(BUILD_SCANSYN_OPCODES "Build the scansyn opcodes" ON)
option(BUILD_DEPRECATED_OPCODES "Build deprecated opcodes" ON)
option(BUILD_CPP_OPCODES "Build CPP plugins" ON)

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
#  See instructions in ../Top/csmodule.c
##########################################

if(BUILD_CPP_OPCODES)
message(STATUS "## Building Plugin Opcodes ##")
## CPP OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##
make_plugin(doppler doppler.cpp)
make_plugin(fractalnoise tl/fractalnoise.cpp)

if(DIRENT_H) 
make_plugin(ftsamplebank ftsamplebank.cpp)
get_filename_component(DIRENT_INCLUDE_DIR ${DIRENT_H} DIRECTORY)
target_include_directories(ftsamplebank PRIVATE ${DIRENT_INCLUDE_DIR})
endif()

make_plugin(lfsr lfsr.cpp)
make_plugin(bformdec2 bformdec2.cpp)
make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)
make_plugin(ampmidid ampmidid.cpp)

make_plugin(arrayops arrayops.cpp)
target_compile_features(arrayops PRIVATE cxx_std_11)

make_plugin(pvsops pvsops.cpp)
target_compile_features(pvsops PRIVATE cxx_std_11)

make_plugin(trigenvsegs trigEnvSegs.cpp)
target_compile_features(trigenvsegs PRIVATE cxx_std_11)

make_plugin(padsynth padsynth_gen.cpp)
target_compile_features(padsynth PRIVATE cxx_std_11)

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

# config file shipped by vcpkg isn't working, so use module
find_package(LibLO MODULE)
check_deps(BUILD_OSC_OPCODES LibLO_FOUND)
if(BUILD_OSC_OPCODES)
    make_plugin(osc OSC.c)
    target_link_libraries(osc PRIVATE LibLO::liblo Threads::Threads)
    if(WIN32)
      target_link_libraries(osc PRIVATE wsock32 ws2_32 iphlpapi)
    endif()
endif()

## deprecated opcodes ##
if(BUILD_DEPRECATED_OPCODES)
    make_plugin(deprecated deprecated.c)
endif()
