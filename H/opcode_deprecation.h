/* Internal metadata for deprecated opcode diagnostics and maintenance policy.
 * H/opcode_deprecations.def is generated from local source descriptors.
 * See docs/opcode-deprecation.md before changing historical behavior.
 * This does not extend OENTRY or the plugin ABI.
 */
#ifndef CSOUND_OPCODE_DEPRECATION_H
#define CSOUND_OPCODE_DEPRECATION_H

#include <stdarg.h>
#include <stddef.h>

#include "csoundCore.h"

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

/* Emits a deprecation warning unless warnings are disabled by message level
   (CS_WARNMSG or CS_NOQQ) or by --no-deprecated-warnings. Shared by the
   library and built-in/plugin opcodes. */
static inline void csoundDeprecationWarning(CSOUND *csound,
                                            const char *fmt, ...)
{
  const OPARMS *oparms = csound->GetOParms(csound);
  va_list args;
  if (oparms->no_deprecated_warnings ||
      (oparms->msglevel & CS_NOQQ) ||
      !(oparms->msglevel & CS_WARNMSG))
    return;
  csound->MessageS(csound, CSOUNDMSG_WARNING, Str("warning: "));
  va_start(args, fmt);
  csound->MessageV(csound, CSOUNDMSG_WARNING, fmt, args);
  va_end(args);
}

#ifdef __cplusplus
}
#endif
#endif
