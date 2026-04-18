# CMakeLists.txt - CSD Tests Configuration
#
# This file configures CSD (Csound Document) tests for different platforms.
#
# Usage:
#   make csdtests         - Run CSD tests normally (non-verbose)
#   make csdtests-verbose - Run CSD tests with verbose output

find_package(Python3 COMPONENTS Interpreter QUIET)

if(Python3_Interpreter_FOUND)
  # Build the common test arguments
  set(COMMON_TEST_ARGS --csound-executable=${CMAKE_BINARY_DIR}/csound --source-dir=${CMAKE_CURRENT_SOURCE_DIR})

  # Platform-specific csdtests targets
  if (MINGW)
    # For MinGW with Wine runtime
    list(APPEND PLATFORM_ARGS --runtime-environment=wine --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --runtime-environment=wine
      --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
    )
  elseif (EMSCRIPTEN)
    # For Emscripten with Node.js runtime
    list(APPEND PLATFORM_ARGS --runtime-environment=${EMSDK_NODE} --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --runtime-environment=${EMSDK_NODE}
      --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
    )
  elseif(MSVC)
    # For MSVC builds
    list(APPEND PLATFORM_ARGS --opcode7dir64=$<TARGET_FILE_DIR:csound-bin>)
    set(CSOUND_EXECUTABLE_PATH $<TARGET_FILE:csound-bin>)
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --csound-executable=${CSOUND_EXECUTABLE_PATH}
      ${PLATFORM_ARGS}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CSOUND_EXECUTABLE_PATH}
      --opcode7dir64=$<TARGET_FILE_DIR:csound-bin>
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
    )
  else()
    # Default for Unix-like systems (Linux, macOS)
    list(APPEND PLATFORM_ARGS --opcode7dir64=${CMAKE_BINARY_DIR})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --opcode7dir64=${CMAKE_BINARY_DIR}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
    )
  endif()
else()
  message(STATUS "Python3 not found; not creating commandline test targets.")
endif()


