/* Internal metadata for deprecated opcode diagnostics and maintenance policy.
 * The catalog is H/opcode_deprecations.def; see docs/opcode-deprecation.md.
 * This does not extend OENTRY or the plugin ABI.
 */
#ifndef CSOUND_OPCODE_DEPRECATION_H
#define CSOUND_OPCODE_DEPRECATION_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CS_DEPRECATION_ALIAS,
  CS_DEPRECATION_LEGACY,
  CS_DEPRECATION_FROZEN
} CS_OPCODE_DEPRECATION_POLICY;

typedef struct {
  const char *name;
  const char *replacement; /* NULL: no direct replacement is documented. */
  CS_OPCODE_DEPRECATION_POLICY policy;
  const char *note;
} CS_OPCODE_DEPRECATION;

/* Accepts a public name or an internal name with an overload suffix. */
const CS_OPCODE_DEPRECATION *csoundOpcodeDeprecation(const char *name);

/* Does not emit a message; callers retain their error/warning policy. */
void csoundOpcodeDeprecationMessage(const char *name, int renamed,
                                  char *buffer, size_t size);

#ifdef __cplusplus
}
#endif
#endif
