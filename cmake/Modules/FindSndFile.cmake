# SNDFILE_FOUND: whether SndFile was found
# SndFile::sndfile: imported module

include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

find_dependency(Vorbis)
find_dependency(FLAC)
find_dependency(Opus)

if (USE_MP3)
	find_dependency(mp3lame)
	find_dependency (MPG123)
endif()

find_library(SndFile_LIBRARY NAMES sndfile)
find_path(SndFile_INCLUDE_DIR sndfile.h)

find_package_handle_standard_args(SndFile
	FOUND_VAR SNDFILE_FOUND
	REQUIRED_VARS SndFile_LIBRARY SndFile_INCLUDE_DIR
)

if(SNDFILE_FOUND AND NOT TARGET SndFile::sndfile)
	add_library(SndFile::sndfile UNKNOWN IMPORTED)
	set_target_properties(SndFile::sndfile PROPERTIES
		IMPORTED_LOCATION ${SndFile_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${SndFile_INCLUDE_DIR}
	)
	set_property(TARGET SndFile::sndfile
		PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES
			m
			FLAC::FLAC
			Opus::opus
			Vorbis::vorbisenc
	)
	if (USE_MP3)
		set_property(TARGET SndFile::sndfile
			APPEND PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES
				mp3lame::mp3lame
				MPG123::libmpg123 
		)
	endif()
endif()

mark_as_advanced(
	SndFile_LIBRARY
	SndFile_INCLUDE_DIR
)