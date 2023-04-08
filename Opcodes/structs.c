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

typedef struct {
    OPDS          h;
    MYFLT*        out;
    CS_STRUCT_VAR*   var;
    MYFLT*        nths[1];
} STRUCT_GET;

typedef struct {
    OPDS          h;
    CS_STRUCT_VAR*   var;
    MYFLT*        nths[1];
    MYFLT*        in;
} STRUCT_SET;

typedef struct {
    OPDS          h;
    CS_STRUCT_VAR*   var;
    MYFLT*        nths[1];
    ARRAYDAT*     in;
} STRUCT_MEMBER_ARRAY_ASSIGN;

typedef struct {
  OPDS      h;
  MYFLT*    out;
  ARRAYDAT* arrayDat;
  MYFLT*    indicies[VARGMAX];
} STRUCT_ARRAY_GET;

static void struct_array_member_assign(
    ARRAYDAT* arraySrc,
    ARRAYDAT* arrayDst,
    CS_TYPE*  memberVarType
) {
    arrayDst->allocated = arraySrc->allocated;
    arrayDst->arrayMemberSize = arraySrc->arrayMemberSize;
    arrayDst->data = arraySrc->data;
    arrayDst->dimensions = arraySrc->dimensions;
    arrayDst->sizes = arraySrc->sizes;
    arrayDst->arrayType = arraySrc->arrayType;
}

static int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p)
{
    // this nth-index will have been verified to exist
    CS_STRUCT_VAR* varIn = p->var;
    int nthInt = (int) *p->nths[0];
    CS_VAR_MEM* member = varIn->members[nthInt];
    // FIXME: This is a hack to get around the fact that we don't have
    int memberDimensions = 0; //varIn->dimensions[nthInt];
    if (memberDimensions > 0) {
        struct_array_member_assign(
            (ARRAYDAT*) &member->value,
            (ARRAYDAT*) p->out,
            member->varType
        );
    } else {
        if (member->varType == ((CS_TYPE*) &CS_VAR_TYPE_S)) {
            STRINGDAT* strDat = (STRINGDAT*) &member->value;
            strDat->refCount += 1;
        }
        *p->out = member->value;
    }

    return OK;
}


static int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p)
{
    int nthInt = (int) *p->nths[0];
    CS_STRUCT_VAR* var = p->var;
    CS_VAR_MEM* member = var->members[nthInt];
    member->value = *p->in;
    return OK;
}

static int32_t struct_member_array_assign(
    CSOUND *csound, STRUCT_MEMBER_ARRAY_ASSIGN *p
) {
    int nthInt = (int) *p->nths[0];
    CS_STRUCT_VAR* var = p->var;
    CS_VAR_MEM* member = var->members[nthInt];
    struct_array_member_assign(
        (ARRAYDAT*) &member->value,
        p->in,
        member->varType
    );
    return OK;
}

static int struct_array_get(
    CSOUND *csound, STRUCT_ARRAY_GET* dat
) {
    ARRAYDAT* arrayDat = dat->arrayDat;
    int index = ((int) *dat->indicies[0]);
    char* mem = (char *) arrayDat->data;
    CS_STRUCT_VAR* srcVar = (CS_STRUCT_VAR*)(mem+index*arrayDat->arrayMemberSize);
    CS_STRUCT_VAR* dstVar = (CS_STRUCT_VAR*) dat->out;
    dstVar->members = srcVar->members;
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
