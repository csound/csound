# Try to find the GMM library.
# Once done this will define:
#  GMM_FOUND - System has the GMM library.
#  GMM_INCLUDE_DIRS - The GMM include directories.
#  GMM_HAS_VECTOR_OSTREAM - Wether GMM defines operator<< for std::vector

find_path(GMM_INCLUDE_DIR gmm/gmm.h)

set(GMM_INCLUDE_DIRS ${GMM_INCLUDE_DIR})

include(CMakePushCheckState)

cmake_push_check_state(RESET)

list(APPEND CMAKE_REQUIRED_INCLUDES ${GMM_INCLUDE_DIR})

check_cxx_source_compiles("
#include <gmm/gmm.h>
#include <sstream>

int main() {
    std::ostringstream stream;
    std::vector<double> vec;
    stream << vec << std::endl;
    return 0;
}
" GMM_HAS_VECTOR_OSTREAM)

cmake_pop_check_state()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GMM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GMM DEFAULT_MSG
    GMM_INCLUDE_DIR)

mark_as_advanced(GMM_INCLUDE_DIR GMM_HAS_VECTOR_OSTREAM)
