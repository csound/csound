#include "csdl.h"

/* A plugin that gives its own version, independent of the Csound version. */
CSOUND_PLUGIN_VERSION("1.2.3-test");

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
