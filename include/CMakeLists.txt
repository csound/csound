file(GLOB csheaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
file(GLOB csheaders_hpp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.hpp")
list(APPEND csheaders ${csheaders_hpp})

# Keep generated headers in each build directory to avoid concurrent writes.
# The NDK build uses its own header, updated by platform/android/CsoundAndroid/build.sh.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/float-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/float-version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(csheaders ${csheaders} PARENT_SCOPE)

if(NOT APPLE)
    INSTALL(FILES ${csheaders} DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/float-version.h
     DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
     DESTINATION ${HEADER_INSTALL_DIR})
endif()
