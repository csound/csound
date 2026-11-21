#include "csdl.h"

/* A plugin that loads but fails to start: csoundModuleCreate() succeeds,
   csoundModuleInit() reports an error. */

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  (void) csound;
  return OK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  (void) csound;
  return NOTOK;
}
