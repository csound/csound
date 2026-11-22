/*
    csound_pre_shared.h:

    Shared preprocessor helpers for the orchestra and score macro preprocessors.

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifndef CSOUND_PRE_SHARED_H
#define CSOUND_PRE_SHARED_H

#include "csoundCore.h"

typedef struct IFDEFSTACK_ {
    struct IFDEFSTACK_  *prv;
    unsigned char   isDef;      /* non-zero if #ifdef is true, or #ifndef   */
                                /*   is false                               */
    unsigned char   isElse;     /* non-zero between #else and #endif        */
    unsigned char   isSkip;     /* sum of: 1: skipping code due to this     */
                                /*   #ifdef, 2: skipping due to parent      */
} IFDEFSTACK;

/* Zero-initialized state starts in orchestra code. Return nonzero for an
   unquoted closing brace. The state can be carried across input chunks. */
int32_t csound_orc_scan_char(ORC_DELIMITER_SCAN *scan, char c);
int32_t csound_orc_scan_in_code(const ORC_DELIMITER_SCAN *scan);

int csound_pre_is_dir(const char *path);
void csound_pre_free_macro(CSOUND *csound, MACRO *mm);
void csound_pre_delete_macro_list(CSOUND *csound, MACRO **head);
MACRO *csound_pre_find_definition(MACRO *mmo, char *s);
int csound_pre_undef_macro_quiet(CSOUND *csound, MACRO **head,
                                 const char *name);
void csound_pre_init_cli_macros(CSOUND *csound, MACRO **head, NAMES *nn,
                                const char *opt_name);
IFDEFSTACK *csound_pre_push_ifdef(CSOUND *csound, IFDEFSTACK **stack,
                                  MACRO *macros, const char *name,
                                  unsigned char isIfndef);
void csound_pre_free_ifdef_stack(CSOUND *csound, IFDEFSTACK **stack);

#endif /* CSOUND_PRE_SHARED_H */
