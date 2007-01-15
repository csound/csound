#ifndef __TOK_H

#define __TOK_H

#include "csoundCore.h"

/* typedef int    (*SUBR)(void *, void *); */

typedef struct ORCTOKEN {
  int           type;
  char          *lexeme;
  int           value;
  double        fvalue;
/*  char          *ins,*outs;
  int           size;*/
  /* SUBR          init,perform,denit; */
  struct ORCTOKEN  *next;
} ORCTOKEN;

extern ORCTOKEN *new_token(CSOUND *, int);

#define HASH_SIZE (4099)

#endif
