# Based on Aman Jagwani's ESP32-P4 and ESP32-S3 build settings.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
     CSOUND_ESP32_TARGET CSOUND_ESP32_GCC)

if(CSOUND_ESP32_TARGET STREQUAL "esp32p4")
    set(CMAKE_SYSTEM_PROCESSOR riscv32)
    set(_esp_compiler riscv32-esp-elf-gcc)
    set(_esp_arch_flags "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f")
elseif(CSOUND_ESP32_TARGET STREQUAL "esp32s3")
    set(CMAKE_SYSTEM_PROCESSOR xtensa)
    set(_esp_compiler xtensa-esp32s3-elf-gcc)
    set(_esp_arch_flags "-mlongcalls")
else()
    message(FATAL_ERROR "Set CSOUND_ESP32_TARGET to esp32p4 or esp32s3")
endif()

find_program(CSOUND_ESP32_GCC NAMES ${_esp_compiler})
if(NOT CSOUND_ESP32_GCC OR NOT EXISTS "${CSOUND_ESP32_GCC}")
    message(FATAL_ERROR
        "${_esp_compiler} not found. Activate the ESP-IDF environment or set "
        "CSOUND_ESP32_GCC to its full path.")
endif()
get_filename_component(_esp_bin "${CSOUND_ESP32_GCC}" DIRECTORY)
get_filename_component(_esp_root "${_esp_bin}" DIRECTORY)
get_filename_component(_esp_gcc_name "${CSOUND_ESP32_GCC}" NAME)
if(NOT _esp_gcc_name MATCHES "^${_esp_compiler}(\\.exe)?$")
    message(FATAL_ERROR "${CSOUND_ESP32_TARGET} requires ${_esp_compiler}")
endif()
string(REGEX REPLACE "gcc(\\.exe)?$" "" _esp_prefix "${CSOUND_ESP32_GCC}")
set(CMAKE_C_COMPILER "${CSOUND_ESP32_GCC}")
set(CMAKE_CXX_COMPILER "${_esp_prefix}g++")
set(CMAKE_ASM_COMPILER "${CSOUND_ESP32_GCC}")
set(CMAKE_AR "${_esp_prefix}ar")
set(CMAKE_RANLIB "${_esp_prefix}ranlib")

set(CMAKE_FIND_ROOT_PATH "${_esp_root}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# INIT variables keep user-supplied flags and avoid adding flags on each configure.
set(_esp_flags "${_esp_arch_flags} --specs=picolibc.specs -fno-builtin -fno-exceptions -ffunction-sections -fdata-sections -fomit-frame-pointer -Wno-attributes -Wno-strict-aliasing")
set(CMAKE_C_FLAGS_INIT "${_esp_flags} -DNO_SERIAL_OPCODES")
set(CMAKE_CXX_FLAGS_INIT "${_esp_flags} -fno-rtti -Wno-register")
set(CMAKE_ASM_FLAGS_INIT "${_esp_flags} -x assembler-with-cpp")
