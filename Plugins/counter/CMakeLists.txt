make_plugin(counter counter.c)
if (INIT_STATIC_MODULES)
    add_definitions("INIT_STATIC_COUNTER")
endif()