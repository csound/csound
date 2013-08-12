include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)


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
if (HAS_FAST_MATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
endif()
if (HAS_CXX_FAST_MATH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()


check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)
if (HAS_VISIBILITY_HIDDEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
if (HAS_CXX_VISBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
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


check_c_compiler_flag(-fomit-frame-pointer HAS_OMIT_FRAME_POINTER)
check_cxx_compiler_flag(-fomit-frame-pointer HAS_CXX_OMIT_FRAME_POINTER)
if (HAS_OMIT_FRAME_POINTER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
endif()
if (HAS_CXX_OMIT_FRAME_POINTER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
endif()
