make_plugin(ambicode1 ambicode1.c)
# needed for asserts
target_compile_definitions(ambicode1 PRIVATE "$<$<CONFIG:Release>:NDEBUG>")