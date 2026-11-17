#include "csdl.h"

/* A plugin whose csoundModuleCreate() and csoundModuleInit() call the
   function the host leaves in the global variable "hook_plugin_create" or
   "hook_plugin_init", if any: plugin code that calls the API, as a plugin
   may, without linking a Csound of its own. */
typedef void (*HOOK)(CSOUND *);

static void call_hook(CSOUND *csound, const char *name)
{
  HOOK *hook = (HOOK *) csound->QueryGlobalVariable(csound, name);
  if (hook != NULL && *hook != NULL)
    (*hook)(csound);
}

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  call_hook(csound, "hook_plugin_create");
  return OK;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
  call_hook(csound, "hook_plugin_init");
  return OK;
}
