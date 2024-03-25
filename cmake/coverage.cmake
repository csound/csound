# link option is required for the compiler option, so set and unset
set(CMAKE_REQUIRED_LINK_OPTIONS "--coverage")
check_cxx_compiler_flag("--coverage" has_coverage)
set(CMAKE_REQUIRED_LINK_OPTIONS "")
if (has_coverage)
    add_compile_options("--coverage")
    add_link_options("--coverage")
endif()

# absolute paths help coverage tools
try_add_compile_option("-fprofile-abs-path")