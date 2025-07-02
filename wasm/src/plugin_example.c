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

#include "csound.h"
#include <csdl.h>

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
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(offset))
    memset(out, '\0', offset * sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out[nsmps], '\0', early * sizeof(MYFLT));
  }

  for (n = offset; n < nsmps; n++)
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

static OENTRY localops[] = {
    {"mult.aa", sizeof(MULT), 0, "a", "aa", NULL, (SUBR)mult_vector},
    {"mult.kk", sizeof(MULT), 0, "k", "kk", NULL, (SUBR)mult_scalar},
    {"mult.ii", sizeof(MULT), 0, "i", "ii", (SUBR)mult_scalar, NULL}};

LINKAGE
