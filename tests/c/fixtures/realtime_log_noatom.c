/* Force the queue startup check without changing the platform headers. */
#include "csoundCore.h"
#undef HAVE_ATOMIC_BUILTIN
#include "../../../Engine/insert.c"
