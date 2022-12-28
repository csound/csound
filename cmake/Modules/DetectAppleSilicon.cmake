# on m1 processors with rosetta installed
# the variable CMAKE_SYSTEM_PROCESSOR will
# show x86_64 when the host system is really arm64

if(APPLE)
  execute_process(COMMAND sysctl -q hw.optional.arm64
    OUTPUT_VARIABLE _sysctl_stdout
    ERROR_VARIABLE _sysctl_stderr
    RESULT_VARIABLE _sysctl_result
  )
  if(_sysctl_result EQUAL 0 AND _sysctl_stdout MATCHES "hw.optional.arm64: 1")
    message(STATUS "APPLE SILICON PROCESSOR DETECTED")
    set(APPLE_SILICON "1")
  endif()
  unset(_sysctl_result)
  unset(_sysctl_stderr)
  unset(_sysctl_stdout)
endif()
