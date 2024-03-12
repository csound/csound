if(WIN32)
  set(ipmidi_LIBS ws2_32)
endif()
make_plugin(ipmidi ipmidi.c "${ipmidi_LIBS}")