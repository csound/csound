# SndFile_FOUND: if found
# SndFile::sndfile: imported module

include(FindPackageHandleStandardArgs)

find_library(SndFile_LIBRARY NAMES sndfile libsndfile-1 libsndfile)
find_path(SndFile_INCLUDE_DIR sndfile.h)

find_package_handle_standard_args(SndFile
	FOUND_VAR SndFile_FOUND
	REQUIRED_VARS SndFile_LIBRARY SndFile_INCLUDE_DIR
)

if (SndFile_FOUND)
	add_library(SndFile::sndfile UNKNOWN IMPORTED)
	set_target_properties(SndFile::sndfile PROPERTIES
		IMPORTED_LOCATION ${SndFile_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${SndFile_INCLUDE_DIR}
	)
endif()
