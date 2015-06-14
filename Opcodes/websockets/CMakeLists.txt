option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)
FIND_PATH ( WEBSOCKETS_H libwebsockets.h
    /usr/local/include
    /usr/include
)

FIND_LIBRARY ( websockets_library websockets
    /usr/local/lib
    /usr/lib
)

find_library (websockets_library NAMES websockets)
check_include_file(libwebsockets.h WEBSOCKETS_H)

check_deps(BUILD_WEBSOCKET_OPCODE websockets_library WEBSOCKETS_H)
if(BUILD_WEBSOCKET_OPCODE)
 	make_plugin(websocketIO WebSocketOpcode.c)
 	target_link_libraries(websocketIO ${websockets_library} ${CSOUNDLIB})
endif()
