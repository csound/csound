/*
    namedins.h:

    Copyright (C) 2002 Istvan Varga

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/


/* namedins.h -- written by Istvan Varga, Oct 2002 */

#ifndef CSOUND_NAMEDINS_H
#define CSOUND_NAMEDINS_H

#include "cs.h"         /* for INSTRTXT */

#ifdef __cplusplus
extern "C" {
#endif

/* check if the string s is a valid instrument or opcode name */
/* return value is zero if the string is not a valid name */

int check_instr_name (char*);

/* find the instrument number for the specified name */
/* return value is zero if none was found */

long named_instr_find (char*);

/* allocate entry for named instrument ip with name s (s must not be freed */
/* after the call, because only the pointer is stored); instrument number */
/* is set to insno */
/* returns zero if the named instr entry could not be allocated */
/* (e.g. because it already exists) */

int named_instr_alloc (char*, INSTRTXT*, long);

/* assign instrument numbers to all named instruments */
/* called by otran */

void named_instr_assign_numbers (void);

/* free memory used by named instruments */
/* called by tranRESET() */

void named_instr_free (void);

/* convert opcode string argument to instrument number */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

long strarg2insno (MYFLT*, char*);

/* same as strarg2insno, but runs at perf time, */
/* and does not support numbered instruments */
/* (used by opcodes like event or schedkwhen) */

long strarg2insno_p (char*);

/* convert opcode string argument to instrument number */
/* (also allows user defined opcode names); if the integer */
/* argument is non-zero, only opcode names are searched */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

long strarg2opcno (MYFLT*, char*, int);

/* ----------------------------------------------------------------------- */
/* the following functions are for efficient management of the opcode list */

/* create new opcode list from opcodlst[] */

void opcode_list_create (void);

/* add new entry to opcode list, with optional check for redefined opcodes */

void opcode_list_add_entry (int, int);

/* free memory used by opcode list */

void opcode_list_free (void);

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */

int find_opcode(char*);

/* ----------------------------------------------------------------------- */
/* These functions replace the functionality of strsav() in rdorch.c.      */

/* allocate space for strsav (called once from rdorchfile()) */

void strsav_create(void);

/* Locate string s in database, and return address of stored string (not */
/* necessarily the same as s). If the string is not defined yet, it is   */
/* copied to the database (in such cases, it is allowed to free s after  */
/* the call).                                                            */

char *strsav_string(char*);

/* Free all memory used by strsav space. Called from orchRESET(). */

void strsav_destroy(void);

#ifdef __cplusplus
}
#endif

#endif          /* CSOUND_NAMEDINS_H */

