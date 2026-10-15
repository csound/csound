include(CheckCSourceCompiles)
include(CMakePushCheckState)

# Probe before enabling pthreads in the target. A normal unthreaded sysroot
# must configure successfully and keep the synchronous engine.
set(wasi_realtime_probe_file "${CMAKE_CURRENT_LIST_DIR}/check_wasi_realtime.c")
function(check_wasi_realtime_runtime)
  cmake_push_check_state(RESET)
  # A compile-only toolchain test would accept wasi-libc's pthread stubs. Link
  # the executable even when the cross toolchain normally probes static libraries.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
  set(CMAKE_REQUIRED_FLAGS "-pthread")
  set(CMAKE_REQUIRED_LINK_OPTIONS "-pthread;-Wl,--shared-memory,--max-memory=268435456")

  # A pthread header or an empty libpthread archive does not supply a thread host.
  # Link real entry points, and reject a compiler configured without Wasm atomics.
  file(READ "${wasi_realtime_probe_file}"
       wasi_realtime_source)
  check_c_source_compiles("${wasi_realtime_source}" HAVE_WASI_REALTIME_RUNTIME)
  cmake_pop_check_state()
endfunction()
check_wasi_realtime_runtime()
