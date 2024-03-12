make_plugin(ftsamplebank ftsamplebank.cpp)
get_filename_component(DIRENT_INCLUDE_DIR ${DIRENT_H} DIRECTORY)
target_include_directories(ftsamplebank PRIVATE 
    ${libcsound_private_include_dirs}
    ${DIRENT_INCLUDE_DIR}
)