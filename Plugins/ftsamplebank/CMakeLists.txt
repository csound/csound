make_plugin(ftsamplebank ftsamplebank.cpp libcsound_internals)
get_filename_component(DIRENT_INCLUDE_DIR ${DIRENT_H} DIRECTORY)
target_include_directories(ftsamplebank PRIVATE ${DIRENT_INCLUDE_DIR})