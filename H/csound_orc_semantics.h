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

#include "csoundCore.h"
#include "csound_orc.h"

/** Gets short version of opcode name, trimming off anything after '.'.
 If opname has no '.' in name, simply returns the opname pointer.
 If the name is truncated, caller is responsible for calling mfree
 on returned value.  Caller should compare the returned value with the
 passed in opname to see if it is different and thus requires mfree'ing. */
#include "find_opcode.h"
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);

#endif
