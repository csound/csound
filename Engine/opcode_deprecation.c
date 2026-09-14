/* Shared source for deprecation guidance; no opcode processing changes. */
#include "csoundCore.h"
#include "opcode_deprecation.h"
#include <stdio.h>
#include <string.h>

#define CSOUND_OPCODE_DEPRECATION(name, replacement, policy, note) \
  { name, replacement, CS_DEPRECATION_##policy, note },
static const CS_OPCODE_DEPRECATION deprecations[] = {
#include "opcode_deprecations.def"
};
#undef CSOUND_OPCODE_DEPRECATION

const CS_OPCODE_DEPRECATION *csoundOpcodeDeprecation(const char *name)
{
  size_t lo = 0, hi = sizeof(deprecations) / sizeof(deprecations[0]);
  size_t length;
  if (name == NULL) return NULL;
  length = strcspn(name, ".");
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    int cmp = strncmp(name, deprecations[mid].name, length);
    if (cmp == 0 && deprecations[mid].name[length] != '\0') cmp = -1;
    if (cmp == 0) return &deprecations[mid];
    if (cmp < 0) hi = mid;
    else lo = mid + 1;
  }
  return NULL;
}

void csoundOpcodeDeprecationMessage(const char *name, int renamed,
                                  char *buffer, size_t size)
{
  const CS_OPCODE_DEPRECATION *info = csoundOpcodeDeprecation(name);
  if (info != NULL && info->replacement != NULL) {
    snprintf(buffer, size,
             renamed ? Str("opcode %s has been renamed; use %s instead") :
                       Str("opcode %s is deprecated; use %s instead"),
             info->name, info->replacement);
  }
  else if (info != NULL) {
    snprintf(buffer, size,
             Str("opcode %s is deprecated; no direct replacement is documented"),
             info->name);
  }
  else {
    snprintf(buffer, size,
             renamed ? Str("opcode %s has been renamed"
                           " (uppercase to lowercase / underscores removed)") :
                       Str("opcode %s is deprecated"), name);
  }
}
