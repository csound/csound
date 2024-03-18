cmake_minimum_required(VERSION 3.5)

add_test(NAME regression COMMAND ${Python3_EXECUTABLE} ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode6dir64=${CMAKE_BINARY_DIR} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
