#ifndef __CSOUND_ORC_H

#define __CSOUND_ORC_H
#include "tok.h"
typedef struct TREE {
  int           type;
  ORCTOKEN         *value;
  int           rate;
  int           len;
  int           line;
  int           locn;
  struct TREE   *left;
  struct TREE   *right;
  struct TREE   *next;
} TREE;
#include "csound_orcparse.h"
#include "csoundCore.h"

enum {
  S_ANDTHEN = T_HIGHEST+1,
  S_APPLY,
};

#define YYDEBUG 1
#ifndef PARSER_DEBUG
#define PARSER_DEBUG (0)
#endif

TREE* make_node(CSOUND *, int, int, int, TREE*, TREE*);
TREE* make_leaf(CSOUND *, int, int, int, ORCTOKEN*);
ORCTOKEN* make_int(CSOUND *,char *);
ORCTOKEN* make_num(CSOUND *,char *);
ORCTOKEN *make_token(CSOUND *csound, char *s);
/*void instr0(CSOUND *, ORCTOKEN*, TREE*, TREE*);*/
/* extern TREE* statement_list; */
/* double get_num(TREE*); */
/*int get_int(TREE*);*/
/*TREE* check_opcode(TREE*, TREE*, TREE*);*/
/*TREE* check_opcode0(TREE*, TREE*);*/
/*void start_instr(int);*/
/* extern double sr, kr;
extern int ksmps, nchnls; */
#endif
