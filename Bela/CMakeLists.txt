option(BUILD_BELA "Build Bela main program" OFF)

if(BUILD_BELA)
  message(STATUS "Building belacsound program.")
  make_executable(belacsound ./main.cpp ${CSOUNDLIB})
  target_link_libraries(belacsound ${CSOUNDLIB} -L$ENV{HOME}/Bela/lib -L/root/Bela/lib
    -Wl,@/usr/xenomai/lib/posix.wrappers -L/usr/xenomai/lib 
    -lpthread_rt -lxenomai -lpthread -lrt -lbela
    -lbelaextra -lNE10)
  target_include_directories(belacsound PRIVATE /usr/xenomai/include
    /usr/xenomai/include/posix
    /usr/local/include $ENV{HOME}/Bela/include /root/Bela/lib)
  set_target_properties(belacsound  PROPERTIES COMPILE_FLAGS
    "-std=c++11 -D_GNU_SOURCE -D_REENTRANT -D__XENO__")
else()
  message(STATUS "Not building belacsound program.")
endif()
