add_executable(wasi-pthread-smoke pthread-smoke.c)
target_compile_features(wasi-pthread-smoke PRIVATE c_std_11)
target_link_libraries(wasi-pthread-smoke PRIVATE Threads::Threads)

add_executable(wasi-realtime-smoke csound-smoke.c)
target_compile_features(wasi-realtime-smoke PRIVATE c_std_11)
target_compile_options(wasi-realtime-smoke PRIVATE ${libcsound_CFLAGS})
target_include_directories(wasi-realtime-smoke PRIVATE
    ${libcsound_private_include_dirs} ${libcsound_public_include_dirs})
target_link_libraries(wasi-realtime-smoke PRIVATE ${CSOUNDLIB} ${libcsound_LIBS})
