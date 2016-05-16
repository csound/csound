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
endif()

check_include_file(libwebsockets.h WEBSOCKETS_H)


if(WEBSOCKETS_H AND websockets_library) 
  set(CMAKE_REQUIRED_INCLUDES libwebsockets.h)
  set(CMAKE_REQUIRED_LIBRARIES ${websockets_library})
  check_function_exists(libwebsocket_cancel_service LIBWEBSOCKET_CANCEL_SERVICE)
endif()

check_deps(BUILD_WEBSOCKET_OPCODE websockets_library WEBSOCKETS_H LIBWEBSOCKET_CANCEL_SERVICE)
if(BUILD_WEBSOCKET_OPCODE)
 	make_plugin(websocketIO WebSocketOpcode.c)
 	target_link_libraries(websocketIO ${websockets_library} ${CSOUNDLIB})
  
  if(WIN32 AND NOT MSVC) 
    target_link_libraries(websocketIO ssl crypto z ws2_32 wsock32)
  endif()
endif()
