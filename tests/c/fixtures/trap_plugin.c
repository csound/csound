#include <stdlib.h>
#include "csdl.h"

/* A plugin that stops the process as soon as it is loaded: --list-plugins
   must read its metadata without loading it. */
CSOUND_PLUGIN_METADATA("9.9.9-trap", "opcodes = ['trapop']\n");

#if defined(__GNUC__) || defined(__clang__)
__attribute__((constructor)) static void stop_on_load(void)
{
  abort();
}
#endif

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  (void) csound;
  abort();
}
