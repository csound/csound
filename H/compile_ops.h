#include <csoundCore.h>

typedef struct _compile {
  OPDS h;
  MYFLT *res;
  MYFLT *str;
  MYFLT *ktrig;
}COMPILE;

typedef struct _retval {
  OPDS h;
  MYFLT *ret;
} RETVAL;

int compile_orc_i(CSOUND *csound, COMPILE *c);
int compile_str_i(CSOUND *csound, COMPILE *c);
int compile_csd_i(CSOUND *csound, COMPILE *c);
int read_score_i(CSOUND *csound, COMPILE *c);
int eval_str_i(CSOUND *csound, COMPILE *p);
int eval_str_k(CSOUND *csound, COMPILE *p);
int retval_i(CSOUND *csound, RETVAL *p);
int eval_str_k(CSOUND *csound, COMPILE *p);
