# Build these only when requested. Timing results are not pass/fail CI checks.
add_executable(poscil_benchmark EXCLUDE_FROM_ALL poscil.c)
target_link_libraries(poscil_benchmark PRIVATE ${CSOUNDLIB})
