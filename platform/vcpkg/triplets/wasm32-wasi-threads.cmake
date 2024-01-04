#set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH)

if(NOT DEFINED ENV{WASI_SDK_PATH})
   find_path(WASI_SDK_PATH "wasm-ld")
else()
   set(WASI_SDK_PATH "$ENV{WASI_SDK_PATH}")
endif()

if(NOT EXISTS "${WASI_SDK_PATH}/share/cmake/wasi-sdk-pthread.cmake")
   message(FATAL_ERROR "wasi-sdk-pthread.cmake toolchain file not found")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME WASI)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${WASI_SDK_PATH}/share/cmake/wasi-sdk-pthread.cmake")
