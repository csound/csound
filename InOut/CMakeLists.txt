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
option(USE_COREMIDI "Build the CoreMIDI I/O Module" ON)
option(USE_AUDIOUNIT "Build teh CoreAudio AudioUnit I/O module (requires CoreAudio)" ON)
option(USE_FLTK "Use FLTK for graphs and widget opcodes" ON)
option(BUILD_VIRTUAL_KEYBOARD "Build Virtual MIDI keyboard" ON)

# FIND LIBRARIES AND HEADERS

if(USE_ALSA)
    find_library(ALSA_LIBRARY asound)
    check_include_file(alsa/asoundlib.h ALSA_HEADER)
endif()
if(USE_PORTAUDIO)
    find_library(PORTAUDIO_LIBRARY portaudio)
    check_include_file(portaudio.h PORTAUDIO_HEADER)
endif()
if(USE_PORTMIDI)
    find_library(PORTMIDI_LIBRARY portmidi)
#    find_library(PORTTIME_LIBRARY porttime)
    check_include_file(portmidi.h PORTMIDI_HEADER)
endif()
if(USE_JACK)
    find_library(JACK_LIBRARY jack)
    find_library(JACKDMP_LIBRARY jackdmp)
    check_include_file(jack/jack.h JACK_HEADER)
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
if(USE_COREMIDI)
    find_path(COREMIDI_INCLUDE_PATH CoreMidi.h)
    find_library(COREMIDI_LIBRARY CoreMidi)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif()
if(USE_AUDIOUNIT)
    find_path(AUDIOUNIT_INCLUDE_PATH AudioUnit.h)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_path(COREAUDIO_INCLUDE_PATH CoreAudio.h)
    find_library(COREAUDIO_LIBRARY CoreAudio)
endif()
if(USE_FLTK OR BUILD_VIRTUAL_KEYBOARD)

  # first, try to use fltk-config for fltk >= 1.3 (FindFLTK is buggy
  # on Unix, where e.g. xft and xinerama options are not dealt with)
  find_program(FLTK_CONFIG_SCRIPT fltk-config)
  if(FLTK_CONFIG_SCRIPT)
    execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --api-version
                    OUTPUT_VARIABLE FLTK_VERSION)
    string(STRIP ${FLTK_VERSION} FLTK_VERSION)
    if(FLTK_VERSION GREATER 1.1)
      set(HAVE_FLTK "Fltk")
      set(FLTK_FOUND 1)
      message(STATUS "Using fltk-config script for Fltk " ${FLTK_VERSION})
      execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --includedir
                      OUTPUT_VARIABLE FLTK_INCLUDE_DIR)
      string(STRIP ${FLTK_INCLUDE_DIR} FLTK_INCLUDE_DIR)
      execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --ldflags
                      OUTPUT_VARIABLE FLTK_LIBRARIES)
      string(STRIP ${FLTK_LIBRARIES} FLTK_LIBRARIES)
      string(REGEX MATCH "fltk[_ ]jpeg" FLTK_JPEG ${FLTK_LIBRARIES})
      string(REGEX MATCH "fltk[_ ]z" FLTK_Z ${FLTK_LIBRARIES})
      string(REGEX MATCH "fltk[_ ]png" FLTK_PNG ${FLTK_LIBRARIES})
    endif(FLTK_VERSION GREATER 1.1)
  endif(FLTK_CONFIG_SCRIPT)
  if(NOT HAVE_FLTK) 

    set(FLTK_SKIP_OPENGL true)
    find_package(FLTK)
  endif()
endif()

# BUILD TARGETS

# check_deps(USE_COREAUDIO APPLE COREAUDIO_INCLUDE_PATH COREAUDIO_LIBRARY)
# if(USE_COREAUDIO)
#    make_plugin(rtcoreaudio rtcoreaudio.c)
#    include_directories(${COREAUDIO_INCLUDE_PATH})
#    target_link_libraries(rtcoreaudio ${COREAUDIO_LIBRARY})
# endif()

check_deps(USE_COREMIDI APPLE COREMIDI_INCLUDE_PATH COREMIDI_LIBRARY COREFOUNDATION_LIBRARY)
if(USE_COREMIDI)
    make_plugin(cmidi cmidi.c)
    include_directories(${COREMIDI_INCLUDE_PATH})
    target_link_libraries(cmidi ${COREMIDI_LIBRARY} ${COREFOUNDATION_LIBRARY})
endif()

