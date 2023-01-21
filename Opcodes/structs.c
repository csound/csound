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
    CS_VAR_MEM*** members; // CS_STRUCT_VAR
    MYFLT*        nths[1];
} STRUCT_GET;

typedef struct {
    OPDS          h;
    MYFLT*        out;
    CS_VAR_MEM*** members; // CS_STRUCT_VAR
    MYFLT*        nths[1];
} STRUCT_SET;



static int32_t struct_member_get(CSOUND *csound, STRUCT_GET *p)
{
    // this nth-index will have been verified to exist
    int nthInt = (int) *p->nths[0];
    CS_VAR_MEM** members = *p->members;
    CS_VAR_MEM* member = members[nthInt];
    printf("p %p p->out %p\n", p, p->out);
    member->varType->copyValue(csound, member->varType, p->out, &member->value);
    return OK;
}

static int32_t struct_member_set(CSOUND *csound, STRUCT_SET *p)
{
    int nthInt = (int) *p->nths[0];
    CS_VAR_MEM** members = *p->members;
    CS_VAR_MEM* member = members[nthInt];
        printf("p %p p->out %p\n", p, p->out);
    member->varType->copyValue(csound, member->varType, p->out, &member->value);
    return OK;
}

static OENTRY structops_localops[] = {
  { "##member_get", sizeof(STRUCT_GET),    0, 1, ".", ".c",  (SUBR)struct_member_get },
  { "##member_set", sizeof(STRUCT_SET),    0, 1, ".",  ".c", (SUBR)struct_member_set }
};


LINKAGE_BUILTIN(structops_localops)
