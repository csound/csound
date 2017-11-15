option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)
FIND_PATH ( WEBSOCKETS_H libwebsockets.h
    /usr/local/include
    /usr/include
)

if(WIN32 AND NOT MSVC) 
  find_library (websockets_library NAMES websockets_static)
else()
  FIND_LIBRARY ( websockets_library websockets
      /usr/local/lib
      /usr/lib
  )
  find_library (websockets_library NAMES websockets)

if(APPLE)
FIND_LIBRARY (ssl_library NAMES ssl.a
      /usr/local/lib
      /usr/lib
  )
   #find_library (crypto_library NAMES crypto.a
   FIND_LIBRARY ( crypto_library NAMES crypto.a
      /usr/local/lib
      /usr/lib
  )
   #find_library (crypto_library NAMES crypto)
endif()

endif()

check_include_file(libwebsockets.h WEBSOCKETS_H)

if(WEBSOCKETS_H AND websockets_library) 
  set(CMAKE_REQUIRED_INCLUDES libwebsockets.h)
  set(CMAKE_REQUIRED_LIBRARIES ${websockets_library})
endif()

check_deps(BUILD_WEBSOCKET_OPCODE websockets_library)
if(BUILD_WEBSOCKET_OPCODE)

 	make_plugin(websocketIO WebSocketOpcode.c)
 	target_link_libraries(websocketIO ${websockets_library} ${CSOUNDLIB})

  if(APPLE)
   target_link_libraries(websocketIO /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a  z)
  endif()
  
  if(WIN32 AND NOT MSVC) 
    target_link_libraries(websocketIO ssl crypto z ws2_32 wsock32)
  endif()
endif()
