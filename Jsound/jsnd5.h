#ifndef __JSND_H

#define __JSND_H
#include "tok.h"
typedef struct TREE {
  int           type;
  TOKEN         *value;
  int           rate;
  int		len;
  struct TREE   *left;
  struct TREE   *right;
} TREE;
#include "jsnd5.tab.h"

enum {
  S_ANDTHEN = T_HIGHEST+1,
  S_APPLY
};

#define YYDEBUG 1

TREE* make_node(int, TREE*, TREE*);
TREE* make_leaf(int, TOKEN*);
TOKEN* make_int(char *);
TOKEN* make_num(char *);
void instr0(TOKEN*, TREE*, TREE*);
extern TREE* statement_list;
double get_num(TREE*);
int get_int(TREE*);
TREE* check_opcode(TREE*, TREE*, TREE*);
TREE* check_opcode0(TREE*, TREE*);
void start_instr(int);
extern double sr, kr;
extern int ksmps, nchnls;
#endif
