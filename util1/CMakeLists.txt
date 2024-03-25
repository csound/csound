# Extra utilities

if(BUILD_UTILITIES)
    make_utility(scsort sortex/smain.c)
    make_utility(extract sortex/xmain.c)

    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG OR MSVC)
        make_utility(cs csd_util/cs.c)

        if(HAVE_UNISTD_H)
            target_compile_definitions(cs PRIVATE "-DHAVE_UNISTD_H")
        endif()

        set(csb64enc_SRCS csd_util/base64.c csd_util/csb64enc.c)
        make_utility(csb64enc "${csb64enc_SRCS}")

        set(makecsd_SRCS csd_util/base64.c csd_util/makecsd.c)
        make_utility(makecsd "${makecsd_SRCS}")

        set(scot_SRCS scot/scot_main.c scot/scot.c)
        make_utility(scot "${scot_SRCS}")
    endif()
endif()
