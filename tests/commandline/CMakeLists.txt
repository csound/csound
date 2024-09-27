cmake_minimum_required(VERSION 3.5)

if (MINGW)
  add_custom_target(csdtests ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --runtime-environment=wine --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
elseif (EMSCRIPTEN)
  add_custom_target(csdtests ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --runtime-environment=${EMSDK_NODE} --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode6dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
elseif(MSVC)
  add_custom_target(csdtests ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --csound-executable=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/csound --opcode7dir64=${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
else()
  add_custom_target(csdtests ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py --csound-executable=${CMAKE_BINARY_DIR}/csound --opcode7dir64=${CMAKE_BINARY_DIR} --source-dir=${CMAKE_CURRENT_SOURCE_DIR})
endif()

