#include "csdl.h"

/* A plugin with a metadata record: its own version, independent of the
   Csound version, and a TOML text. */
CSOUND_PLUGIN_METADATA("1.2.3-test", "opcodes = ['vtest', 'vtest2']\n");

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
