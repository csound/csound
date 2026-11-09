#ifndef CSOUND_STRARG2NAME_H
#define CSOUND_STRARG2NAME_H

#include "csoundCore.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* csound->StringArg2Name for a caller with a fixed-size buffer: 's' is 'size'
   bytes, and a strset string or string p-field that does not fit is refused:
   's' comes back empty, with a warning, so that the caller's open fails
   with its ordinary error rather than opening a file named by what fits,
   which might exist and be overwritten. An empty name cannot be opened:
   every path through FileOpen, FindInputFile and FindOutputFile refuses it,
   and a direct fopen fails on it. StringArg2Name cannot do this, as it has
   no size to go by, so a name from the score can run off the end of any
   buffer given to it; a caller that cannot bound one must pass NULL and
   free the result. Its rules for a name are repeated here because nothing
   in the library but the function table reaches an opcode built as a
   plugin. This allocates nothing itself, so it can run during performance;
   only a name that is refused, a failure of the score, prints the warning. */

static inline char *strarg2name_bounded(CSOUND *csound, char *s, size_t size,
                                        void *p, const char *baseName,
                                        int32_t is_string)
{
    const char *src = (const char *) p;
    int32_t pfield = 0;
    size_t i = 0;

    if (!is_string) {
      cs_float value = *((cs_float *) p);
      if (IsStringCode(value)) {
        /* p-field string, unquote */
        src = csound->GetArgString(csound, value);
        if (src == NULL)        /* a string code with no string behind it */
          src = "";
        if (*src == '"')
          src++;
        pfield = 1;
      }
      else {
        int32_t n = (int32_t) ((cs_double) value + (value >= 0 ? 0.5 : -0.5));
        src = (n >= 0 && n <= csound->GetStrsetsMax(csound))
              ? csound->GetStrsets(csound, n) : NULL;
        if (src == NULL) {
          snprintf(s, size, "%s%d", baseName, n);
          return s;
        }
      }
    }
    while (i + 1 < size && src[i] != '\0' && !(pfield && src[i] == '"')) {
      s[i] = src[i];
      i++;
    }
    s[i] = '\0';
    if (src[i] != '\0' && !(pfield && src[i] == '"')) {
      s[0] = '\0';
      csound->Warning(csound,
                      csound->LocalizeString(Str_noop("string too long, name "
                                                      "refused: at most %d "
                                                      "characters fit: "
                                                      "%.40s...\n")),
                      (int32_t) i, src);
    }
    return s;
}

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_STRARG2NAME_H */
