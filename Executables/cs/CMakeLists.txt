make_utility(cs cs.c)
if(HAVE_UNISTD_H)
    target_compile_definitions(cs PRIVATE "HAVE_UNISTD_H")
endif()