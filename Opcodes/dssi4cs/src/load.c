/* load.c

   Copyright 2005 Richard W.E. Furse
   with Csound adjustments by Andres Cabrera, Istvan Varga, John ffitch

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

#include "utils.h"
#include <dlfcn.h>

static void *try_library(CSOUND *csound, const char *path, int flags)
{
    const char *suffixes[] = {"", ".so", NULL};
    for (int i = 0; suffixes[i]; ++i) {
        size_t size = strlen(path) + strlen(suffixes[i]) + 1;
        char *name = csound->Malloc(csound, size);
        snprintf(name, size, "%s%s", path, suffixes[i]);
        void *library = dlopen(name, flags | RTLD_LOCAL);
        csound->Free(csound, name);
        if (library) return library;
    }
    return NULL;
}

void *dlopenLADSPA(CSOUND *csound, const char *name, int32_t flags)
{
    if (strchr(name, '/')) return try_library(csound, name, flags);
    const char *paths[] = {csound->GetEnv(csound, "DSSI_PATH"),
                          csound->GetEnv(csound, "LADSPA_PATH"),
                          DSSI4CS_DEFAULT_PATH};
    for (unsigned i = 0; i < sizeof(paths)/sizeof(paths[0]); ++i) {
        const char *start = paths[i];
        while (start && *start) {
            const char *end = strchr(start, ':');
            size_t len = end ? (size_t)(end - start) : strlen(start);
            if (len) {
                size_t size = len + strlen(name) + 2;
                char *path = csound->Malloc(csound, size);
                snprintf(path, size, "%.*s/%s", (int)len, start, name);
                void *library = try_library(csound, path, flags);
                csound->Free(csound, path);
                if (library) return library;
            }
            start = end ? end + 1 : NULL;
        }
    }
    return NULL;
}
