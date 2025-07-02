/*
    opcode.h:

    Copyright (C) 2024 by Victor Lazzarini

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

#pragma once

#include "csoundCore.h"
#include "aops.h"

typedef struct _opinfo {
  OPDS h;
  OPCODEREF *ref;
} OPINFO;

typedef struct _oprun {
  OPDS h;
  MYFLT *args[VARGMAX];
} OPRUN;

typedef struct _oparray {
  OPDS h;
  ARRAYDAT *r;
  OPCODEREF *ref;
  MYFLT  *n;
  MYFLT  *ovl;
} OPARRAY;

int32_t opcode_info(CSOUND *csound, OPINFO *p);
int32_t opcode_ref(CSOUND *csound, ASSIGN *p);
int32_t opcode_object_info(CSOUND *csound, OPINFO *p);
int32_t opcode_delete(CSOUND *csound, AOP *p);
int32_t create_opcode_simple(CSOUND *csound, AOP *p);
int32_t opcode_delete_array(CSOUND *csound, AOP *p);
int32_t create_opcode_array(CSOUND *csound, OPARRAY *p);
int32_t opcode_object_init(CSOUND *csound, OPRUN *p);
int32_t opcode_object_perf(CSOUND *csound, OPRUN *p);
int32_t opcode_run_perf(CSOUND *csound, OPRUN *p);
int32_t opcode_array_perf(CSOUND *csound, OPRUN *p);
int32_t opcode_array_init(CSOUND *csound, OPRUN *p);
int32_t copy_opcode_obj(CSOUND *csound, ASSIGN *p);
int32_t set_opcode_param(CSOUND *csound, AOP *p);
int32_t get_opcode_output(CSOUND *csound, AOP *p); 
