option(BUILD_CHUA_OPCODES "Build the chua opcodes (requires boost)" ON)
option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" ON)
option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)
option(BUILD_JACK_OPCODES "Build the JACK related opcodes" ON)
option(BUILD_LINEAR_ALGEBRA_OPCODES "Build the Linear Algebra Opcodes" ON)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)
option(BUILD_PYTHON_OPCODES "Build the Python opcodes" ON)
option(BUILD_LUA_OPCODES "Build the Lua opcodes" ON)
option(BUILD_WIIMOTE_OPCODES "Build the Wiimote opcodes" ON) 
option(BUILD_P5GLOVE_OPCODES "Build the P5Glove opcodes" ON) 
option(BUILD_SERIAL_OPCODES "Build the serial opcode" ON)
option(BUILD_PLATEREV_OPCODES "Build the platerev opcode" ON)

find_package(Boost)
check_include_file(ladspa.h LADSPA_H)
check_include_file(fluidsynth.h FLUIDSYNTH_H)
find_library(FLUIDSYNTH_LIBRARY fluidsynth)
find_library(JACK_LIBRARY jack)
find_library(JACKDMP_LIBRARY jackdmp)
check_include_file(jack/jack.h JACK_HEADER)
check_include_file_cxx(gmm/gmm.h GMM_HEADER)
check_include_file(lo/lo.h OSC_HEADER)
find_library(LIBLO_LIBRARY lo)
find_package(PNG)
find_package(PythonLibs)
#find_library(LUA_LIBRARIES luajit)
find_package(Lua51)
check_include_file(wiiuse.h WIIUSE_H)
find_library(WIIUSE_LIB wiiuse)
check_include_file(p5glove.h P5GLOVE_H)
find_library(P5GLOVE_LIB p5glove)

## OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##

make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)

make_plugin(ampmidid ampmidid.cpp)
make_plugin(cs_date date.c)
make_plugin(system_call system_call.c)
make_plugin(doppler doppler.cpp)
make_plugin(fractalnoise tl/fractalnoise.cpp)

make_plugin(fareygen fareygen.c)

make_plugin(cellular cellular.c)

if(BUILD_SERIAL_OPCODES)
   make_plugin(serial serial.c)
endif()

if(BUILD_PLATEREV_OPCODES)
   make_plugin(platerev platerev.c)
endif()

# temporarily do not build on Windows until these opcodes are fixed
# to compile
# these are not plugins anymore
# if(NOT WIN32) 
#    make_plugin(udprecv sockrecv.c)
#    make_plugin(udpsend socksend.c)
# endif()

set(scansyn_SRCS
    scansyn.c scansynx.c)
make_plugin(scansyn "${scansyn_SRCS}" m)

if(UNIX)
    make_plugin(control control.c)
endif()

if(LINUX)
    make_plugin(urandom urandom.c)
    make_plugin(joystick linuxjoystick.c)
endif()

## OPCODES WITH EXTERNAL DEPENDENCIES ##

check_deps(BUILD_CHUA_OPCODES Boost_FOUND)
if(BUILD_CHUA_OPCODES)
    include_directories(${Boost_INCLUDE_DIRS})
    make_plugin(chua chua/ChuaOscillator.cpp)
endif()

check_deps(BUILD_DSSI_OPCODES LADSPA_H)
if(BUILD_DSSI_OPCODES)
    set(dssi_SRC dssi4cs/src/load.c dssi4cs/src/dssi4cs.c)
    make_plugin(dssi4cs "${dssi_SRC}" dl)
endif()

check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_H)
if(BUILD_FLUID_OPCODES)
    make_plugin(fluidOpcodes fluidOpcodes/fluidOpcodes.cpp ${FLUIDSYNTH_LIBRARY})
endif()

