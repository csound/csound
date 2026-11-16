#include "csdl.h"

/* A plugin that does not load: csoundModuleCreate() reports an error. */

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  (void) csound;
  return NOTOK;
}
