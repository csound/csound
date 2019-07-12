/*
    csound_orc.h:

    Copyright (C) 2007, 2017 by Stee Yi ad John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef __CSOUND_ORC_H

#define __CSOUND_ORC_H

#define YYDEBUG 1

#include "parse_param.h"
#include "score_param.h"
#include "tok.h"
#include "csound_orcparse.h"
#include "csoundCore.h"

enum {
  S_ANDTHEN = T_HIGHEST+1,
  S_APPLY,
};

typedef struct type_table {
    OENTRY* udos;
    CS_VAR_POOL* globalPool;
    CS_VAR_POOL* instr0LocalPool;
    CS_VAR_POOL* localPool;
    CONS_CELL* labelList;
} TYPE_TABLE;


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

void query_deprecated_opcode(CSOUND *, ORCTOKEN *);
int  query_reversewrite_opcode(CSOUND *, ORCTOKEN *);

    // holds matching oentries from opcodeList
    // has space for 16 matches and next pointer in case more are found
    // (unlikely though)

typedef struct oentries {
      int count;                /* Number of etries in table */
  //char *opname;
  //int prvnum;
      OENTRY* entries[0];       /* Entended by count entries */
    } OENTRIES;

#endif
