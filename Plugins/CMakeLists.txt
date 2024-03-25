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

option(BUILD_CPP_OPCODES "Build CPP plugins" ON)
option(BUILD_DEPRECATED_OPCODES "Build deprecated opcodes" ON)
option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" OFF)
option(USE_IPMIDI "Build the IPMIDI I/O module" ON)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" ON)
option(BUILD_PADSYNTH_OPCODES "Build the Padsynth opcode" ON)
option(USE_PORTMIDI "Build the PortMIDI I/O module" ON)
option(BUILD_SCANSYN_OPCODES "Build the scansyn opcodes" ON)
option(USE_ALSA "Build the ALSA I/O module" ON)
option(USE_AUDIOUNIT "Build the CoreAudio AudioUnit I/O module (requires CoreAudio)" ON)
option(USE_COREMIDI "Build the CoreMIDI I/O Module" ON)
option(USE_JACK "Build the jack I/O module and opcodes" ON)
option(USE_PORTAUDIO "Build the PortAudio I/O module" ON)
option(USE_PULSEAUDIO "Build the PulseAudio I/O module" ON)

find_package(AudioUnit MODULE)
find_package(CoreAudio MODULE)
find_package(CoreFoundation MODULE)
find_package(CoreMIDI MODULE)
find_package(Jack MODULE)
# config file shipped by vcpkg isn't working, so use module
find_package(LibLO MODULE)

if (USE_VCPKG)
    find_package(portaudio CONFIG)
    # could be the static or dynamic version
    assign_bool(portaudio_FOUND (TARGET portaudio_static) OR (TARGET portaudio))
else()
    find_package(portaudio MODULE)
endif()

if (USE_VCPKG)
    find_package(PortMidi CONFIG)
else()
    find_package(PortMidi MODULE)
endif()

find_package(PulseAudio MODULE)

# alsa can only be loaded once, and might already be loaded by portmidi
if (NOT TARGET ALSA::ALSA)
    find_package(ALSA MODULE)
endif()

check_deps(BUILD_DSSI_OPCODES LINUX)
check_deps(BUILD_OSC_OPCODES LibLO_FOUND)
check_deps(USE_ALSA ALSA_FOUND LINUX)
check_deps(USE_AUDIOUNIT CoreAudio_FOUND AudioUnit_FOUND CoreFoundation_FOUND)
check_deps(USE_COREMIDI CoreMIDI_FOUND CoreFoundation_FOUND)
check_deps(USE_JACK Jack_FOUND)
check_deps(USE_PORTAUDIO portaudio_FOUND)
check_deps(USE_PORTMIDI PortMidi_FOUND)
check_deps(USE_PULSEAUDIO PulseAudio_FOUND)

if (LINUX)
    add_subdirectory(joystick)
endif()

if (UNIX)
    add_subdirectory(control)
    add_subdirectory(urandom)
endif()

if (WIN32)
    add_subdirectory(rtwinmm)
endif()

if (BUILD_CPP_OPCODES)
    add_subdirectory(ampmidid)
    add_subdirectory(arrayops)
    add_subdirectory(bformdec2)
    add_subdirectory(doppler)
    add_subdirectory(fractalnoise)
    add_subdirectory(lsfr)
    add_subdirectory(mixer)
    add_subdirectory(pvsops)
    add_subdirectory(signalflowgraph)
    add_subdirectory(trigenvsegs)

    if (DIRENT_H)
        add_subdirectory(ftsamplebank)
    endif()
endif()

if (BUILD_DEPRECATED_OPCODES)
    add_subdirectory(deprecated)
endif()

if (BUILD_DSSI_OPCODES)
    add_subdirectory(dssi4cs)
endif()

if (HAIKU)
    add_subdirectory(rthaiku)
endif()

if (BUILD_OSC_OPCODES)
    add_subdirectory(osc)
endif()

if (BUILD_PADSYNTH_OPCODES)
    add_subdirectory(padsynth)
endif()

if (BUILD_SCANSYN_OPCODES)
    add_subdirectory(scansyn)
endif()

if (BUILD_UTILITIES)
    add_subdirectory(stdutil)
endif()

if (USE_ALSA)
    add_subdirectory(rtalsa)
endif()

if (USE_AUDIOUNIT)
    add_subdirectory(rtauhal)
endif()

if (USE_COREMIDI)
    add_subdirectory(cmidi)
endif()

if (USE_IPMIDI)
    add_subdirectory(ipmidi)
endif()

if (USE_JACK)
    add_subdirectory(rtjack)
endif()

if (USE_PORTAUDIO)
    add_subdirectory(rtpa)
endif()

if (USE_PORTMIDI)
    add_subdirectory(pmidi)
endif()

if (USE_PULSEAUDIO)
    add_subdirectory(rtpulse)
endif()