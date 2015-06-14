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
option(BUILD_FAUST_OPCODES "Build the Faust opcodes" OFF)
option(BUILD_VST4CS_OPCODES "Build the vst4cs opcodes" OFF)
option(BUILD_EXCITER_OPCODES "Build the exciter opcode" ON)
option(BUILD_BUCHLA_OPCODES "Build the Buchla filter opcode" ON)
option(BUILD_PADSYNTH_OPCODES "Build the Padsynth opcode" ON)
option(BUILD_SCANSYN_OPCODES "Build the scansyn opcodes" ON)

find_package(Boost)
check_include_file(ladspa.h LADSPA_H)
check_include_file(fluidsynth.h FLUIDSYNTH_H)
find_library(FLUIDSYNTH_LIBRARY fluidsynth)
find_library(JACK_LIBRARY jack)
find_library(JACKDMP_LIBRARY jackdmp)
check_include_file(jack/jack.h JACK_HEADER)
check_include_file_cxx(gmm/gmm.h GMM_HEADER)
check_include_file_cxx(eigen3/E.h GMM_HEADER)
check_include_file(lo/lo.h OSC_HEADER)
find_library(LIBLO_LIBRARY lo)
find_package(ZLIB)
find_package(PNG)
set(PYTHON_INCLUDE_DIRS /usr/include/Python2.7)
set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs)

find_library(LUAJIT_LIBRARY luajit)
if(NOT LUAJIT_LIBRARY)
    find_library(LUAJIT_LIBRARY luajit-5.1)
endif()
if(NOT LUAJIT_LIBRARY AND WIN32)
    find_library(LUAJIT_LIBRARY lua51)
endif()
find_path(LUA_H_PATH lua.h PATH_SUFFIXES luajit-2.0)

check_include_file(wiiuse.h WIIUSE_H)
find_library(WIIUSE_LIB wiiuse)
check_include_file(p5glove.h P5GLOVE_H)
find_library(P5GLOVE_LIB p5glove)

if(BUILD_FAUST_OPCODES)
find_package(LLVM)
exec_program(llvm-config ARGS --libs OUTPUT_VARIABLE LLVM_LIBS)
find_library(FAUST_LIBRARY faust.a HINTS /usr/local/lib/faust)
endif()

## OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##

make_plugin(mixer mixer.cpp)
make_plugin(signalflowgraph signalflowgraph.cpp)

make_plugin(ampmidid ampmidid.cpp)
make_plugin(cs_date date.c)
make_plugin(system_call system_call.c)
make_plugin(doppler doppler.cpp)
make_plugin(fractalnoise tl/fractalnoise.cpp)

make_plugin(fareygen fareygen.c)

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
endif()
endif()


make_plugin(cellular cellular.c)

if(BUILD_SERIAL_OPCODES)
   make_plugin(serial serial.c)
endif()

if(BUILD_PLATEREV_OPCODES)
   make_plugin(platerev platerev.c m)
endif()

if(BUILD_SCANSYN_OPCODES)
    set(scansyn_SRCS
        scansyn.c scansynx.c)
    make_plugin(scansyn "${scansyn_SRCS}" m)
endif()

if(UNIX)
    make_plugin(control control.c)
endif()

if(LINUX)
    make_plugin(urandom urandom.c)
    make_plugin(joystick linuxjoystick.c)
endif()

## OPCODES WITH EXTERNAL DEPENDENCIES ##

check_deps(BUILD_CHUA_OPCODES Boost_INCLUDE_DIRS)
if(BUILD_CHUA_OPCODES)
    include_directories(${Boost_INCLUDE_DIRS})
    make_plugin(chua chua/ChuaOscillator.cpp)
endif()

check_deps(BUILD_DSSI_OPCODES LADSPA_H)
if(BUILD_DSSI_OPCODES)
    set(dssi_SRC dssi4cs/src/load.c dssi4cs/src/dssi4cs.c)
    make_plugin(dssi4cs "${dssi_SRC}" dl)
endif()

check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_H FLUIDSYNTH_LIBRARY)
if(BUILD_FLUID_OPCODES)
    if(EXISTS ${FLUIDSYNTH_INCLUDE_DIR})
        include_directories("${FLUIDSYNTH_INCLUDE_DIR}")
     endif()
    make_plugin(fluidOpcodes fluidOpcodes/fluidOpcodes.cpp ${FLUIDSYNTH_LIBRARY})
    add_dependency_to_framework(fluidOpcodes ${FLUIDSYNTH_LIBRARY})
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

check_deps(BUILD_OSC_OPCODES LIBLO_LIBRARY OSC_HEADER)
if(BUILD_OSC_OPCODES)
   message(STATUS "OSC OPCODES!!!!!!")
    make_plugin(osc OSC.c)
    if(WIN32)
      target_link_libraries(osc ${LIBLO_LIBRARY} pthread wsock32 ws2_32)
    else()
      target_link_libraries(osc ${LIBLO_LIBRARY} pthread)
    endif()
    add_dependency_to_framework(osc ${LIBLO_LIBRARY})
