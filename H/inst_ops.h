/*
    inst_ops.h:

    Copyright (C) 2025 Victor Lazzarini

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

#ifndef INSTOPS_H
#define INSTOPS_H

#include "csoundCore.h"
#include "aops.h"

typedef struct {
    OPDS    h;
    INSTANCEREF *inst;
    MYFLT  *ktrig;
} KILLOP;
  
typedef struct {                        /* IV - Oct 20 2002 */
    OPDS    h;
    MYFLT   *i_insno, *iname;
} NSTRNUM;

typedef struct {                        /* JPff Feb 2019 */
    OPDS    h;
    STRINGDAT *ans;
    MYFLT     *num;
} NSTRSTR;

typedef struct {
    OPDS    h;
    MYFLT   *insno;
} DELETEIN;

typedef struct {
  OPDS h;
  INSTANCEREF *out;
  INSTREF *in;
} CREATE_INSTANCE;

typedef struct {
  OPDS h;
  MYFLT *err;
  MYFLT *args[VARGMAX];
} INIT_INSTANCE;

typedef struct {
  OPDS h;
  MYFLT *out;
  INSTANCEREF *in;
} PERF_INSTR;

typedef struct {
  OPDS h;
  INSTANCEREF *in;
} DEL_INSTR;

typedef struct {
  OPDS h;
  MYFLT *out;
  INSTANCEREF *in;
  INSTANCEREF *nxt;
  MYFLT *mode;
} SPLICE_INSTR;

typedef struct {
  OPDS h;
  INSTANCEREF *in;
  MYFLT *pause;
} PAUSE_INSTR;

typedef struct {
  OPDS h;
  INSTANCEREF *in;
  MYFLT *par;
  MYFLT *val;
} PARM_INSTR;

int32_t kill_instancek(CSOUND *csound, KILLOP *p);
int32 sa_early(CSOUND *csound, AOP *p);
int32 sa_offset(CSOUND *csound, AOP *p);
int32_t create_instance_opcode(CSOUND *csound, CREATE_INSTANCE *p);
int32_t init_instance_opcode(CSOUND *csound, INIT_INSTANCE *p);
int32_t perf_instance_opcode(CSOUND *csound, PERF_INSTR *p);
int32_t delete_instance_opcode(CSOUND *csound, DEL_INSTR *p);
int32_t pause_instance_opcode(CSOUND *csound, PAUSE_INSTR *p);
int32_t set_instance_parameter(CSOUND *csound, PARM_INSTR *p);
int32_t get_instance(CSOUND *csound, DEL_INSTR *p);
int32_t splice_instance(CSOUND *csound, SPLICE_INSTR *p);

INSDS *instance(CSOUND *, int32_t);
INSDS *create_instance(CSOUND *csound, int32_t insno);
void free_instance(CSOUND *csound, INSDS *ip);
int32_t instr_num(CSOUND *csound, INSTRTXT *instr);
void free_instr_var_memory(CSOUND* csound, INSDS* ip);
int32_t init_instance(CSOUND *csound, INSDS *ip, EVTBLK *newevtp);
int32_t instr_context_check(CSOUND *csound, INSDS *ip, INSDS *insdshead);

#endif
