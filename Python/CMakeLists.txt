## Python module ##

option(BUILD_PYTHON_INTERFACE "Build the Python interface" ON)

find_program(PYTHON3_EXECUTABLE python3)

if(BUILD_PYTHON_INTERFACE AND NOT APPLE AND PYTHON3_EXECUTABLE)
      execute_process (
           COMMAND ${PYTHON3_EXECUTABLE} -c
           "import site, sys; sys.stdout.write(site.getsitepackages()[0])"
           OUTPUT_VARIABLE PYTHON3_SITE_PACKAGES
         )
      SET(PYTHON3_MODULE_INSTALL_DIR ${PYTHON3_SITE_PACKAGES} CACHE
       PATH "Python3 module install dir")
       
      if (PYTHON3_MODULE_INSTALL_DIR)
         message(STATUS "PYTHON3 MODULE INSTALL DIR: ${PYTHON3_MODULE_INSTALL_DIR}" )
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ctcsound.py
            DESTINATION ${PYTHON3_MODULE_INSTALL_DIR})
      endif()
endif(NOT APPLE)

