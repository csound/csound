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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
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

/* Parser nodes can be detached or share children during rewriting. Track
   their allocations directly for worker error recovery, without walking a
   partially rewritten tree. Native parsing keeps its existing ownership. */
static inline void *csoundParserOwn(CSOUND *csound, void *ptr)
{
#ifdef CSOUND_WASI_REALTIME
    return csoundWasiOwnAllocation(csound, ptr, NULL);
#else
    (void) csound;
    return ptr;
#endif
}

void csoundFreeParserVarPool(CSOUND *csound, void *ptr);
static inline CS_VAR_POOL *csoundParserCreateVarPool(CSOUND *csound)
{
    CS_VAR_POOL *pool = csoundCreateVarPool(csound);
#ifdef CSOUND_WASI_REALTIME
    csoundWasiOwnAllocation(csound, pool, csoundFreeParserVarPool);
#endif
    return pool;
}

static inline char *csoundParserStrdup(CSOUND *csound, const char *value)
{
    return (char *)csoundParserOwn(csound, csoundStrdup(csound, value));
}

static inline CONS_CELL *csoundParserCons(CSOUND *csound, void *value,
                                         CONS_CELL *next)
{
    return (CONS_CELL *)csoundParserOwn(csound, cs_cons(csound, value, next));
}


#ifndef PARSER_DEBUG

#define PARSER_DEBUG (0)
#endif

TREE* make_node(CSOUND *, int32_t, uint64_t, int32_t, TREE*, TREE*);
TREE* make_leaf(CSOUND *, int32_t, uint64_t, int32_t, ORCTOKEN*);
ORCTOKEN* make_int(CSOUND *,char *, void*);
ORCTOKEN* make_num(CSOUND *,char *, void *);
ORCTOKEN *make_token(CSOUND *csound, char *s, void *);
TREE* copy_node(CSOUND*, TREE*);
extern void csp_orc_sa_print_list(CSOUND*);

int32_t  query_reversewrite_opcode(CSOUND *, ORCTOKEN *);

/* Diagnostic only; called after execution-rate resolution. */
void csound_orc_dump_templates(CSOUND *, ENGINE_STATE *);


#endif
