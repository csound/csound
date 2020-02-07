/*
    find_opcode.h:

    Copyright (C) 2016 by John ffitc

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
#ifndef _FIND_OPCODE_H_
#define _FIND_OPCODE_H_

char* get_opcode_short_name(CSOUND* csound, char* opname);

PUBLIC OENTRY* find_opcode_new(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
PUBLIC OENTRY* find_opcode_exact(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound);
/* find OENTRY with the specified name in opcode list */

OENTRY* find_opcode(CSOUND *, char *);
#endif
