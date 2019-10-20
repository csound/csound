cmake_minimum_required(VERSION 2.8)

# runner.sh has 2 positional arguments: (csound_bin, opcode6dir64)
add_custom_target(soak sh runner.sh ${CMAKE_BINARY_DIR}/csound ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
