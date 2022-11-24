
check_c_compiler_flag(-ftree-vectorize HAS_TREE_VECTORIZE)
check_cxx_compiler_flag(-ftree-vectorize HAS_CXX_TREE_VECTORIZE)
if (HAS_TREE_VECTORISE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
endif()
if (HAS_CXX_TREE_VECTORISE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
endif()


check_c_compiler_flag(-ffast-math HAS_FAST_MATH)
check_cxx_compiler_flag(-ffast-math HAS_CXX_FAST_MATH)
if (HAS_FAST_MATH AND NOT MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
endif()
if (HAS_CXX_FAST_MATH AND NOT MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()



if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
check_c_compiler_flag(-mfpmath=sse HAS_FPMATH_SSE)
check_cxx_compiler_flag(-mfpmath=sse HAS_CXX_FPMATH_SSE)
  if (HAS_FPMATH_SSE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
endif()
if (HAS_CXX_FPMATH_SSE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
endif()

endif()


check_c_compiler_flag(-msse2 HAS_SSE2)
check_cxx_compiler_flag(-msse2 HAS_CXX_SSE2)
if (HAS_SSE2 AND NOT IOS AND NOT WASM AND NOT APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
endif()
if (HAS_CXX_SSE2 AND NOT IOS AND NOT WASM AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
endif()


check_c_compiler_flag(-fomit-frame-pointer HAS_OMIT_FRAME_POINTER)
check_cxx_compiler_flag(-fomit-frame-pointer HAS_CXX_OMIT_FRAME_POINTER)
if (HAS_OMIT_FRAME_POINTER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
endif()
if (HAS_CXX_OMIT_FRAME_POINTER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
endif()
