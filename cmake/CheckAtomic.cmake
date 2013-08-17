
include(CheckCSourceCompiles)

check_c_source_compiles("
#include <stdio.h>
#include <stdlib.h>

int main() {
  static volatile unsigned long val = 0;
  unsigned long u =  __sync_add_and_fetch(&val, 1); 

  if(u - 1 == 0) { 
    return 0;
  } 
 
  return 1; 
}

" HAVE_ATOMIC_BUILTIN)


