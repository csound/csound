option(BUILD_HDF5_OPCODES "Build the hdf5 read/write opcodes" ON)

if(MSVC)
    find_package(HDF5 CONFIG REQUIRED)
    
    check_deps(BUILD_HDF5_OPCODES HDF5_FOUND)
    if(BUILD_HDF5_OPCODES)
        make_plugin(hdf5ops HDF5IO.c)
        target_include_directories(hdf5ops PRIVATE ${HDF5_INCLUDE_DIRS})
        target_link_libraries(hdf5ops PRIVATE hdf5::hdf5-static hdf5::hdf5_hl-static)
    endif()
else()
    find_package(HDF5)
    
    check_deps(BUILD_HDF5_OPCODES HDF5_FOUND)
    if(BUILD_HDF5_OPCODES)
        message(STATUS "HDF5_INCLUDE_DIRS: ${HDF5_INCLUDE_DIRS}")
        message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")
        make_plugin(hdf5ops HDF5IO.c)
        target_include_directories(hdf5ops PRIVATE ${HDF5_INCLUDE_DIRS})
        target_link_libraries(hdf5ops ${HDF5_LIBRARIES})
    endif()
endif()

