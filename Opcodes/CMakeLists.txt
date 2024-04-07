option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" OFF)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
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
#  See instructions in ../Top/csmodule.c
##########################################

## platform-dependent opcodes ##
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
