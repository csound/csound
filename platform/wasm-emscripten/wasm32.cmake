include($ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_LINKER_FLAGS "-fPIC")
