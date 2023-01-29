/*
    structs.c:

    Copyright (C) 2023

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

#include "csoundCore.h"
#include "interlocks.h"

typedef struct structvar {
  CS_VAR_MEM** members;
  int* dimensions[VARGMAX];
} STRUCT_VAR;

typedef struct {
    OPDS          h;
    MYFLT*        out;
    STRUCT_VAR*   var;
    MYFLT*        nths[1];
} STRUCT_GET;

typedef struct {
    OPDS          h;
    STRUCT_VAR*   var;
    MYFLT*        nths[1];
    MYFLT*        in;
} STRUCT_SET;

typedef struct {
    OPDS          h;
    STRUCT_VAR*   var;
    MYFLT*        nths[1];
    ARRAYDAT*     in;
} STRUCT_MEMBER_ARRAY_ASSIGN;

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;

static int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p)
{
    // this nth-index will have been verified to exist
    STRUCT_VAR* var = p->var;
    int nthInt = (int) *p->nths[0];
    CS_VAR_MEM* member = var->members[nthInt];
    int memberDimensions = *var->dimensions[nthInt];

    if (memberDimensions > 0) {
        ARRAYDAT* arraySrc = (ARRAYDAT*) &member->value;
        ARRAYDAT* arrayDst = (ARRAYDAT*) p->out;
        arrayDst->allocated = arraySrc->allocated;
        arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
        arrayDst->data = arraySrc->data;
        arrayDst->dimensions = arraySrc->dimensions;
        arrayDst->sizes = arraySrc->sizes;
        arrayDst->arrayType = arraySrc->arrayType;
    } else {
        *p->out = member->value;
    }

    return OK;
}

static int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p)
{
    int nthInt = (int) *p->nths[0];
    STRUCT_VAR* var = p->var;
    CS_VAR_MEM* member = var->members[nthInt];
    member->value = *p->in;
    return OK;
}

static int32_t struct_member_array_assign(
    CSOUND *csound, STRUCT_MEMBER_ARRAY_ASSIGN *p
) {
    int nthInt = (int) *p->nths[0];
    STRUCT_VAR* var = p->var;
    CS_VAR_MEM* member = var->members[nthInt];
    ARRAYDAT* dstArr = (ARRAYDAT*) &member->value;
    ARRAYDAT* srcArr = p->in;

    dstArr->dimensions = srcArr->dimensions;
    dstArr->sizes = srcArr->sizes;
    dstArr->arrayMemberSize = srcArr->arrayMemberSize;
    dstArr->data = srcArr->data;
    dstArr->allocated = srcArr->allocated;
    dstArr->arrayType = srcArr->arrayType;

    return OK;
}

static int struct_array_get(
    CSOUND *csound, STRUCT_ARRAY_GET* dat
) {
    ARRAYDAT* arrayDat = dat->arrayDat;
    int index = ((int) *dat->indicies[0]);
    char* mem = (char *) arrayDat->data;
    MYFLT* srcData = (MYFLT*)(mem+index*arrayDat->arrayMemberSize);

    STRUCT_VAR* srcVar = (STRUCT_VAR*) srcData;
    STRUCT_VAR* dstVar = (STRUCT_VAR*) dat->out;

    dstVar->members = srcVar->members;
    *dstVar->dimensions = *srcVar->dimensions;
    return OK;
}


static OENTRY structops_localops[] = {
  { "##member_get", sizeof(STRUCT_GET),    0, 1, ".", ".c",  (SUBR)struct_member_get },
  { "##member_set", sizeof(STRUCT_SET),    0, 1, "",  ".c.", (SUBR)struct_member_set },
  { "##member_set", sizeof(STRUCT_MEMBER_ARRAY_ASSIGN),
    0, 1, "",  ".[]c.[]", (SUBR)struct_member_array_assign },
  { "##array_get_struct", sizeof(STRUCT_ARRAY_GET),
    0, 1, ".", ".[]m", (SUBR)struct_array_get }
};


LINKAGE_BUILTIN(structops_localops)
