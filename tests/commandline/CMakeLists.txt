cmake_minimum_required(VERSION 2.8)

if(MSVC) 
  add_custom_target(csdtests python test.py --csound-executable=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/csound --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else()
  add_custom_target(csdtests python test.py --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode6dir64=${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

