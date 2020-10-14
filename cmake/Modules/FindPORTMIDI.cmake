#  PORTMIDI_FOUND - system has libportmidi
#  PORTMIDI_INCLUDE_DIRS - the libportmidi include directory
#  PORTMIDI_LIBRARIES - Link these to use libportmidi

if (PORTMIDI_LIBRARIES AND PORTMIDI_INCLUDE_DIRS)
  # in cache already
  set(PORTMIDI_FOUND TRUE)
else ()
  find_path(PORTMIDI_INCLUDE_DIR
    NAMES
      portmidi.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(PORTMIDI_LIBRARY
    NAMES
      portmidi
      portmidi_s
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(PORTMIDI_INCLUDE_DIRS ${PORTMIDI_INCLUDE_DIR})
  set(PORTMIDI_LIBRARIES ${PORTMIDI_LIBRARY})

  if (PORTMIDI_INCLUDE_DIRS AND PORTMIDI_LIBRARIES)
    set(PORTMIDI_FOUND TRUE)
  endif ()

  if (PORTMIDI_FOUND)
    if (NOT Portmidi_FIND_QUIETLY)
      message(STATUS "Found PortMidi: ${PORTMIDI_LIBRARIES}")
    endif ()
  else ()
    if (Portmidi_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PortMidi")
    endif ()
  endif()

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)
endif()
