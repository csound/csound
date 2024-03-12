set(CS_MAIN_SRCS csound_main.c)

if(LINUX)
    list(APPEND CS_MAIN_SRCS sched.c)
endif()

make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB};${MATH_LIBRARY}" csound)

if(LINUX)
    find_package(Threads)
    target_link_libraries(csound-bin PRIVATE Threads::Threads)
endif()