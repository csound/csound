set(BUILD_RELEASE ON)
set(BUILD_INSTALLER ON)
set(BUILD_STATIC_LIBRARY ON)
set(BUILD_TESTS OFF)

set(BUILD_CSBEATS ON)
set(BUILD_CXX_INTERFACE ON)
set(BUILD_DSSI_OPCODES OFF)
set(BUILD_JAVA_INTERFACE ON)
set(BUILD_MULTI_CORE ON)
set(BUILD_OSC_OPCODES ON)
set(BUILD_PADSYNTH_OPCODES ON)
set(BUILD_SCANSYN_OPCODES ON)
set(BUILD_UTILITIES ON)

set(USE_ALSA 0) # N/A
set(USE_ATOMIC_BUILTIN 0) # Needs code changes for MSVC
set(USE_AUDIOUNIT 0) # N/A
set(USE_CURL 0)
set(USE_COMPILER_OPTIMIZATIONS 1)
set(USE_COREMIDI 0) # N/A
set(USE_DOUBLE 1)
set(USE_GETTEXT	0)
set(USE_IPMIDI 1)
set(USE_JACK 0) # N/A
set(USE_LIB64 1)
set(USE_LRINT 1)
set(USE_MP3 1)
set(USE_PORTAUDIO 1)
set(USE_PORTMIDI 1)
set(USE_PULSEAUDIO 0) # N/A
set(USE_SYSTEM_PORTSMF 1)

set(HAVE_BIG_ENDIAN 0)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_16BIT_TYPE "unsigned short")
set(FAIL_MISSING ON) # Enable when packaging

# Disable the following warnings in msvc
# - C4244 loss of data in conversion
# - C4267 loss of data in conversion
# - C4005 macro refinitions
# - C4996 unsafe functions
# - C4047 levels of indirection difference (int and void*)
# - C4090 different const qualifiers
# - C4477 format string type differences
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244 /wd4267 /wd4005 /wd4996 /wd4047 /wd4090 /wd4477")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4267 /wd4005 /wd4996 /wd4047 /wd4090 /wd4477 /wd4251")
