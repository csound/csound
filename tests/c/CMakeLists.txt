#ENABLE_TESTING()

add_executable(csoundTypeSystemTest csound_type_system_test.c)
target_link_libraries(csoundTypeSystemTest ${CSOUNDLIB})
set(csoundTypeSystemTest_CFLAGS -D__BUILDING_LIBCSOUND)

add_test(NAME TestCsoundTypeSystem
	COMMAND $<TARGET_FILE:csoundTypeSystemTest>)

