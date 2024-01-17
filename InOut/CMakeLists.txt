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
    else()
        find_package(PORTAUDIO)
    endif()
endif()

if(USE_PORTMIDI)
    if(USE_VCPKG)
        find_package(PortMidi CONFIG REQUIRED)
    else()
        find_package(PORTMIDI)
    endif()
endif()


if(USE_JACK)
    find_library(JACK_LIBRARY jack)
    check_include_file(jack/jack.h JACK_HEADER)
    if(APPLE)
    # VL: could not find a way to make check_include_file() work so
    # I am hardcoding it for MacOS
    set(JACK_HEADER  "jack/jack.h")
    endif()
    # HLO: including the jack2 common source dir
    # prevents "cannot open include file" errors
    find_path(JACK_INCLUDE_PATH jack/jack.h)
endif()

find_package(AudioUnit)
find_package(CoreAudio)
find_package(CoreFoundation)
find_package(CoreMIDI)

# BUILD TARGETS

check_deps(USE_COREMIDI
    CoreAudio_FOUND
    CoreMIDI_FOUND
    CoreFoundation_FOUND
)
if(USE_COREMIDI)
    make_plugin(cmidi cmidi.c)
    target_link_libraries(cmidi PUBLIC
        CoreAudio::coreaudio
        CoreFoundation::corefoundation
        CoreMIDI::coremidi
    )
endif()

check_deps(USE_AUDIOUNIT
    AudioUnit_FOUND
    CoreAudio_FOUND
    CoreFoundation_FOUND
)
if (USE_AUDIOUNIT)
    make_plugin(rtauhal rtauhal.c)
    target_link_libraries(rtauhal PUBLIC
        AudioUnit::audiounit
        CoreAudio::coreaudio
        CoreFoundation::corefoundation
    )
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

if(PORTAUDIO_FOUND OR USE_VCPKG)
    message(STATUS "Building rtpa module.")
    make_plugin(rtpa rtpa.c)
    if(USE_VCPKG)
        target_link_libraries(rtpa PRIVATE portaudio_static)
    else()
        target_include_directories(rtpa PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
        target_link_libraries(rtpa PRIVATE ${PORTAUDIO_LIBRARIES})
    endif()
else()
  message(STATUS "Not building Portaudio backend...")
  message(STATUS "Portaudio v19 Found: ${PORTAUDIO_V19}.")
endif()

if(PORTMIDI_FOUND OR USE_VCPKG)
    if(USE_VCPKG)
        find_package(Threads REQUIRED)
        make_plugin(pmidi pmidi.c)
        target_link_libraries(pmidi PRIVATE PortMidi::portmidi)
    else()
        make_plugin(pmidi pmidi.c)
        target_include_directories(pmidi PRIVATE ${PORTMIDI_INCLUDE_DIRS})
        target_link_libraries(pmidi PRIVATE ${PORTMIDI_LIBRARIES})
        if(WIN32)
            target_link_libraries(pmidi PRIVATE ${CSOUND_WINDOWS_LIBRARIES})
            elseif(LINUX AND ALSA_LIBRARY)
            target_link_libraries(pmidi PRIVATE ${ALSA_LIBRARY})
        endif()
    endif()
#else()
 message(STATUS "Not building Portmidi backend...")
endif()

#check_deps(USE_IPMIDI PORTMIDI_HEADER PORTMIDI_LIBRARY)
if(USE_IPMIDI)
    if(WIN32)
      set(ipmidi_LIBS ws2_32)
    endif()
    make_plugin(ipmidi ipmidi.c "${ipmidi_LIBS}")
endif()

check_deps(USE_JACK JACK_HEADER JACK_LIBRARY JACK_INCLUDE_PATH Threads_FOUND)
if(USE_JACK)
    set(rtjack_LIBS ${JACK_LIBRARY} Threads::Threads)
    # set(rtjack_SRCS rtjack.c natsort.c)
    set(rtjack_SRCS rtjack.c alphanumcmp.c)
    make_plugin(rtjack "${rtjack_SRCS}" "${rtjack_LIBS}")
    # make_plugin(rtjack rtjack.c "${rtjack_LIBS}")
    target_include_directories(rtjack PRIVATE "${JACK_INCLUDE_PATH}")
endif()

if(HAIKU)
	find_library(HAIKU_MEDIA_LIBRARY media)
	find_library(HAIKU_MIDI_LIBRARY midi2)
	set(haiku_SRCS rthaiku.cpp HaikuAudio.cpp HaikuMidi.cpp)
	make_plugin(rthaiku "${haiku_SRCS}")
    target_link_libraries(rthaiku PRIVATE ${HAIKU_MEDIA_LIBRARY})
    target_link_libraries(rthaiku PRIVATE ${HAIKU_MIDI_LIBRARY})
endif()


