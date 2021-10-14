# REAL TIME AUDIO AND MIDI

message(STATUS "")
message(STATUS "## REALTIME AUDIO AND MIDI ##")

option(USE_PULSEAUDIO "Build the PulseAudio I/O module" ON)
option(USE_PORTAUDIO "Build the PortAudio I/O module" ON)
option(USE_PORTMIDI "Build the PortMIDI I/O module" ON)
option(USE_IPMIDI "Build the IPMIDI I/O module" ON)
option(USE_JACK "Build the jack I/O module and opcodes" ON)
option(USE_ALSA "Build the ALSA I/O module" ON)
# option(USE_COREAUDIO "Build the CoreAudio I/O module" ON)
# option(USE_COREMIDI "Build the CoreMIDI I/O Module" ON)
# option(USE_AUDIOUNIT "Build the CoreAudio AudioUnit I/O module (requires CoreAudio)" ON)

list(APPEND CMAKE_REQUIRED_INCLUDES "/usr/local/include")

# FIND LIBRARIES AND HEADERS

if(USE_ALSA AND LINUX)
    find_library(ALSA_LIBRARY asound)
    check_include_file(alsa/asoundlib.h ALSA_HEADER)
endif()
if(USE_PORTAUDIO)
    # FIXME the msvc branch should use the VCPKG config file for portaudio but it's currently broken
    # if(MSVC)
        # find_package(PORTAUDIO CONFIG REQUIRED)
    # else()
        find_package(PORTAUDIO)
    # endif()
    
    # find_path(PORTAUDIO_INCLUDE_PATH portaudio.h)
    # find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio_x64)

    # if(PORTAUDIO_INCLUDE_PATH AND PORTAUDIO_LIBRARY)
    #     try_run(PORTAUDIO_V19 PORTAUDIO_TEST_COMPILED
    #         ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/portaudio_test.c
    #         CMAKE_FLAGS -DLINK_LIBRARIES:STRING=${PORTAUDIO_LIBRARY}
    #         COMPILE_DEFINITIONS "-I${PORTAUDIO_INCLUDE_PATH}"
    #         COMPILE_OUTPUT_VARIABLE OUTPUT)
    #     if(NOT PORTAUDIO_TEST_COMPILED AND NOT WIN32)
    #         set(PORTAUDIO_V19 false)
    #     endif()
    # else()
    #     set(PORTAUDIO_V19 false)
    # endif()
endif()
if(USE_PORTMIDI)
    find_package(PORTMIDI)
endif()
if(USE_JACK)
    find_library(JACK_LIBRARY jack)
    check_include_file(jack/jack.h JACK_HEADER)
    if(APPLE)
    # VL: could not find a way to make check_include_file() work so
    # I am hardcoding it for MacOS
    set(JACK_HEADER  "jack/jack.h")
    endif()
    if(WIN32)
    # HLO: including the jack2 common source dir
    # prevents "cannot open include file" errors
    find_path(JACK_INCLUDE_PATH jack/jack.h)
    endif()
endif()
if(USE_PULSEAUDIO)
    find_library(PULSEAUDIO_LIBRARY pulse)
    find_library(PULSESIMPLE_LIBRARY pulse-simple)
    check_include_file(pulse/simple.h PULSEAUDIO_HEADER)
endif()
# if(USE_COREAUDIO)
#    find_path(COREAUDIO_INCLUDE_PATH CoreAudio.h)
#    find_library(COREAUDIO_LIBRARY CoreAudio)
# endif()
if(APPLE)
    find_path(COREMIDI_INCLUDE_PATH CoreMIDI.h)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_path(COREAUDIO_INCLUDE_PATH CoreAudio.h)
    find_library(COREAUDIO_LIBRARY CoreAudio)
endif()

# BUILD TARGETS

# check_deps(USE_COREAUDIO APPLE COREAUDIO_INCLUDE_PATH COREAUDIO_LIBRARY)
# if(USE_COREAUDIO)
#    make_plugin(rtcoreaudio rtcoreaudio.c)
#    target_include_directories(rtcoreaudio PRIVATE ${COREAUDIO_INCLUDE_PATH})
#    target_link_libraries(rtcoreaudio ${COREAUDIO_LIBRARY})
# endif()

if(APPLE)
#    check_deps(USE_COREMIDI APPLE COREMIDI_INCLUDE_PATH COREMIDI_LIBRARY COREFOUNDATION_LIBRARY)
#   check_deps(USE_AUDIOUNIT APPLE COREAUDIO_INCLUDE_PATH COREAUDIO_LIBRARY AUDIOUNIT_INCLUDE_PATH AUDIOUNIT_LIBRARY COREFOUNDATION_LIBRARY)
        make_plugin(cmidi cmidi.c)
        target_include_directories(cmidi PRIVATE ${COREMIDI_INCLUDE_PATH})
        target_link_libraries(cmidi ${COREMIDI_LIBRARY} ${COREFOUNDATION_LIBRARY})
        make_plugin(rtauhal rtauhal.c)
        target_include_directories(rtauhal PRIVATE ${AUDIOUNIT_INCLUDE_PATH})
        target_link_libraries(rtauhal ${AUDIOUNIT_LIBRARY} ${COREFOUNDATION_LIBRARY} ${COREAUDIO_LIBRARY})
