cmake_minimum_required(VERSION 3.5)

if (MINGW)
  add_test(NAME csdtests COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --use-wine=true --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
elseif(MSVC)
  add_test(NAME csdtests COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --csound-executable=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/csound --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
else()
  add_test(NAME csdtests COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode6dir64=${CMAKE_BINARY_DIR} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
endif()

