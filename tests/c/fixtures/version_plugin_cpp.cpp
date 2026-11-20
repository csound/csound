#include "csdl.h"

/* The same as version_plugin.c, built as C++: the version keeps C linkage
   and external linkage although it is const. */
CSOUND_PLUGIN_VERSION("4.5.6-cpp");

extern "C" {
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
}
