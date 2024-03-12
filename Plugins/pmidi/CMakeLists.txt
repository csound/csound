make_plugin(pmidi pmidi.c)
target_link_libraries(pmidi PRIVATE PortMidi::portmidi)
