make_plugin(system_call system_call.c)
if (WIN32)
    target_compile_definitions(system_call PRIVATE "WIN32")
endif()
