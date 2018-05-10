file(GLOB csheaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
file(GLOB csheaders_hpp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.hpp")
list(APPEND csheaders ${csheaders_hpp})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/float-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/float-version.h)

# These don't live here... but we list them here to have all headers
# in one place
list(APPEND csheaders
    ../interfaces/CsoundFile.hpp
    ../interfaces/CppSound.hpp
    ../interfaces/filebuilding.h
    ../interfaces/csPerfThread.hpp)

set(csheaders ${csheaders} PARENT_SCOPE)

if(NOT APPLE)
    INSTALL(FILES ${csheaders} DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/float-version.h DESTINATION ${HEADER_INSTALL_DIR})
endif()

