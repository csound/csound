/* Private source registration for locsend and spsend. */
#ifndef CSOUND_SPATIAL_SEND_H
#define CSOUND_SPATIAL_SEND_H

#include "csoundCore.h"

#define SPATIAL_SOURCES_GLOBAL "_spatial_sources"
enum { SPATIAL_LOCSIG, SPATIAL_SPACE, SPATIAL_SOURCE_TYPES };

typedef struct spatial_source {
    struct spatial_source *next;
    OPDS *opcode;
    INSDS *owner;
} SPATIAL_SOURCE;

typedef struct {
    void *mutex;
    SPATIAL_SOURCE *head[SPATIAL_SOURCE_TYPES];
} SPATIAL_SOURCES;

/* Nodes live in the source opcodes. Only the latest source of each type in
   an instrument is registered; existing sends keep their resolved pointers.
   The mutex protects init/deinit on different engine threads. No registry
   operation runs in a source or send's audio callback. */
static inline void spatial_source_remove(CSOUND *csound, OPDS *opcode)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryGlobalVariable(csound, SPATIAL_SOURCES_GLOBAL);
    /* UGen contexts can be assigned before standard opcodes are loaded. */
    if (sources == NULL) return;
    csound->LockMutex(sources->mutex);
    for (int32_t type = 0; type < SPATIAL_SOURCE_TYPES; ++type) {
      SPATIAL_SOURCE **link = &sources->head[type];
      while (*link != NULL) {
        if ((*link)->opcode == opcode) {
          *link = (*link)->next;
          break;
        }
        link = &(*link)->next;
      }
    }
    csound->UnlockMutex(sources->mutex);
}

static inline void spatial_source_register(CSOUND *csound,
                                          SPATIAL_SOURCE *source,
                                          OPDS *opcode, int32_t type)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryGlobalVariable(csound, SPATIAL_SOURCES_GLOBAL);
    csound->LockMutex(sources->mutex);
    SPATIAL_SOURCE **link = &sources->head[type];
    while (*link != NULL) {
      if ((*link)->owner == opcode->insdshead) {
        *link = (*link)->next;
        break;
      }
      link = &(*link)->next;
    }
    source->opcode = opcode;
    source->owner = opcode->insdshead;
    source->next = sources->head[type];
    sources->head[type] = source;
    csound->UnlockMutex(sources->mutex);
}

static inline OPDS *spatial_source_find(CSOUND *csound, INSDS *owner,
                                       int32_t type)
{
    SPATIAL_SOURCES *sources = (SPATIAL_SOURCES *)
        csound->QueryGlobalVariable(csound, SPATIAL_SOURCES_GLOBAL);
    OPDS *opcode = NULL;
    csound->LockMutex(sources->mutex);
    for (SPATIAL_SOURCE *source = sources->head[type]; source != NULL;
         source = source->next) {
      if (source->owner == owner) {
        opcode = source->opcode;
        break;
      }
    }
    csound->UnlockMutex(sources->mutex);
    return opcode;
}

#endif
