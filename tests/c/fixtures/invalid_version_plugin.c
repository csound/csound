#include "csdl.h"

/* A plugin whose version string has a control character: it still loads,
   without a version. */
CSOUND_PLUGIN_VERSION("1.0\n");

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
  return OK;
}
