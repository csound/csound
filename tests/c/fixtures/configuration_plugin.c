#include "csdl.h"
#include "cfgvar.h"

typedef struct {
  char client[64];
  int32_t enabled;
} PLUGIN_OPTIONS;

PUBLIC int32_t csoundModuleInfo(void)
{
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
  PLUGIN_OPTIONS *options;
  int32_t length = 64;
  int32_t result;

  if (csound->CreateGlobalVariable(csound, "test_plugin_options",
                                   sizeof(PLUGIN_OPTIONS)) != 0)
    return CSOUND_ERROR;
  options = csound->QueryGlobalVariable(csound, "test_plugin_options");
  options->enabled = 1;
  result = csound->CreateConfigurationVariable(
      csound, "test_plugin_client", options->client, CSOUNDCFG_STRING,
      0, NULL, &length, "Test plugin client name", NULL);
  if (result != CSOUNDCFG_SUCCESS)
    return result;
  return csound->CreateConfigurationVariable(
      csound, "test_plugin_enabled", &options->enabled, CSOUNDCFG_BOOLEAN,
      0, NULL, NULL, "Enable the test plugin", NULL);
}
