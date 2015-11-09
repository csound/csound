option(BUILD_CUDA_OPCODES "Build the CUDA GPU opcodes" OFF)

if(BUILD_CUDA_OPCODES)
 FIND_PACKAGE(CUDA REQUIRED)
 check_deps(BUILD_CUDA_OPCODES CUDA_FOUND)
 if(BUILD_CUDA_OPCODES)
 function(make_cuda_plugin libname srcs)

 SET(LIB_TYPE_DYNAMIC) #set the lib type
 CUDA_ADD_LIBRARY(${libname} ${LIB_TYPE} ${srcs})

    set(i 2)
    while( ${i} LESS ${ARGC} )
        if(NOT MSVC OR NOT("${ARGV${i}}" MATCHES "m"))
            target_link_libraries(${libname} ${ARGV${i}})
        endif()
        math(EXPR i "${i}+1")
    endwhile()

    set_target_properties(${libname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})

    install(TARGETS ${libname}
	LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" )

endfunction(make_cuda_plugin)
 
 #find_library(CUFFT cufft HINTS "/Developer/NVIDIA/CUDA-6.0/lib")
 include_directories(${CMAKE_HOME_DIRECTORY}/include) 
 include_directories(/usr/local/include)

 if(APPLE)
 if(CMAKE_OSX_DEPLOYMENT_TARGET MATCHES 10.6)
 SET(CUDA_NVCC_FLAGS  "-arch=sm_11 -use_fast_math" CACHE STRING "nvcc flags" FORCE)
  make_cuda_plugin(cudaop1 adsyn11.cu)
  make_cuda_plugin(cudaop2 pvsopsf.cu)
  make_cuda_plugin(cudaop3 slidingm.cu)
  make_cuda_plugin(cudaop4 convf.cu)
   make_cuda_plugin(cudaop5 pconv11.cu)
   CUDA_ADD_CUFFT_TO_TARGET(cudaop2)
      CUDA_ADD_CUFFT_TO_TARGET(cudaop5)
 # target_link_libraries(cudaop2 ${CUFFT})
 #target_link_libraries(cudaop5 ${CUFFT})
 else()
SET(CUDA_NVCC_FLAGS  "-ccbin /usr/bin/clang -arch=sm_30 -Xcompiler
  -stdlib=libstdc++ -use_fast_math" CACHE STRING "nvcc flags" FORCE)
  make_cuda_plugin(cudaop1 adsyn.cu)
  make_cuda_plugin(cudaop2 pvsops.cu)
  make_cuda_plugin(cudaop3 slidingm.cu)
  make_cuda_plugin(cudaop4 conv.cu)
   make_cuda_plugin(cudaop5 pconv.cu)
   CUDA_ADD_CUFFT_TO_TARGET(cudaop2)
   CUDA_ADD_CUFFT_TO_TARGET(cudaop5)
   #target_link_libraries(cudaop5 ${CUFFT})
  #target_link_libraries(cudaop2 ${CUFFT})
 endif()
 else()
  SET(CUDA_NVCC_FLAGS  "-arch=sm_20 -use_fast_math" CACHE STRING "nvcc flags" FORCE)
  make_cuda_plugin(cudaop1 adsyn.cu)
  make_cuda_plugin(cudaop2 pvsops.cu)
  make_cuda_plugin(cudaop3 slidingm.cu)
  make_cuda_plugin(cudaop4 conv.cu)
  make_cuda_plugin(cudaop5 pconv.cu)
  target_link_libraries(cudaop2 ${CUFFT})
   target_link_libraries(cudaop5 ${CUFFT})
 endif()

 SET (CUDA_VERBOSE_BUILD ON CACHE BOOL "nvcc verbose" FORCE)

 endif()

endif()
