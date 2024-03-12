# should probably do checks for these libs
make_plugin(rtwinmm rtwinmm.c gdi32 kernel32)
if(MSVC)
    target_link_libraries(rtwinmm PRIVATE winmm.lib)
else()
    target_link_libraries(rtwinmm PRIVATE winmm)
endif()