endif()

check_deps(BUILD_IMAGE_OPCODES PNG_FOUND)
if(BUILD_IMAGE_OPCODES)
    include_directories(${PNG_INCLUDE_DIR})
    make_plugin(image imageOpcodes.c "${PNG_LIBRARIES}")
    add_compiler_flags(${PNG_DEFINITIONS} TARGETS image)
    add_dependency_to_framework(image ${PNG_LIBRARY})
endif()

check_deps(BUILD_PYTHON_OPCODES PYTHONLIBS_FOUND)
if(BUILD_PYTHON_OPCODES)
    include_directories(${PYTHON_INCLUDE_DIRS})
    include_directories(${CMAKE_HOME_DIRECTORY}/include)
    make_plugin(py py/pythonopcodes.c ${PYTHON_LIBRARIES})
endif()

if(NOT APPLE)
  check_deps(BUILD_LUA_OPCODES LUAJIT_LIBRARY LUA_H_PATH)
  if(BUILD_LUA_OPCODES)
    make_plugin(LuaCsound LuaCsound.cpp ${LUAJIT_LIBRARY})
    add_compiler_flags(${OpenMP_CXX_FLAGS} TARGETS LuaCsound)
    add_compiler_flags(${OpenMP_CXX_FLAGS} TARGETS LuaCsound LINKER)
    include_directories(${LUA_H_PATH})
  endif()
endif()

check_deps(BUILD_WIIMOTE_OPCODES WIIUSE_H WIIUSE_LIB)
if(BUILD_WIIMOTE_OPCODES)
    make_plugin(wiimote wiimote.c ${WIIUSE_LIB})
    add_dependency_to_framework(wiimote ${WIIUSE_LIB})

endif()

if(BUILD_EXCITER_OPCODES)
    make_plugin(exciter exciter.c)
endif()

if(BUILD_BUCHLA_OPCODES)
    make_plugin(buchla buchla.c)
endif()

check_deps(BUILD_P5GLOVE_OPCODES P5GLOVE_H)
if(BUILD_P5GLOVE_OPCODES)
    make_plugin(p5g p5glove.c ${P5GLOVE_LIB})
endif()

check_deps(BUILD_FAUST_OPCODES LLVM_FOUND FAUST_LIBRARY)
if(BUILD_FAUST_OPCODES)
  make_plugin(faustcsound faustgen.cpp)
  target_link_libraries(faustcsound -lcrypto)
 if(APPLE)
 if(${OSX_VERSION} STREQUAL "10.6")
   add_compiler_flags("-DFAUSTFLOAT=double "  TARGETS faustcsound)
    target_link_libraries(faustcsound ${FAUST_LIBRARY} -ldl -L$ENV{HOME}/lib -L/usr/local/lib ${LLVM_LIBS})
  else()
     add_compiler_flags("-DFAUSTFLOAT=double -stdlib=libc++"  TARGETS faustcsound)
     target_link_libraries(faustcsound ${FAUST_LIBRARY} -stdlib=libc++ -ldl -L$ENV{HOME}/lib -L/usr/local/lib ${LLVM_LIBS})
  endif()
  else()
    add_compiler_flags("-DFAUSTFLOAT=double "  TARGETS faustcsound)
    target_link_libraries(faustcsound ${FAUST_LIBRARY} -ldl -L$ENV{HOME}/lib -L/usr/local/lib ${LLVM_LIBS})
   endif()

  include_directories(${LLVM_INCLUDE_DIRS})
endif()


# OGG Opcode previously was built only for OLPC
#if(VORBISFILE_LIBRARY)
#    set(vorbis_LIBS ${libcsound_LIBS} ${VORBISFILE_LIBRARY})
#    make_plugin(ogg ogg.c "${vorbis_LIBS}")
#endif()

check_deps(BUILD_VST4CS_OPCODES VSTSDK2X_DIR FLTK_FOUND)
if(BUILD_VST4CS_OPCODES)
    include_directories(${CMAKE_HOME_DIRECTORY}/interfaces)
    include_directories(${VSTSDK2X_DIR})
    include_directories(${VSTSDK2X_DIR}/public.sdk/source/vst2.x)
    include_directories(${VSTSDK2X_DIR}/pluginterfaces/source/vst2.x)
    include_directories(${FLTK_INCLUDE_DIR})
    add_definitions(-DVST_FORCE_DEPRECATED=0 -DCS_VSTHOST)
    set(VST4CS_SRCS
        vst4cs/src/vst4cs.cpp
        vst4cs/src/vsthost.cpp
        vst4cs/src/fxbank.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
        ${VSTSDK2X_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp)
    add_library(vst4cs SHARED ${VST4CS_SRCS})
    set_target_properties(vst4cs PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    target_link_libraries(vst4cs ${CSOUNDLIB} ${FLTK_LIBRARIES})
    install(TARGETS vst4cs
        LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR})
endif()


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
add_subdirectory(cuda)
add_subdirectory(opencl)
add_subdirectory(hdf5)
add_subdirectory(websockets)
add_subdirectory(framebuffer)

