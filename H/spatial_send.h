/* Private source registration for locsend and spsend. */
#ifndef CSOUND_SPATIAL_SEND_H
#define CSOUND_SPATIAL_SEND_H

#include "csoundCore.h"

#define SPATIAL_SOURCES_INSTANCE "csound.spatial.sources"
enum { SPATIAL_LOCSIG, SPATIAL_SPACE, SPATIAL_SOURCE_TYPES };

typedef struct {
    OPDS *latest[SPATIAL_SOURCE_TYPES];
} SPATIAL_SOURCES;

/* Keep the latest source of each type in this instance. Existing sends cache
   their own source pointers. The engine keeps this record through reinit and
   deinit, then clears it before reuse. Audio callbacks do not query it. */
static inline void spatial_source_remove(CSOUND *csound, OPDS *opcode)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryInstanceVariable(csound, opcode->insdshead,
                                     SPATIAL_SOURCES_INSTANCE);
    if (sources == NULL)
      return;
    for (int32_t type = 0; type < SPATIAL_SOURCE_TYPES; ++type)
      if (sources->latest[type] == opcode)
        sources->latest[type] = NULL;
}

static inline int32_t spatial_source_register(CSOUND *csound, OPDS *opcode,
                                              int32_t type)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryInstanceVariable(csound, opcode->insdshead,
                                     SPATIAL_SOURCES_INSTANCE);
    if (sources == NULL) {
      if (csound->CreateInstanceVariable(csound, opcode->insdshead,
            SPATIAL_SOURCES_INSTANCE, sizeof(SPATIAL_SOURCES)) != OK)
        return csound->InitError(csound, "%s",
                                Str("could not allocate spatial source state"));
      sources = (SPATIAL_SOURCES *)csound->QueryInstanceVariable(
          csound, opcode->insdshead, SPATIAL_SOURCES_INSTANCE);
    }
    sources->latest[type] = opcode;
    return OK;
}

static inline OPDS *spatial_source_find(CSOUND *csound, INSDS *owner,
                                       int32_t type)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryInstanceVariable(csound, owner, SPATIAL_SOURCES_INSTANCE);
    return sources != NULL ? sources->latest[type] : NULL;
}

#endif
