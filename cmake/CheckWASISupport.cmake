# Check if the compiler supports WASI target
function(check_wasi_support RESULT_VAR)
    # First, check if WASI_SDK_PATH is set in the environment
    if(DEFINED ENV{WASI_SDK_PATH})
        set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})
        if(EXISTS "${WASI_SDK_PATH}/bin/clang")
            message(STATUS "Found WASI SDK at ${WASI_SDK_PATH}")
            set(${RESULT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
    endif()

    # Check if clang supports wasm32-wasi target
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} --target=wasm32-wasi -dM -E - < /dev/null
        RESULT_VARIABLE CLANG_WASI_RESULT
        OUTPUT_VARIABLE CLANG_WASI_OUTPUT
        ERROR_QUIET
    )

    if(CLANG_WASI_RESULT EQUAL 0 AND CLANG_WASI_OUTPUT MATCHES "__wasm__")
        message(STATUS "WASI support detected in compiler (${CMAKE_C_COMPILER})")
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
        return()
    endif()

    # Fallback to try_compile method
    set(WASI_TEST_SOURCE "
        #include <stdlib.h>
        #include <stdio.h>
        int main() {
        #if defined(__wasm__) || defined(__wasi__)
            printf(\"WASI is supported\\n\");
            return 0;
        #else
            #error WASI is not supported
        #endif
        }
    ")

    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    file(WRITE "${CMAKE_BINARY_DIR}/wasi_test.c" "${WASI_TEST_SOURCE}")

    try_compile(
        WASI_SUPPORTED
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/wasi_test.c
        CMAKE_FLAGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/wasm32-wasi.cmake"
        OUTPUT_VARIABLE WASI_OUTPUT
    )

    if(WASI_SUPPORTED)
        message(STATUS "WASI support detected in compiler via try_compile")
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    else()
        message(STATUS "WASI support not detected in compiler")
        message(STATUS "To enable WASI support, install the WASI SDK and set WASI_SDK_PATH")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()