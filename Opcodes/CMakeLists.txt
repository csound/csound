option(BUILD_CHUA_OPCODES "Build the chua opcodes (requires boost)" ON)
option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" ON)

find_package(Boost)
check_include_file(ladspa.h LADSPA_H)

## OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##

make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)

make_plugin(ampmidid ampmidid.cpp)
make_plugin(cs_date date.c)
make_plugin(system_call system_call.c)
make_plugin(mutexops mutexops.cpp)
make_plugin(doppler doppler.cpp)

# temporarily do not build on Windows until these opcodes are fixed
# to compile
if(NOT WIN32) 
    make_plugin(udprecv sockrecv.c)
    make_plugin(udpsend socksend.c)

endif()

set(scansyn_SRCS
    scansyn.c scansynx.c)
make_plugin(scansyn "${scansyn_SRCS}" m)

if(UNIX)
    make_plugin(control control.c)
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
    make_plugin(dssi4cs "${dssi_SRC}" )
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

    
          

#if wiifound==1:
#  WiiEnvironment = pluginEnvironment.Clone()
#  makePlugin(WiiEnvironment, 'wiimote', ['wiimote.c'])
#if p5gfound==1:
#  P5GEnvironment = pluginEnvironment.Clone()
#  makePlugin(P5GEnvironment, 'p5g', ['p5glove.c'])

add_subdirectory(stk)
