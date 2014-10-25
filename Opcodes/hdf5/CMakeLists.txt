option(BUILD_HDF5_OPCODES "Build the hdf5 read/write opcodes" OFF)

if(BUILD_HDF5_OPCODES)

	find_library (HDF5_library NAMES hdf5)

	include_directories(${CMAKE_HOME_DIRECTORY}/include) 
 
	include_directories(/usr/local/include)
 
 	make_plugin(hdf5 HDF5IO.c)
 	target_link_libraries(hdf5 ${HDF5_library})

endif()
