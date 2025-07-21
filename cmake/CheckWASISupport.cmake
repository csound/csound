function(detect_wasi_support OUT_VAR)
  message(STATUS "Checking for WASI support...")
  message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
  message(STATUS "CMAKE_SYSROOT: ${CMAKE_SYSROOT}")

  # First check if the compiler supports wasm32 target
  execute_process(
      COMMAND ${CMAKE_C_COMPILER} --print-targets
      RESULT_VARIABLE rv
      OUTPUT_VARIABLE compiler_targets
      ERROR_QUIET)

  if (rv EQUAL 0 AND compiler_targets MATCHES "wasm32")
    set(_has_wasm_backend TRUE)
    message(STATUS "Compiler supports wasm32 target")
  else()
    set(_has_wasm_backend FALSE)
    message(STATUS "Compiler does not support wasm32 target")
  endif()

  # Check if we can compile for the WASI triple
  if(_has_wasm_backend)
    # Create a simple test file
    set(TEST_SOURCE "${CMAKE_BINARY_DIR}/test_wasi.c")
    file(WRITE ${TEST_SOURCE} "
#if !(defined(__wasm__) || defined(__wasi__))
#  error no wasi
#endif
int main(void) { return 0; }
")

    # Build the compile command
    set(COMPILE_COMMAND "${CMAKE_C_COMPILER}")
    list(APPEND COMPILE_COMMAND "--target=wasm32-wasi")
    if(CMAKE_SYSROOT)
      list(APPEND COMPILE_COMMAND "--sysroot=${CMAKE_SYSROOT}")
    endif()
    list(APPEND COMPILE_COMMAND "-c")
    list(APPEND COMPILE_COMMAND "${TEST_SOURCE}")
    list(APPEND COMPILE_COMMAND "-o")
    list(APPEND COMPILE_COMMAND "${CMAKE_BINARY_DIR}/test_wasi.o")

    # Try to compile
    execute_process(
        COMMAND ${COMPILE_COMMAND}
        RESULT_VARIABLE compile_result
        OUTPUT_VARIABLE compile_output
        ERROR_VARIABLE compile_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)

    if (compile_result EQUAL 0)
      set(_HAS_WASI TRUE)
      message(STATUS "WASI compilation test passed")
    else()
      set(_HAS_WASI FALSE)
      message(STATUS "WASI compilation test failed")
      message(STATUS "Compile command: ${COMPILE_COMMAND}")
      message(STATUS "Error output: ${compile_error}")
    endif()

    # Clean up
    file(REMOVE ${TEST_SOURCE})
    file(REMOVE "${CMAKE_BINARY_DIR}/test_wasi.o")
  endif()

  if (_has_wasm_backend AND _HAS_WASI)
    message(STATUS "WASI detected successfully")
    set(${OUT_VAR} TRUE PARENT_SCOPE)
  else()
    message(STATUS "WASI *not* detected - install wasi-sdk or a Clang with wasm32 support")
    set(${OUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Function to configure WASI toolchain settings
function(configure_wasi_toolchain)
  if(NOT CMAKE_C_COMPILER_TARGET)
    set(CMAKE_C_COMPILER_TARGET wasm32-wasi PARENT_SCOPE)
  endif()
  if(NOT CMAKE_CXX_COMPILER_TARGET)
    set(CMAKE_CXX_COMPILER_TARGET wasm32-wasi PARENT_SCOPE)
  endif()

  # Set system information
  set(CMAKE_SYSTEM_NAME WASI PARENT_SCOPE)
  set(CMAKE_SYSTEM_VERSION 1 PARENT_SCOPE)
  set(CMAKE_SYSTEM_PROCESSOR wasm32 PARENT_SCOPE)

  # WASI doesn't support shared libraries
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "WASI doesn't support shared libraries" FORCE)

  # Set sysroot if WASI_SDK_PREFIX is defined
  if(DEFINED ENV{WASI_SDK_PREFIX})
    set(CMAKE_SYSROOT "$ENV{WASI_SDK_PREFIX}/share/wasi-sysroot" PARENT_SCOPE)
    set(CMAKE_FIND_ROOT_PATH "$ENV{WASI_SDK_PREFIX}/share/wasi-sysroot" PARENT_SCOPE)
  endif()

  # Compiler flags for WASI
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions" PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti" PARENT_SCOPE)

  # Don't look for programs in the host system
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER PARENT_SCOPE)
  # Only look for libraries and includes in the sysroot
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY PARENT_SCOPE)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY PARENT_SCOPE)

  # Disable features not available in WASI
  set(CMAKE_SKIP_RPATH TRUE PARENT_SCOPE)
endfunction()
