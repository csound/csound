/* Internal typed p-fields and the Csound 7 string-code adapter. */
#ifndef CSOUND_PFIELD_H
#define CSOUND_PFIELD_H

#include "csoundCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CS_PFIELD_STRING_LIMIT 65536U

typedef enum { CS_PFIELD_NUMBER, CS_PFIELD_STRING } CS_PFIELD_KIND;

typedef struct {
  CS_PFIELD_KIND kind;
  union {
    cs_float number;
    uint32_t string_index;
  } value;
} CS_PFIELD;

/* Fields include p0. Strings are owned, with checked offsets and lengths.
   This is internal: neither its layout nor its size is part of the ABI. */
typedef struct {
  size_t count;
  CS_PFIELD *fields;
  uint32_t string_count;
  size_t string_bytes;
  size_t string_capacity;
  size_t offset_capacity;
  size_t *offsets;
  char *strings;
} CS_PFIELDS;

/* All mutating calls run under the caller's event/instance synchronization.
   Create/import own their data; destroy accepts NULL. SetString copies text.
   Export creates a separate owned legacy event; release it with FreeEvent.
   A numeric NaN that aliases a string code cannot cross the legacy adapter
   alongside strings. The typed representation itself has no such limit. */
CS_PFIELDS *csoundPFieldsCreate(CSOUND *, size_t count);
void csoundPFieldsDestroy(CSOUND *, CS_PFIELDS *);
int32_t csoundPFieldsSetString(CSOUND *, CS_PFIELDS *, size_t, const char *);
const char *csoundPFieldsString(const CS_PFIELDS *, size_t);
CS_PFIELDS *csoundPFieldsImport(CSOUND *, const EVTBLK *);
int32_t csoundPFieldsExport(CSOUND *, const CS_PFIELDS *, EVTBLK *);
void csoundPFieldsFreeEvent(CSOUND *, EVTBLK *);

/* Legacy strings must be valid NUL-terminated buffers supplied by the caller;
   EVTBLK has no byte extent. A zero count with a buffer means one string, as
   in older table generators. New callers must supply the actual count. */
int32_t csoundPFieldStringIndex(cs_float, uint32_t *);
/* Encoding requires an index below CS_PFIELD_STRING_LIMIT. */
cs_float csoundPFieldStringCode(uint32_t);
const char *csoundEventStringArg(const EVTBLK *, cs_float);
const char *csoundEventPFieldString(const EVTBLK *, int32_t);
size_t csoundEventStringBytes(const EVTBLK *);

/* Instance storage is independent of the caller's event. UDO lookup follows
   the same parent chain as the legacy GetArgString call. Reset runs only
   after deinit; ties and reinit retain storage until it is replaced. */
int32_t csoundInstancePFields(CSOUND *, INSDS *, const EVTBLK *);
int32_t csoundInstancePFieldsTake(CSOUND *, INSDS *, CS_PFIELDS *);
const char *csoundInstanceStringArg(const INSDS *, cs_float);
void csoundResetInstancePFields(CSOUND *, INSDS *);
void csoundFreeInstancePrivate(CSOUND *, INSDS *);

#ifdef __cplusplus
}
#endif
#endif
