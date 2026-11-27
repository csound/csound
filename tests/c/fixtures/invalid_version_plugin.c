#include "csdl.h"

/* A plugin whose version has a control character: its record is invalid,
   and it still loads. */
CSOUND_PLUGIN_METADATA("1.0\n", "");

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
