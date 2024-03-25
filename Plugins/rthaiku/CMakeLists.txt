find_library(HAIKU_MEDIA_LIBRARY media)
find_library(HAIKU_MIDI_LIBRARY midi2)
set(haiku_SRCS rthaiku.cpp HaikuAudio.cpp HaikuMidi.cpp)
make_plugin(rthaiku "${haiku_SRCS}" ${HAIKU_MEDIA_LIBRARY} ${HAIKU_MIDI_LIBRARY})
	