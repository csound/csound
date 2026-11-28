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
    VERBATIM
  )
  add_dependencies(csdtests csdtest-harness-tests)
  add_dependencies(csdtests-verbose csdtest-harness-tests)
  if(TARGET csound-bin)
    add_dependencies(csdtests csound-bin)
    add_dependencies(csdtests-verbose csound-bin)
  endif()

  if(HAVE_WASMTIME AND TARGET csound-bin AND USE_DOUBLE)
    find_program(CSOUND_WASM_TEST_C_COMPILER NAMES wasm32-wasip3-clang
      DOC "WASI Preview 3 C compiler used to build the Wasmtime test plugin")
    get_filename_component(_wasm_test_tool_dir "${CSOUND_WASM_TEST_C_COMPILER}" DIRECTORY)
    find_program(CSOUND_WASM_TEST_LINKER NAMES wasm-ld HINTS "${_wasm_test_tool_dir}"
      DOC "WebAssembly linker used to build the Wasmtime test plugin")
    set(CSOUND_WASM_TEST_SYSROOT "" CACHE PATH "Optional sysroot for the Wasmtime test plugin")

    if(CSOUND_WASM_TEST_C_COMPILER AND CSOUND_WASM_TEST_LINKER)
      set(_wasm_test_plugin
        "${CMAKE_CURRENT_BINARY_DIR}/wasmtime/velvetlp.wasm")
      set(_wasm_fixture_args
        "--compiler=${CSOUND_WASM_TEST_C_COMPILER}"
        "--linker=${CSOUND_WASM_TEST_LINKER}"
        "--source=${CMAKE_CURRENT_SOURCE_DIR}/wasmtime/velvetlp.wasm.fixture.c"
        "--output=${_wasm_test_plugin}"
        "--source-include=${PROJECT_SOURCE_DIR}/include"
        "--generated-include=${PROJECT_BINARY_DIR}/include"
        "--top-include=${PROJECT_SOURCE_DIR}/Top"
        --use-double
      )
      if(CSOUND_WASM_TEST_SYSROOT)
        list(APPEND _wasm_fixture_args
          "--sysroot=${CSOUND_WASM_TEST_SYSROOT}")
      endif()
      add_test(
        NAME commandline_wasmtime_opcode_fixture
        COMMAND
          ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/wasmtime/build_wasm_fixture.py
          ${_wasm_fixture_args}
      )
      set_tests_properties(commandline_wasmtime_opcode_fixture PROPERTIES
        FIXTURES_SETUP wasmtime_opcode_fixture
        LABELS "commandline;wasmtime"
        TIMEOUT 60
      )

      add_test(
        NAME commandline_wasmtime_opcode_cache
        COMMAND
          ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/wasmtime/test_wasmtime_opcode_cache.py
          --csound-executable=$<TARGET_FILE:csound-bin>
          --opcode7dir64=$<TARGET_FILE_DIR:csound-bin>
          --source-dir=${CMAKE_CURRENT_SOURCE_DIR}/wasmtime
          --plugin=${_wasm_test_plugin}
      )
      set_tests_properties(commandline_wasmtime_opcode_cache PROPERTIES
        FIXTURES_REQUIRED wasmtime_opcode_fixture
        LABELS "commandline;wasmtime"
        TIMEOUT 60
      )
    else()
      message(STATUS
        "WASI Preview 3 compiler or wasm-ld missing. Wasmtime opcode tests are disabled.")
    endif()

    unset(_wasm_fixture_args)
    unset(_wasm_test_compiler)
    unset(_wasm_test_plugin)
    unset(_wasm_test_program CACHE)
    unset(_wasm_test_sysroot)
  endif()
else()
  message(STATUS "Python3 not found; not creating commandline test targets.")
endif()
