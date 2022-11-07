option(BUILD_BELA "Build Bela main program" OFF)

if(BUILD_BELA)
  message(STATUS "Building belacsound program.")
  execute_process(COMMAND /usr/xenomai/bin/xeno-config --skin=posix --cflags OUTPUT_VARIABLE XENOMAI_POSIX_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND /usr/xenomai/bin/xeno-config --skin cobalt --ldflags --no-auto-init --no-mode-check OUTPUT_VARIABLE XENOMAI_POSIX_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT DEFINED BELA_HOME)
    set (BELA_HOME "$ENV{HOME}/Bela")
  endif(NOT DEFINED BELA_HOME)
  set (BELA_SRCS ./main.cpp ${BELA_HOME}/libraries/Trill/Trill.cpp)
  make_executable(belacsound "${BELA_SRCS}" ${CSOUNDLIB})
  set(MAIN_FLAGS "-std=c++11 ${XENOMAI_POSIX_CFLAGS}")
  set_source_files_properties(./main.cpp PROPERTIES COMPILE_FLAGS ${MAIN_FLAGS})
  target_link_libraries(belacsound ${CSOUNDLIB} -L${BELA_HOME}/lib
    -fno-pie -no-pie ${XENOMAI_POSIX_LDFLAGS} -lbela
    -lbelaextra)
  target_include_directories(belacsound PRIVATE /usr/xenomai/include
    /usr/xenomai/include/posix
    /usr/local/include ${BELA_HOME}/include ${BELA_HOME}/ )
else()
  message(STATUS "Not building belacsound program.")
endif()
