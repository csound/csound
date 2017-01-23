option(BUILD_EMUGENS_OPCODES "Build emugens opcodes" ON)

if(BUILD_EMUGENS_OPCODES)
  # set(SOURCES emugens.c scugens.c)
  make_plugin(scugens scugens.c)
  make_plugin(emugens emugens.c)
endif()