check_deps(USE_AUDIOUNIT APPLE COREAUDIO_INCLUDE_PATH COREAUDIO_LIBRARY AUDIOUNIT_INCLUDE_PATH AUDIOUNIT_LIBRARY COREFOUNDATION_LIBRARY)
if(USE_AUDIOUNIT)
    make_plugin(rtauhal rtauhal.c)
    include_directories(${AUDIOUNIT_INCLUDE_PATH})
    target_link_libraries(rtauhal ${AUDIOUNIT_LIBRARY} ${COREFOUNDATION_LIBRARY} ${COREAUDIO_LIBRARY})
endif()

check_deps(USE_ALSA ALSA_HEADER ALSA_LIBRARY PTHREAD_LIBRARY)
if(USE_ALSA) 
    set(rtalsa_LIBS 
        ${ALSA_LIBRARY} ${PTHREAD_LIBRARY} m)
    make_plugin(rtalsa rtalsa.c ${rtalsa_LIBS})
endif()

if(WIN32)
    message(STATUS "Building Windows MME plugin(rtwinmm).")
    set(rtwinmm_LIBS
        winmm gdi32 kernel32) # should probably do checks for these libs
    make_plugin(rtwinmm rtwinmm.c "${rtwinmm_LIBS}")
endif()


check_deps(USE_PULSEAUDIO PULSEAUDIO_HEADER PULSEAUDIO_LIBRARY PULSESIMPLE_LIBRARY)
if(USE_PULSEAUDIO)
    make_plugin(rtpulse rtpulse.c ${PULSEAUDIO_LIBRARY} ${PULSESIMPLE_LIBRARY})
endif()

check_deps(USE_PORTAUDIO PORTAUDIO_HEADER PORTAUDIO_LIBRARY)
if(USE_PORTAUDIO)
    set(rtpa_LIBS ${PORTAUDIO_LIBRARY})
    
    if(WIN32)
        list(APPEND rtpa_LIBS winmm dsound ${CSOUND_WINDOWS_LIBRARIES})
    endif()

    make_plugin(rtpa rtpa.c ${rtpa_LIBS})
endif()

check_deps(USE_PORTMIDI PORTMIDI_HEADER PORTMIDI_LIBRARY)
if(USE_PORTMIDI)
    set(pmidi_LIBS ${PORTMIDI_LIBRARY})
    #if(NOT APPLE)
    #    list(APPEND pmidi_LIBS ${PORTTIME_LIBRARY})
    #endif()
    if(WIN32)
        list(APPEND pmidi_LIBS ${CSOUND_WINDOWS_LIBRARIES})
    endif()
    if(LINUX AND ALSA_LIBRARY)
        list(APPEND pmidi_LIBS ${ALSA_LIBRARY})
    endif()
    make_plugin(pmidi pmidi.c "${pmidi_LIBS}")
endif()

if(USE_IPMIDI)
    set(pmidi_LIBS )
    #if(NOT APPLE)
    #    list(APPEND pmidi_LIBS ${PORTTIME_LIBRARY})
    #endif()
    if(WIN32)
        list(APPEND pmidi_LIBS LIBS ${CSOUND_WINDOWS_LIBRARIES})
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
        list(APPEND rtjack_LIBS
            ${JACKDMP_LIBRARY} ${PTHREAD_LIBRARY})
    else()
        list(APPEND rtjack_LIBS
            ${JACK_LIBRARY} ${PTHREAD_LIBRARY})
    endif()
    make_plugin(rtjack rtjack.c "${rtjack_LIBS}")
endif()

check_deps(USE_FLTK FLTK_FOUND)
if(USE_FLTK)
    set(widgets_SRCS FL_graph.cpp winFLTK.c widgets.cpp)
    make_plugin(widgets "${widgets_SRCS}" "${FLTK_LIBRARIES}")
endif()

check_deps(BUILD_VIRTUAL_KEYBOARD FLTK_FOUND)
if(BUILD_VIRTUAL_KEYBOARD)
    set(virtual_SRCS virtual_keyboard/FLTKKeyboard.cpp
        virtual_keyboard/FLTKKeyboardWindow.cpp
        virtual_keyboard/FLTKKeyboardWidget.cpp
        virtual_keyboard/virtual_keyboard.cpp
        virtual_keyboard/Bank.cpp
        virtual_keyboard/KeyboardMapping.cpp
        virtual_keyboard/Program.cpp
        virtual_keyboard/SliderBank.cpp
        virtual_keyboard/SliderData.cpp)
    make_plugin(virtual "${virtual_SRCS}" "${FLTK_LIBRARIES}")
    include_directories(".")
    include_directories(${FLTK_INCLUDE_DIR})
endif()

message(STATUS "")
