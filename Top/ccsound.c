/* Console Csound using the Csound API. */

#include <stdio.h>
#include <stdarg.h>
#include "csound.h"

void csoundMessage0(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

int main(int argc, char **argv)
{
  void *csound = csoundCreate(0);
  int rc = csoundPerform(csound, argc, argv);
  csoundCleanup(csound);
  csoundReset(csound);
  return rc;
}
