file(GLOB csheaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
file(GLOB csheaders_hpp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.hpp")
list(APPEND csheaders ${csheaders_hpp})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/float-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/float-version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

# copy version.h for Android
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_SOURCE_DIR}/Android/CsoundAndroid/jni/version.h)

set(csheaders ${csheaders} PARENT_SCOPE)

if(NOT APPLE)
    INSTALL(FILES ${csheaders} DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/float-version.h
     DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
     DESTINATION ${HEADER_INSTALL_DIR})
endif()

