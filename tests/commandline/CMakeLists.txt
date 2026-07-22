# CMakeLists.txt - CSD Tests Configuration
#
# This file configures CSD (Csound Document) tests for different platforms.
#
# Usage:
#   make csdtests         - Run CSD tests normally (non-verbose)
#   make csdtests-verbose - Run CSD tests with verbose output

find_package(Python3 COMPONENTS Interpreter QUIET)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/test_realtime_compile_opcodes.orc
  ${CMAKE_CURRENT_BINARY_DIR}/test_realtime_compile_opcodes.orc
  COPYONLY
)

if(Python3_Interpreter_FOUND)
  # Build the common test arguments
  set(COMMON_TEST_ARGS --csound-executable=${CMAKE_BINARY_DIR}/csound --source-dir=${CMAKE_CURRENT_SOURCE_DIR})

  # Platform-specific csdtests targets
  if (MINGW)
    # For MinGW with Wine runtime
    list(APPEND PLATFORM_ARGS --runtime-executable=wine --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
      VERBATIM
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --runtime-executable=wine
      --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
    )
  elseif (EMSCRIPTEN)
    # For Emscripten with Node.js runtime
    list(APPEND PLATFORM_ARGS --runtime-executable=${EMSDK_NODE} --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
      VERBATIM
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --runtime-executable=${EMSDK_NODE}
      --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
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
      VERBATIM
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CSOUND_EXECUTABLE_PATH}
      --opcode7dir64=$<TARGET_FILE_DIR:csound-bin>
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
    )
  else()
    # Default for Unix-like systems (Linux, macOS)
    list(APPEND PLATFORM_ARGS --opcode7dir64=${CMAKE_BINARY_DIR})
    add_custom_target(csdtests
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      ${COMMON_TEST_ARGS}
      ${PLATFORM_ARGS}
      VERBATIM
    )
    # Verbose variant target
    add_custom_target(csdtests-verbose
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --verbose
      --csound-executable=${CMAKE_BINARY_DIR}/csound
      --opcode7dir64=${CMAKE_BINARY_DIR}
      --source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
    )
  endif()

  add_custom_target(csdtest-harness-tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_harness.py
  )
  add_dependencies(csdtests csdtest-harness-tests)
  add_dependencies(csdtests-verbose csdtest-harness-tests)
else()
  message(STATUS "Python3 not found; not creating commandline test targets.")
endif()
