make_plugin(serial serial.c)
if (WIN32)
    target_compile_definitions(serial PRIVATE "WIN32")
endif()