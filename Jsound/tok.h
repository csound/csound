#ifndef __TOK_H

#define __TOK_H

typedef int    (*SUBR)(void *, void *);

typedef struct TOKEN {
  int           type;
  char          *lexeme;
  int           value;
  double        fvalue;
  char          *ins,*outs;
  int           size;
  SUBR          init,perform,denit;
  struct TOKEN  *next;
} TOKEN;

extern TOKEN *new_token(int);

#define HASH_SIZE (1024)

#endif
