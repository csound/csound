#  PORTAUDIO_FOUND - system has libportaudio
#  PORTAUDIO_INCLUDE_DIRS - the libportaudio include directory
#  PORTAUDIO_LIBRARIES - Link these to use libportaudio

if(PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
  # in cache already
  set(PORTAUDIO_FOUND TRUE)
else()
  find_path(PORTAUDIO_INCLUDE_DIR
    NAMES
      portaudio.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )
  
  find_library(PORTAUDIO_LIBRARY
    NAMES
      portaudio portaudio_x64
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(PORTAUDIO_INCLUDE_DIRS
    ${PORTAUDIO_INCLUDE_DIR}
  )

  set(PORTAUDIO_LIBRARIES
    ${PORTAUDIO_LIBRARY}
  )

  if(PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)
    set(PORTAUDIO_FOUND TRUE)
  endif()

  if(PORTAUDIO_FOUND)
    if (NOT Portaudio_FIND_QUIETLY)
      message(STATUS "Found Portaudio: ${PORTAUDIO_LIBRARIES}")
    endif()
  else()
    if (Portaudio_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Portaudio")
    endif()
  endif()

  # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
  mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)

endif()