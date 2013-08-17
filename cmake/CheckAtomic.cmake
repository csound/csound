
include(CheckCSourceCompiles)

check_c_source_compiles("

int main() {
  int val = 0;
  __sync_lock_test_and_set(&val, 1); 
}

" HAVE_ATOMIC_BUILTIN)


