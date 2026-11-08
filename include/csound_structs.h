/* Plugin-defined orchestra structs. SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef CSOUND_STRUCTS_H
#define CSOUND_STRUCTS_H
#include "csound_type_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *name;
  const char *type; /* e.g. "i", "S", "Type", "Type[]" */
} CSOUND_STRUCT_MEMBER;

typedef struct csstructvar {
  CS_VAR_MEM **members; /* Managed member blocks; may be owned or aliased. */
  int32_t memberCount;
  int32_t ownsMembers; /* Use the type callbacks to copy or free this value. */
} CS_STRUCT_VAR;

/** Register an instance-wide struct during plugin initialization, before
 * compiling orchestra code. Names are global to the Csound instance.
 * Copies the definition; returns its type, owned by Csound until reset, or NULL
 * for an invalid definition or a name already in use. Member types must exist;
 * self-reference is allowed through arrays. No registration during performance.
 * Plugins call csound->RegisterStruct(), not this symbol directly. */
PUBLIC const CS_TYPE *csoundRegisterStruct(CSOUND *, const char *name,
                                         const CSOUND_STRUCT_MEMBER *, size_t);
#ifdef __cplusplus
}
#endif
#endif
