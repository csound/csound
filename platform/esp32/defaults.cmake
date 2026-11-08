# Defaults for a static library linked into an ESP32 firmware project.
# Explicit -D options still take precedence.
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set(BARE_METAL ON CACHE BOOL "Build for bare metal")
set(USE_DOUBLE OFF CACHE BOOL "Use double-precision audio samples")
set(USE_FLOAT ON CACHE BOOL "Use single precision for cs_float and cs_double")

foreach(_esp_option
        BUILD_PLUGINS BUILD_TESTS BUILD_DOCS BUILD_UTILITIES BUILD_CSBEATS
        BUILD_CSOUND_COMMAND BUILD_PERFTHREAD_CLASS BUILD_MULTI_CORE
        BUILD_OSC_OPCODES BUILD_DEPRECATED_OPCODES INSTALL_PYTHON_INTERFACE
        REQUIRE_PTHREADS USE_LIBSNDFILE USE_LIBSAMPLERATE USE_GETTEXT USE_CURL
        USE_DEFAULT_OPCODEDIR USE_ALSA USE_JACK USE_IPMIDI USE_PORTMIDI
        USE_PORTAUDIO USE_PULSEAUDIO USE_PIPEWIRE USE_COREMIDI USE_AUDIOUNIT)
    set(${_esp_option} OFF CACHE BOOL "Disabled by default for ESP32")
endforeach()
