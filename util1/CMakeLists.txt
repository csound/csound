#extra utilities

make_utility(scsort      sortex/smain.c)
make_utility(extract     sortex/xmain.c)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    if(HAVE_UNISTD_H)
      add_definitions("-DHAVE_UNISTD_H")
    endif()
    make_utility(cs         csd_util/cs.c)
    
    set(csb64enc_SRCS 
        csd_util/base64.c csd_util/csb64enc.c)
    make_utility(csb64enc "${csb64enc_SRCS}")
    
    set(makecsd_SRCS 
        csd_util/base64.c csd_util/makecsd.c)
    make_utility(makecsd "${makecsd_SRCS}")
    
    set(scot_SRCS 
        scot/scot_main.c scot/scot.c)
    make_utility(scot "${scot_SRCS}")
endif()

