option(BUILD_BELA "Build Bela main program" OFF)

if(BUILD_BELA)
  message(STATUS "Building belacsound program.")
  execute_process(COMMAND /usr/xenomai/bin/xeno-config --skin=posix --cflags OUTPUT_VARIABLE XENOMAI_POSIX_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND /usr/xenomai/bin/xeno-config --skin=posix --ldflags OUTPUT_VARIABLE XENOMAI_POSIX_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  make_executable(belacsound ./main.cpp ${CSOUNDLIB})
  set(MAIN_FLAGS "-std=c++11 ${XENOMAI_POSIX_CFLAGS}")
  set_source_files_properties(./main.cpp PROPERTIES COMPILE_FLAGS ${MAIN_FLAGS})
  target_link_libraries(belacsound ${CSOUNDLIB} -L$ENV{HOME}/Bela/lib -L/root/Bela/lib
    -fno-pie -no-pie ${XENOMAI_POSIX_LDFLAGS} -lbela
    -lbelaextra -lNE10)
  target_include_directories(belacsound PRIVATE /usr/xenomai/include
    /usr/xenomai/include/posix
    /usr/local/include $ENV{HOME}/Bela/include /root/Bela/lib)
else()
  message(STATUS "Not building belacsound program.")
endif()
