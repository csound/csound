## Python module ##

option(INSTALL_PYTHON_INTERFACE "Install the Python interface" ON)

find_package(Python3)

assign_bool(NOT_APPLE NOT APPLE)
check_deps(INSTALL_PYTHON_INTERFACE NOT_APPLE Python3_FOUND)

if(INSTALL_PYTHON_INTERFACE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ctcsound.py
        DESTINATION ${Python3_SITELIB})
endif()

