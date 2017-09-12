option(BUILD_OPENCL_OPCODES "Build the OpenCL parallel opcodes" OFF)

if(BUILD_OPENCL_OPCODES)
find_library (OpenCL_library NAMES OpenCL)

if(BUILD_OPENCL_OPCODES)

function(make_opencl_plugin libname srcs)

    if(APPLE)
        add_library(${libname} SHARED ${srcs})
    else()
        add_library(${libname} MODULE ${srcs})
    endif()

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
   
endfunction(make_opencl_plugin)
 
 include_directories(${CMAKE_HOME_DIRECTORY}/include) 
 include_directories(/usr/local/include)
 
 make_opencl_plugin(clop cladsynth.c)
 target_link_libraries(clop ${OpenCL_library})

 endif()

endif()
