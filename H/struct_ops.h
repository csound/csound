/*
    tok.h:

    Copyright (C) 2025 by Hlöðver Sigurðsson

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

#ifndef CSOUND_STRUCT_OPS_H
#define CSOUND_STRUCT_OPS_H

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"


typedef struct {
    OPDS          h;
    MYFLT*        out;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant) - MUST be last, variable length!
} STRUCT_GET;

typedef struct {
    OPDS          h;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant)
    MYFLT*        in;           // Value to set
} STRUCT_SET;

typedef struct {
    OPDS          h;
    MYFLT*        var;          // Struct variable (will be cast to CS_STRUCT_VAR*)
    MYFLT*        nths[1];      // Member index (constant)
    ARRAYDAT*     in;           // Array to assign
} STRUCT_MEMBER_ARRAY_ASSIGN;

typedef struct {
  OPDS      h;
  CS_STRUCT_VAR* dst;
  CS_STRUCT_VAR* src;
  /* Deferred-free capture of pre-alias destination members */
  CS_VAR_MEM** oldMembers;
  int32_t      oldMemberCount;
  int32_t      oldOwned;
} STRUCT_ALIAS;

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;

int32_t array_set_struct(CSOUND* csound, ARRAY_SET *p);
int32_t struct_array_get(CSOUND *csound, STRUCT_ARRAY_GET* dat);
int32_t struct_member_get_init_and_perf(CSOUND *csound, STRUCT_GET *p);
int32_t struct_member_get_init(CSOUND *csound, STRUCT_GET *p);
int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p);
int32_t struct_member_set_init(CSOUND *csound, STRUCT_SET *p);
int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p);
int32_t struct_member_array_assign(
    CSOUND *csound, STRUCT_MEMBER_ARRAY_ASSIGN *p
);
int32_t struct_member_set_init_and_perf(CSOUND *csound, STRUCT_SET *p);
#endif
