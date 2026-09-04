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

  if(HAVE_WASMTIME AND TARGET csound-bin AND USE_DOUBLE)
    set(_wasm_test_compiler "${CSOUND_WASM_TEST_C_COMPILER}")
    set(_wasm_test_sysroot "${CSOUND_WASM_TEST_SYSROOT}")
    if(NOT _wasm_test_compiler AND
       DEFINED ENV{WASI_SDK_PATH} AND
       EXISTS "$ENV{WASI_SDK_PATH}/bin/clang")
      set(_wasm_test_compiler "$ENV{WASI_SDK_PATH}/bin/clang")
      if(NOT _wasm_test_sysroot)
        set(_wasm_test_sysroot "$ENV{WASI_SDK_PATH}/share/wasi-sysroot")
      endif()
    endif()
    if(NOT _wasm_test_compiler)
      find_program(_wasm_test_program
        NAMES wasm32-wasi-clang wasm32-unknown-wasi-clang
      )
      set(_wasm_test_compiler "${_wasm_test_program}")
    endif()

    set(CSOUND_WASM_TEST_C_COMPILER "${_wasm_test_compiler}" CACHE FILEPATH
      "WASI C compiler used to build the Wasmtime test plugin" FORCE)
    set(CSOUND_WASM_TEST_SYSROOT "${_wasm_test_sysroot}" CACHE PATH
      "WASI sysroot used to build the Wasmtime test plugin" FORCE)

    if(CSOUND_WASM_TEST_C_COMPILER)
      set(_wasm_test_plugin
        "${CMAKE_CURRENT_BINARY_DIR}/wasmtime/velvetlp.wasm")
      set(_wasm_fixture_args
        "--compiler=${CSOUND_WASM_TEST_C_COMPILER}"
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
        "WASI C compiler not found; Wasmtime opcode tests are disabled")
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
