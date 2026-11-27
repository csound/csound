# Loaded through CMAKE_PROJECT_TOP_LEVEL_INCLUDES by the portable Linux build
# in csound_builds.yml, i.e. immediately after Csound's top-level project()
# call. PortAudio and PortMidi are linked statically, so the plugins that use
# them must also link the system ALSA/JACK libraries explicitly. Defer target
# configuration until the I/O plugin subdirectory has added its targets.
function(configure_portable_audio_plugins)
  if(TARGET rtpa)
    target_link_libraries(rtpa PRIVATE asound jack)
  endif()
  if(TARGET pmidi)
    target_link_libraries(pmidi PRIVATE asound)
  endif()
endfunction()

cmake_language(DEFER CALL configure_portable_audio_plugins)
