make_plugin(mixer mixer.cpp)
target_include_directories(mixer PRIVATE ${libcsound_private_include_dirs})
