/*
    compile_ops.h:

    Copyright (C) 2013 by Victor Lazzarini

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

typedef struct rosc {
  OPDS h;
  MYFLT *kstatus;
  MYFLT *out[32];
  STRINGDAT *address, *type;
} ROSC;

typedef struct rosca {
  OPDS h;
  MYFLT *kstatus;
  ARRAYDAT *out;
  STRINGDAT *address, *type;
} ROSCA;

typedef struct _cinstr {
  OPDS h;
  INSTREF *instr;
  STRINGDAT *code;
} CINSTR;

typedef struct _carinstr {
  OPDS h;
  STRINGDAT *code;
  MYFLT *argums[VARGMAX-1];
} CARINSTR;

typedef struct _rinstr {
  OPDS h;
  INSTREF *instr;
  MYFLT *argums[VARGMAX-1];
} RINSTR;

typedef struct _rinstrk {
  OPDS h;
  MYFLT *ktrig;
  INSTREF *instr;
  MYFLT *argums[VARGMAX-1];
} RINSTRK;


int32_t compile_orc_i(CSOUND *csound, COMPILE *c);
int32_t compile_str_i(CSOUND *csound, COMPILE *c);
int32_t compile_csd_i(CSOUND *csound, COMPILE *c);
int32_t read_score_i(CSOUND *csound, COMPILE *c);
int32_t eval_str_i(CSOUND *csound, COMPILE *p);
int32_t eval_str_k(CSOUND *csound, COMPILE *p);
int32_t retval_i(CSOUND *csound, RETVAL *p);
int32_t eval_str_k(CSOUND *csound, COMPILE *p);
int32_t readOSC_perf(CSOUND *csound, ROSC *p);
int32_t readOSCarray_perf(CSOUND *csound, ROSCA *p);
int32_t readOSCarray_init(CSOUND *csound, ROSCA *p);
int32_t compile_instr(CSOUND *csound, CINSTR *p);
int32_t compile_and_run_instr(CSOUND *csound, CARINSTR *p); 
int32_t run_instr(CSOUND *csound, RINSTR *p); 
int32_t run_instr_k(CSOUND *csound, RINSTRK *p); 
