# REAL TIME AUDIO AND MIDI

message(STATUS "")
message(STATUS "## REALTIME AUDIO AND MIDI ##")

option(USE_PULSEAUDIO "Build the PulseAudio I/O module" ON)
option(USE_PORTAUDIO "Build the PortAudio I/O module" ON)
option(USE_PORTMIDI "Build the PortMIDI I/O module" ON)
option(USE_IPMIDI "Build the IPMIDI I/O module" ON)
option(USE_JACK "Build the jack I/O module and opcodes" ON)
option(USE_ALSA "Build the ALSA I/O module" ON)
option(USE_COREMIDI "Build the CoreMIDI I/O Module" ON)
option(USE_AUDIOUNIT "Build the CoreAudio AudioUnit I/O module (requires CoreAudio)" ON)

list(APPEND CMAKE_REQUIRED_INCLUDES "/usr/local/include")

# FIND LIBRARIES AND HEADERS

if(USE_PORTAUDIO)
    if (USE_VCPKG)
        find_package(portaudio CONFIG REQUIRED)
        # could be the static or dynamic version
        assign_bool(portaudio_FOUND (TARGET portaudio_static) OR (TARGET portaudio))
    else()
        find_package(portaudio MODULE)
    endif()
endif()

if(USE_PORTMIDI)
    if(USE_VCPKG)
        find_package(PortMidi CONFIG REQUIRED)
    else()
        find_package(PortMidi MODULE)
    endif()
endif()

if(USE_JACK)
    find_package(Jack MODULE)
endif()

# BUILD TARGETS

if(APPLE)
    find_package(CoreFoundation MODULE)

    find_package(CoreMIDI MODULE)
    check_deps(USE_COREMIDI CoreMIDI_FOUND CoreFoundation_FOUND)
    if(USE_COREMIDI)
        make_plugin(cmidi cmidi.c)
        if(OSXCROSS_TARGET)
            target_link_libraries(cmidi PRIVATE ${CoreMIDI_LIBRARY} ${CoreFoundation_LIBRARY})
        else()
            target_link_libraries(cmidi PRIVATE CoreMIDI::coremidi CoreFoundation::corefoundation)
        endif()
    endif()

    find_package(CoreAudio MODULE)
    find_package(AudioUnit MODULE)
    check_deps(USE_AUDIOUNIT CoreAudio_FOUND AudioUnit_FOUND CoreFoundation_FOUND)
    if (USE_AUDIOUNIT)
        make_plugin(rtauhal rtauhal.c)
        if(OSXCROSS_TARGET)
            target_link_libraries(rtauhal PRIVATE ${AudioUnit_LIBRARY} ${CoreFoundation_LIBRARY} ${CoreAudio_LIBRARY})
        else()
            target_link_libraries(rtauhal PRIVATE AudioUnit::audiounit CoreFoundation::corefoundation CoreAudio::coreaudio)
        endif()
    endif()
endif()

if(USE_ALSA AND LINUX)
    if (USE_VCPKG)
        find_package(ALSA MODULE REQUIRED)
    else()
        find_package(ALSA REQUIRED)
    endif()

    make_plugin(rtalsa rtalsa.c ALSA::ALSA)
endif()

if(WIN32)
    message(STATUS "Building Windows MME plugin(rtwinmm).")

    if(MSVC)
      make_plugin(rtwinmm rtwinmm.c)
      target_link_libraries(rtwinmm PRIVATE winmm.lib)
      target_link_libraries(rtwinmm PRIVATE gdi32)
      target_link_libraries(rtwinmm PRIVATE kernel32)
    else()
      set(rtwinmm_LIBS
          winmm gdi32 kernel32) # should probably do checks for these libs
      make_plugin(rtwinmm rtwinmm.c "${rtwinmm_LIBS}")
    endif()
endif()

find_package(PulseAudio)
check_deps(USE_PULSEAUDIO PulseAudio_FOUND)

if(USE_PULSEAUDIO)
    make_plugin(rtpulse rtpulse.c PulseAudio::PulseAudio)
endif()

check_deps(USE_PORTAUDIO portaudio_FOUND)
if(USE_PORTAUDIO)
    make_plugin(rtpa rtpa.c)
    # use the static library if available
    target_link_libraries(rtpa PRIVATE $<IF:$<TARGET_EXISTS:portaudio_static>,portaudio_static,portaudio>)
endif()

check_deps(USE_PORTMIDI PortMidi_FOUND)
if(USE_PORTMIDI)
    make_plugin(pmidi pmidi.c)
    target_link_libraries(pmidi PRIVATE PortMidi::portmidi)
endif()

if(USE_IPMIDI)
    if(WIN32)
      set(ipmidi_LIBS ws2_32)
    endif()

    make_plugin(ipmidi ipmidi.c "${ipmidi_LIBS}")
endif()

check_deps(USE_JACK Jack_FOUND)
if(USE_JACK)
    set(rtjack_SRCS rtjack.c alphanumcmp.c)
    make_plugin(rtjack "${rtjack_SRCS}")
    target_link_libraries(rtjack PRIVATE Jack::jack)
endif()

if(HAIKU)
	find_library(HAIKU_MEDIA_LIBRARY media)
	find_library(HAIKU_MIDI_LIBRARY midi2)

	set(haiku_SRCS rthaiku.cpp HaikuAudio.cpp HaikuMidi.cpp)

	make_plugin(rthaiku "${haiku_SRCS}")
    target_link_libraries(rthaiku PRIVATE ${HAIKU_MEDIA_LIBRARY})
    target_link_libraries(rthaiku PRIVATE ${HAIKU_MIDI_LIBRARY})
endif()


