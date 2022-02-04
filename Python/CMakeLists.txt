## Python module ##

if(NOT APPLE)
      execute_process (
           COMMAND python3 -c
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

