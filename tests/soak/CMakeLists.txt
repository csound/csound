cmake_minimum_required(VERSION 2.8)

if(USE_COMPILER_OPTIMIZATIONS)
  add_custom_target(soak ALL
    ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiler optimizations must be turned off when running the soak tests. Run cmake with -DUSE_COMPILER_OPTIMIZATIONS=0 flag")
else()
  # runner.sh has 2 positional arguments: (csound_bin, opcode6dir64)
  add_custom_target(soak sh runner.sh ${CMAKE_BINARY_DIR}/csound ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
