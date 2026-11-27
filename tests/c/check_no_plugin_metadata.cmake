# Fails if FILE holds the plugin metadata marker. libcsound reads plugin
# files for the marker, so it must not hold the marker itself: the reader
# builds it at run time, and a compiler that folded it into a constant
# would make libcsound look like a plugin with a metadata record.
file(READ "${FILE}" contents HEX)
# "CSPLMETA", in hex
string(FIND "${contents}" "4353504c4d455441" position)
if(NOT position EQUAL -1)
    message(FATAL_ERROR "${FILE} holds the plugin metadata marker")
endif()
