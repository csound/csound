/*
 csound_orc_arguments.h:

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

#ifndef MAX_STRUCT_ARG_SIZE
#define MAX_STRUCT_ARG_SIZE (256)
#endif

#ifndef CSOUND_ORC_STRUCTS_H
#define CSOUND_ORC_STRUCTS_H 1

#include "csoundCore.h"

typedef struct csstructvar {
  CS_VAR_MEM** members;
  int* dimensions;
} CS_STRUCT_VAR;

typedef struct initstructvar {
  OPDS h;
  MYFLT* out;
  MYFLT* inArgs[VARGMAX];
} INIT_STRUCT_VAR;

int add_struct_definition(CSOUND*, TREE*);
int findStructMemberIndex(CONS_CELL*, char*);
CS_VARIABLE* getStructMember(CONS_CELL*, char*);
char* getStructPathFromTree(CSOUND*, TREE*);

#endif