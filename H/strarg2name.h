#ifndef CSOUND_STRARG2NAME_H
#define CSOUND_STRARG2NAME_H

#include "csoundCore.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* csound->StringArg2Name for a caller with a fixed-size buffer: 's' is 'size'
   bytes, and a strset string or string p-field that does not fit is cut
   short at it, with a warning. StringArg2Name cannot do this, as it has no
   size to go by, so a name from the score can run off the end of any buffer
   given to it; a caller that cannot bound one must pass NULL and free the
   result. Its rules for a name are repeated here because nothing in the
   library but the function table reaches an opcode built as a plugin. This
   allocates nothing itself, so it can run during performance; only a name
   that is cut, a failure of the score, prints the warning. */

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
    if (src[i] != '\0' && !(pfield && src[i] == '"'))
      csound->Warning(csound,
                      csound->LocalizeString(Str_noop("string too long, "
                                                      "cut to %d characters\n")),
                      (int32_t) i);
    return s;
}

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_STRARG2NAME_H */
