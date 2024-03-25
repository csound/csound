# use the static library if available
make_plugin(rtpa rtpa.c $<IF:$<TARGET_EXISTS:portaudio_static>,portaudio_static,portaudio>)