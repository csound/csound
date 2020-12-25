/*
  mult.c: simple multiplying opcodes

  Copyright (C) 2018 Victor Lazzarini
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

#include <csdl.h>
#include "csound.h"

typedef struct {
  OPDS h;
  MYFLT *out, *in1, *in2;
} MULT;


int mult_scalar(CSOUND *csound, MULT *p) {
  *p->out = *p->in1 * *p->in2;
  return OK;
}

int mult_vector(CSOUND *csound, MULT *p) {
  MYFLT *out = p->out;
  MYFLT *in1 = p->in1;
  MYFLT *in2 = p->in2;
  uint32_t    offset = p->h.insdshead->ksmps_offset;
  uint32_t    early  = p->h.insdshead->ksmps_no_end;
  uint32_t    n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

  for(n = offset; n < nsmps; n++)
    out[n] = in1[n] * in2[n];

  return OK;
}

/* The mult opcode is overloaded for
   a, k, and i inputs. For these cases, it is
   recommended to append an identifier extension .
   to the name for debugging purposes (not strictly required).
   For the user, the extension is not used and all
   overloads are called "mult"
*/

/* extern OENTRY localops; */

/* int csoundAppendOpcodes(CSOUND *, const OENTRY *opcodeList, int n); */

static OENTRY localops[] =
  {
   {  "mult.aa", sizeof(MULT), 0, 2, "a", "aa",
     NULL, (SUBR) mult_vector },
   { "mult.kk", sizeof(MULT), 0, 2, "k", "kk",
     NULL, (SUBR) mult_scalar },
   { "mult.ii", sizeof(MULT), 0, 1, "i", "ii",
        (SUBR) mult_scalar, NULL }
  };



// DUMMY MAIN (never called, but is needed)
/* int main (int argc, char *argv[] ) {} */

/* typedef int32_t (*fp_type_32)(CSOUND *, const OENTRY *opcodeList, int32_t n); */
/* typedef void (*fp_type_32_error_msg)(CSOUND *csound, const char *msg, ...); */
/* void csoundErrorMsg(CSOUND *csound, const char *msg, ...); */
/* int csoundAppendOpcodes(CSOUND *, const OENTRY *opcodeList, int n); */
/* fp_type_32_error_msg csoundErrorMsg_ = &csoundErrorMsg; */
/* fp_type_32 csoundAppendOpcodes_ = &csoundAppendOpcodes; */

/* extern int32_t (*loadWasmPluginFromOentries)(CSOUND*, OENTRY*); */

/* void loadWasmPluginFromOentries_(CSOUND*, OENTRY*); */

/* typedef void (*fp_wasm_load)(CSOUND *, OENTRY *); */

/* __attribute__((used)) fp_wasm_load loadWasmPluginFromOentries = &loadWasmPluginFromOentries_; */

/* __attribute__((used)) */
/* void loadWasmPluginFromOentries(CSOUND* csound, OENTRY* opcodeEntry); */

typedef void (*fp_wasm_load)(CSOUND *, OENTRY *);
/* void loadWasmPluginFromOentries_(CSOUND* csound, OENTRY* opcodeEntry); */
fp_wasm_load loadWasmPluginFromOentries;
/* void (*loadWasmPluginFromOentries)(fp_wasm_load); */


/* typedef void (*fp_type_32)(CSOUND *, OENTRY *); */

/* __attribute__((used)) fp_type_32 loadWasmPluginFromOentries = loadWasmPluginFromOentries; */




extern int32_t init(CSOUND *csound) {
  /* fp_type_32 fp32_external = &function_ret_32; */
  printf("addr plugin \n"); //  &loadWasmPluginFromOentries
  /* (loadWasmPluginFromOentries)(csound, &localops[0]); */
  (*loadWasmPluginFromOentries)(csound, &localops[0]);
  return (int32_t) 0;

  /* return csoundAppendOpcodes(csound, &(localops[0]), (int32_t) (sizeof(localops) / sizeof(OENTRY))); */
  /* return csoundAppendOpcode(csound, "mult.kk", sizeof(MULT), 0, 2, "k", "kk", NULL, (SUBR) mult_scalar); */
}
/* LINKAGE; */

/* extern int init(CSOUND *csound) { */
/*   {  "mult.aa", sizeof(MULT), 0,  2,  "a", "aa",          NULL,         (SUBR) mult_vector } */
/*    { "wterrain2", S(WAVETER), TR, 3,  "a", "kkkkkkkkkkk", (SUBR)wtinit, (SUBR)wtPerf }, */
/*          csoundAppendOpcode(csound, opname,    int dsblksiz, f, t,  o, iint,      i,        a, k) */
/*   return csoundAppendOpcode(csound, "mult.aa", sizeof(MULT), 0, 2, "a", "aa", NULL, (SUBR) mult_vector) | */
/*     csoundAppendOpcode(csound, "mult.kk", sizeof(MULT), 0, 2, "k", "kk", NULL, (SUBR) mult_scalar) | */
/*     csoundAppendOpcode(csound, "mult.ii", sizeof(MULT), 0, 1, "i", "ii", (SUBR) mult_scalar, NULL); */
/*   /\* return csound->AppendOpcodes(csound, &(localops[0]), (int32_t) (sizeof(localops) / sizeof(OENTRY))); *\/ */
/* } */