set(JACK_LIB (JACK_LIBRARY OR JACKDMP_LIBRARY))
check_deps(BUILD_JACK_OPCODES JACK_HEADER JACK_LIB)
if(BUILD_JACK_OPCODES)
    set(jack_LIBS ${PTHREAD_LIBRARY})
    if(JACKDMP_LIBRARY)
        list(APPEND jack_LIBS ${JACKDMP_LIBRARY})
    else()
        list(APPEND jack_LIBS ${JACK_LIBRARY})
    endif()
    make_plugin(jacko jacko.cpp "${jack_LIBS}")
    make_plugin(jackTransport jackTransport.c "${jack_LIBS}")
endif()

check_deps(BUILD_LINEAR_ALGEBRA_OPCODES GMM_HEADER)
if(BUILD_LINEAR_ALGEBRA_OPCODES)
    make_plugin(linear_algebra linear_algebra.cpp)
endif()

check_deps(BUILD_OSC_OPCODES OSC_HEADER LIBLO_LIBRARY)
if(BUILD_OSC_OPCODES)
    make_plugin(osc OSC.c)
    target_link_libraries(osc ${LIBLO_LIBRARY} pthread)
endif()

check_deps(BUILD_IMAGE_OPCODES PNG_FOUND)
if(BUILD_IMAGE_OPCODES)
    include_directories(${PNG_INCLUDE_DIR})
    make_plugin(image imageOpcodes.c "${PNG_LIBRARIES}")
    add_compiler_flags(${PNG_DEFINITIONS} TARGETS image)
endif()

check_deps(BUILD_PYTHON_OPCODES PYTHONLIBS_FOUND)
if(BUILD_PYTHON_OPCODES)
    include_directories(${PYTHON_INCLUDE_DIRS})
    make_plugin(py py/pythonopcodes.c ${PYTHON_LIBRARIES})
endif()

#check_deps(BUILD_LUA_OPCODES LUA51_FOUND OPENMP_FOUND)
if(BUILD_LUA_OPCODES)
    if(LUA_LIBRARIES)
        find_path(LUA_INCLUDE_DIR  lua.h
            /usr/include/luajit-2.0
            /usr/local/include/luajit-2.0)
        include_directories(${LUA_INCLUDE_DIR})
        make_plugin(LuaCsound LuaCsound.cpp ${LUA_LIBRARIES})
        add_compiler_flags(${OpenMP_CXX_FLAGS} TARGETS LuaCsound)
        add_compiler_flags(${OpenMP_CXX_FLAGS} TARGETS LuaCsound LINKER)
        endif()
endif()

check_deps(BUILD_WIIMOTE_OPCODES WIIUSE_H)
if(BUILD_WIIMOTE_OPCODES)
    make_plugin(wiimote wiimote.c ${WIIUSE_LIB})
endif()

check_deps(BUILD_P5GLOVE_OPCODES P5GLOVE_H)
if(BUILD_P5GLOVE_OPCODES)
    make_plugin(p5g p5glove.c ${P5GLOVE_LIB})
endif()

# OGG Opcode previously was built only for OLPC
#if(VORBISFILE_LIBRARY)
#    set(vorbis_LIBS ${libcsound_LIBS} ${VORBISFILE_LIBRARY})
#    make_plugin(ogg ogg.c "${vorbis_LIBS}")
#endif()



#set(sfont_FLAGS "")

#if(CMAKE_C_COMPILER MATCHES "gcc")
#    set(sfont_FLAGS ${sfont_FLAGS} -fno-strict-aliasing)
#endif()
#if(BIG_ENDIAN)
#    set(sfont_FLAGS ${sfont_FLAGS} -DWORDS_BIGENDIAN)
#endif()
#message(STATUS "Compiler flags used for sfont lib: ${sfont_FLAGS}")
#make_plugin(sfont sfont.c m)
#set_target_properties(sfont
#    PROPERTIES COMPILER_FLAGS "${sfont_FLAGS}")
    
add_subdirectory(stk)
