/*
    csound_orc_sematics.h:

    Copyright (C) 2013 by Steve Yi

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

#ifndef CSOUND_ORC_SEMANTICS_H
#define CSOUND_ORC_SEMANTICS_H

#include "csound.h"              // for CSOUND, TREE
#include "csound_orc.h"          // for TYPE_TABLE
#include "csound_type_system.h"  // for CS_VAR_MEM

char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);

typedef struct csstructvar {
  CS_VAR_MEM** members;
} CS_STRUCT_VAR;

#endif
