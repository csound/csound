option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" OFF)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
option(BUILD_DEPRECATED_OPCODES "Build deprecated opcodes" ON)

##########################################
##  External opcodes can be optionally built as plugins
###########################################
if(BUILD_PLUGINS)

 foreach(SRC IN LISTS cppops_SRCS hrtf_ops_SRCS
 sock_ops_SRCS externs_SRCS)
  get_filename_component(LIBNAME ${SRC} NAME_WE)
  get_filename_component(LIBSRC ${SRC} NAME)
  make_plugin(${LIBNAME} ${LIBSRC})
 endforeach()

if(UNIX)
 foreach(SRC IN LISTS unixops_SRCS)
  get_filename_component(LIBNAME ${SRC} NAME_WE)
  get_filename_component(LIBSRC ${SRC} NAME)
  make_plugin(${LIBNAME} ${LIBSRC})
   endforeach()
endif()
 
 foreach(SRC IN LISTS scanops_SRCS)
  get_filename_component(SSRC ${SRC} NAME)
  list(APPEND scanops ${SSRC})
 endforeach()
  make_plugin(scansyn "${scanops}")

 
 foreach(SRC IN LISTS mp3in_SRCS)
   string(CONCAT MSRC "../" ${SRC})
  list(APPEND mp3inops ${MSRC})
 endforeach()
  make_plugin(mp3in "${mp3inops}")  


endif() 





##########################################
##  Other plugin opcodes are in the following categories 
# 1) platform-specific opcodes
# 2) opcodes with external dependencies
# 3) deprecated opcodes
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
