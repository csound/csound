# Build these only when requested. Timing results are not pass/fail CI checks.
add_executable(poscil_benchmark EXCLUDE_FROM_ALL poscil.c)
target_link_libraries(poscil_benchmark PRIVATE ${CSOUNDLIB})
add_executable(optimizer_timeline EXCLUDE_FROM_ALL optimizer_timeline.c)
target_link_libraries(optimizer_timeline PRIVATE ${CSOUNDLIB} ${CMAKE_DL_LIBS})
if(WIN32)
    target_compile_definitions(optimizer_timeline PRIVATE
        CSOUND_BENCH_DLL="$<TARGET_FILE_NAME:${CSOUNDLIB}>")
endif()

add_executable(scalar_cse_bench EXCLUDE_FROM_ALL scalar_cse_bench.c)
target_link_libraries(scalar_cse_bench PRIVATE ${CSOUNDLIB} ${CMAKE_DL_LIBS})
if(WIN32)
  target_compile_definitions(scalar_cse_bench PRIVATE
    CSOUND_CSE_BENCH_DLL="$<TARGET_FILE_NAME:${CSOUNDLIB}>")
endif()
