#include <stdlib.h>
#include "csdl.h"

/* A plugin that does not load: csoundModuleCreate() registers a keyboard
   callback, which outlives a reset, and then reports an error. Csound must
   keep the library open while the callback can be called, and must neither
   start nor destroy the module, so the other two functions abort. */

/* sensekey calls it: it counts its calls in a channel a test reads. */
static int32_t count_call(void *userData, void *p, uint32_t type)
{
  CSOUND    *csound = (CSOUND *) userData;
  cs_float  *calls;

  (void) p;
  (void) type;
  if (csound->GetChannelPtr(csound, (void **) &calls, "failing_create_calls",
                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL)
      == CSOUND_SUCCESS)
    *calls += 1;
  return OK;
}

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  csound->Set_KeyCallback(csound, count_call, csound,
                          CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT);
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
