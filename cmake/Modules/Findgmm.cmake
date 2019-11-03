# Try to find the GMM library.
# Once done this will define:
#  GMM_FOUND - System has the GMM library.
#  GMM_VERSION
#  GMM_INCLUDE_DIR - The GMM include directories.
#  GMM_HAS_VECTOR_OSTREAM - Wether GMM defines operator<< for std::vector

if (GMM_INCLUDE_DIR)
  	# in cache already
  	set(GMM_FOUND TRUE)
else (GMM_INCLUDE_DIR)
	find_path(GMM_INCLUDE_DIR NAMES gmm/gmm.h
	 	PATHS
	 		${INCLUDE_INSTALL_DIR}
	 		${GMM_INCLUDE_PATH}
		)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(GMM DEFAULT_MSG GMM_INCLUDE_DIR)
	mark_as_advanced(GMM_INCLUDE_DIR)
endif(GMM_INCLUDE_DIR)