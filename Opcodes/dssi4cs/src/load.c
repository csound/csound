/* load.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include "utils.h"
#include <dlfcn.h>

/*****************************************************************************/

/* This function provides a wrapping of dlopen(). When the filename is
   not an absolute path (i.e. does not begin with / character), this
   routine will search the LADSPA_PATH for the file. */
/* TODO static? */
void   *dlopenLADSPA(CSOUND *csound, const char *pcFilename, int iFlag)
{

    char   *pcBuffer;
    const char *pcEnd;
          char *pcLADSPAPath = NULL;
    const char *pcDSSIPath;
    const char *pcStart;
    int     iEndsInSO;
    int     iNeedSlash;
    size_t  iFilenameLength;
    void   *pvResult;

    iFilenameLength = strlen(pcFilename);
    pvResult = NULL;

    if (pcFilename[0] == '/') {
      /* The filename is absolute. Assume the user knows what he/she is
         doing and simply dlopen() it. */
      pvResult = dlopen(pcFilename, iFlag);
      if (pvResult != NULL)
        return pvResult;
    }
    else {
      /* If the filename is not absolute then we wish to check along the
         LADSPA_PATH path to see if we can find the file there. We do
         NOT call dlopen() directly as this would find plugins on the
         LD_LIBRARY_PATH, whereas the LADSPA_PATH is the correct place
         to search. */
      pcLADSPAPath = getenv("LADSPA_PATH");
      pcDSSIPath = getenv("DSSI_PATH");
      if (pcDSSIPath) {
        int len = strlen(pcLADSPAPath)+strlen(pcDSSIPath)+2;
        char *tmp = (char*)malloc(len);
        snprintf(tmp, len, "%s:%s", pcLADSPAPath, pcDSSIPath);
        pcLADSPAPath = tmp;
      }
      if (pcLADSPAPath) {
        pcStart = pcLADSPAPath;
        while (*pcStart != '\0') {
          pcEnd = pcStart;
          while (*pcEnd != ':' && *pcEnd != '\0')
            pcEnd++;

          pcBuffer = csound->Malloc(csound,
                                    iFilenameLength + 2 + (pcEnd - pcStart));
          if (pcEnd > pcStart)
            strncpy(pcBuffer, pcStart, pcEnd - pcStart);
          iNeedSlash = 0;
          if (pcEnd > pcStart)
            if (*(pcEnd - 1) != '/') {
              iNeedSlash = 1;
              pcBuffer[pcEnd - pcStart] = '/';
            }
          strcpy(pcBuffer + iNeedSlash + (pcEnd - pcStart), pcFilename);

          pvResult = dlopen(pcBuffer, iFlag);

          csound->Free(csound, pcBuffer);
          if (pvResult != NULL)
            return pvResult;
          
          pcStart = pcEnd;
          if (*pcStart == ':')
            pcStart++;
        }
      }
    }
    free(pcLADSPAPath);
    /* As a last ditch effort, check if filename does not end with
       ".so". In this case, add this suffix and recurse. */
    iEndsInSO = 0;
    if (iFilenameLength > 3)
      iEndsInSO = (strcmp(pcFilename + iFilenameLength - 3, ".so") == 0);
    if (!iEndsInSO) {
      pcBuffer = csound->Malloc(csound, iFilenameLength + 4);
      strcpy(pcBuffer, pcFilename);
      strcat(pcBuffer, ".so");
      pvResult = dlopenLADSPA(csound, pcBuffer, iFlag);
      csound->Free(csound, pcBuffer);
    }

    if (pvResult != NULL)
      return pvResult;

    /* If nothing has worked, then at least we can make sure we set the
       correct error message - and this should correspond to a call to
       dlopen() with the actual filename requested. The dlopen() manual
       page does not specify whether the first or last error message
       will be kept when multiple calls are made to dlopen(). We've
       covered the former case - now we can handle the latter by calling
       dlopen() again here. */
    return dlopen(pcFilename, iFlag);
}

/*****************************************************************************/

void   *loadLADSPAPluginLibrary(CSOUND *csound, const char *pcPluginFilename)
{

    void   *pvPluginHandle;

    /* pvPluginHandle = dlopenLADSPA(csound, pcPluginFilename, RTLD_LAZY); */
    pvPluginHandle = dlopenLADSPA(csound, pcPluginFilename, RTLD_NOW);
    if (!pvPluginHandle) {
      csound->Die(csound, Str("Failed to load plugin \"%s\": %s"),
                          pcPluginFilename, dlerror());
    }

    return pvPluginHandle;
}

/*****************************************************************************/

void unloadLADSPAPluginLibrary(CSOUND *csound, void *pvLADSPAPluginLibrary)
{
    dlclose(pvLADSPAPluginLibrary);
}

/*****************************************************************************/

const LADSPA_Descriptor *
    findLADSPAPluginDescriptor(CSOUND *csound,
                               void *pvLADSPAPluginLibrary,
                               const char *pcPluginLibraryFilename,
                               const char *pcPluginLabel)
{

    const LADSPA_Descriptor *psDescriptor;
    LADSPA_Descriptor_Function pfDescriptorFunction;
    unsigned long lPluginIndex;

    dlerror();
    pfDescriptorFunction
        = (LADSPA_Descriptor_Function) dlsym(pvLADSPAPluginLibrary,
                                             "ladspa_descriptor");
    if (!pfDescriptorFunction) {
      const char *pcError = dlerror();

      if (pcError) {
        csound->Die(csound, Str("Unable to find ladspa_descriptor() function "
                                "in plugin library file \"%s\": %s.\n"
                                "Are you sure this is a LADSPA plugin file ?"),
                            pcPluginLibraryFilename, pcError);
      }
      else {
        csound->Die(csound, Str("Unable to find ladspa_descriptor() function "
                                "in plugin library file \"%s\".\n"
                                "Are you sure this is a LADSPA plugin file ?"),
                            pcPluginLibraryFilename);
      }
    }

    for (lPluginIndex = 0; ; lPluginIndex++) {
      psDescriptor = pfDescriptorFunction(lPluginIndex);
      if (psDescriptor == NULL)
        break;
      if (strcmp(psDescriptor->Label, pcPluginLabel) == 0)
        return psDescriptor;
    }

    csound->Die(csound, Str("Unable to find label \"%s\" "
                            "in plugin library file \"%s\"."),
                        pcPluginLabel, pcPluginLibraryFilename);
    return NULL;    /* compiler only; not reached */
}

