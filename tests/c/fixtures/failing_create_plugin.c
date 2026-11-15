#include <stdlib.h>
#include "csdl.h"

/* A plugin that does not load: csoundModuleCreate() reports an error. Csound
   must then neither start nor destroy it, so the other two abort. */

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  (void) csound;
  return NOTOK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  (void) csound;
  abort();
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound)
{
  (void) csound;
  abort();
}
