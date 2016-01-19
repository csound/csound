option(BUILD_HDF5_OPCODES "Build the hdf5 read/write opcodes" ON)

find_library (HDF5_library NAMES hdf5)
check_include_file(hdf5.h HDF5_H)

check_deps(BUILD_HDF5_OPCODES HDF5_library HDF5_H)
if(BUILD_HDF5_OPCODES)
 	make_plugin(hdf5 HDF5IO.c)
 	target_link_libraries(hdf5 ${HDF5_library})
  if(WIN32 AND NOT MSVC) 
 	  target_link_libraries(hdf5 szip z)
  endif()
endif()