endif()

check_deps(USE_ALSA ALSA_HEADER ALSA_LIBRARY PTHREAD_LIBRARY)
if(USE_ALSA)
    set(rtalsa_LIBS
        ${ALSA_LIBRARY} ${PTHREAD_LIBRARY} ${MATH_LIBRARY})
    make_plugin(rtalsa rtalsa.c ${rtalsa_LIBS})
endif()

if(WIN32)
    message(STATUS "Building Windows MME plugin(rtwinmm).")
    if(MSVC)
      make_plugin(rtwinmm rtwinmm.c)
      target_link_libraries(rtwinmm winmm.lib)
      target_link_libraries(rtwinmm gdi32)
      target_link_libraries(rtwinmm kernel32)
    else()
      set(rtwinmm_LIBS
          winmm gdi32 kernel32) # should probably do checks for these libs
      make_plugin(rtwinmm rtwinmm.c "${rtwinmm_LIBS}")
    endif()
endif()


check_deps(USE_PULSEAUDIO PULSEAUDIO_HEADER PULSEAUDIO_LIBRARY PULSESIMPLE_LIBRARY)
if(USE_PULSEAUDIO)
    make_plugin(rtpulse rtpulse.c ${PULSEAUDIO_LIBRARY} ${PULSESIMPLE_LIBRARY})
endif()

if(PORTAUDIO_FOUND)
    message(STATUS "Building rtpa module.")
    make_plugin(rtpa rtpa.c)

    if(MSVC)
        # FIXME should be able to use VCPKG export but not working currently
        # target_link_libraries(rtpa PRIVATE portaudio portaudio_static)
        target_link_libraries(rtpa PRIVATE ${PORTAUDIO_LIBRARIES})
    else()
        target_include_directories(rtpa PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
        target_link_libraries(rtpa ${PORTAUDIO_LIBRARIES})
    endif()
else()
  message(STATUS "Not building Portaudio Driver...")
  message(STATUS "Portaudio v19 Found: ${PORTAUDIO_V19}.")
endif()

if(USE_PORTMIDI AND PORTMIDI_FOUND)
    make_plugin(pmidi pmidi.c)
    target_include_directories(pmidi PRIVATE ${PORTMIDI_INCLUDE_DIRS})
    target_link_libraries(pmidi ${PORTMIDI_LIBRARIES})
    if(WIN32)
      target_link_libraries(pmidi ${CSOUND_WINDOWS_LIBRARIES})
    elseif(LINUX AND ALSA_LIBRARY)
      target_link_libraries(pmidi ${ALSA_LIBRARY})
    endif()
endif()

#check_deps(USE_IPMIDI PORTMIDI_HEADER PORTMIDI_LIBRARY)
if(USE_IPMIDI)
    if(WIN32)
      set(ipmidi_LIBS ws2_32)
    endif()
    make_plugin(ipmidi ipmidi.c "${ipmidi_LIBS}")
endif()

check_deps(USE_JACK JACK_HEADER JACK_LIBRARY)
if(USE_JACK)
    set(rtjack_LIBS "")
    if(LINUX)
        list(APPEND rtjack_LIBS
            ${JACK_LIBRARY} ${PTHREAD_LIBRARY})
    elseif(WIN32)
        check_deps(JACK_INCLUDE_PATH)
        include_directories("${JACK_INCLUDE_PATH}")
        list(APPEND rtjack_LIBS
            ${JACK_LIBRARY} ${PTHREAD_LIBRARY})
    else()
        list(APPEND rtjack_LIBS
            ${JACK_LIBRARY} ${PTHREAD_LIBRARY})
    endif()
    # set(rtjack_SRCS rtjack.c natsort.c)
    set(rtjack_SRCS rtjack.c alphanumcmp.c)
    make_plugin(rtjack "${rtjack_SRCS}" "${rtjack_LIBS}")
    # make_plugin(rtjack rtjack.c "${rtjack_LIBS}")
endif()

if(HAIKU)
	find_library(HAIKU_MEDIA_LIBRARY media)
	find_library(HAIKU_MIDI_LIBRARY midi2)
	set(haiku_SRCS rthaiku.cpp HaikuAudio.cpp HaikuMidi.cpp)
	make_plugin(rthaiku "${haiku_SRCS}")
    target_link_libraries(rthaiku ${HAIKU_MEDIA_LIBRARY})
    target_link_libraries(rthaiku ${HAIKU_MIDI_LIBRARY})
endif()


