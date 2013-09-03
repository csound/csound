
include(CheckCSourceCompiles)

if(USE_DOUBLE)
    set(type "int64_t")
else()
    set(type "int32_t")
endif()

check_c_source_compiles("
#include <stdio.h>
#include <stdlib.h>

int main() {
  static volatile ${type} val = 0;
  ${type} u =  __sync_add_and_fetch(&val, 1);

  if(u - 1 == 0) { 
    return 0;
  } 
 
  return 1; 
}

" HAVE_ATOMIC_BUILTIN)


