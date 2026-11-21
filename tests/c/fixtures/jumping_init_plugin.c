#include "csdl.h"

/* A plugin that stops Csound as it starts: csoundModuleCreate() succeeds,
   and csoundModuleInit() jumps, as a csound->Die() in it would, instead of
   returning an error. */

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
  csound->LongJmp(csound, 1);
  return NOTOK;
}
