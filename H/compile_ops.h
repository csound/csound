#include <csoundCore.h>

typedef struct _compile {
  OPDS h;
  MYFLT *res;
  MYFLT *str;
}COMPILE;

int compile_orc_i(CSOUND *csound, COMPILE *c);
int compile_str_i(CSOUND *csound, COMPILE *c);
int read_score_i(CSOUND *csound, COMPILE *c);
