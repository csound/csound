
include(CheckCSourceCompiles)
include(CMakePushCheckState)

if(USE_DOUBLE)
    set(type "int64_t")
else()
    set(type "int32_t")
endif()

set(atomic_code "
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
  static volatile ${type} val = 0;
  ${type} u =  __atomic_add_fetch(&val, 1, __ATOMIC_RELAXED);

  if(u - 1 == 0) {
    return 0;
  }

  return 1;
}

")

check_c_source_compiles("${atomic_code}" HAVE_ATOMIC_BUILTIN_NOLIB)

set(ATOMIC_LIB "")
if (HAVE_ATOMIC_BUILTIN_NOLIB)
    set(HAVE_ATOMIC_BUILTIN HAVE_ATOMIC_BUILTIN_NOLIB)
else()
    # Sometimes linking against libatomic is required for atomic ops, if
    # the platform doesn't support lock-free atomics.
    cmake_push_check_state()
    list(APPEND CMAKE_REQUIRED_LIBRARIES atomic)
    check_c_source_compiles("${atomic_code}" HAVE_ATOMIC_BUILTIN)
    cmake_pop_check_state()
    set(ATOMIC_LIB "atomic")
endif()
