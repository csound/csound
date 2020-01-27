/*
    parse-param.h:

    Copyright (C) 2012 John ffitch

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
#ifndef __PARSE_PARAM_H
#define __PARSE_PARAM_H

typedef struct IFDEFSTACK_ {
    struct IFDEFSTACK_  *prv;
    unsigned char   isDef;      /* non-zero if #ifdef is true, or #ifndef   */
                                /*   is false                               */
    unsigned char   isElse;     /* non-zero between #else and #endif        */
    unsigned char   isSkip;     /* sum of: 1: skipping code due to this     */
                                /*   #ifdef, 2: skipping due to parent      */
} IFDEFSTACK;


typedef struct pre_parm_s {
    void            *yyscanner;
  //MACRO           *macros;
    MACRON          *alt_stack; //[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    unsigned int macro_stack_size;
    IFDEFSTACK      *ifdefStack;
    unsigned char   isIfndef;
    unsigned char   isString;
    uint16_t        line;
    uint64_t        locn;
    uint64_t        llocn;
    uint16_t        depth;
    uint16_t        lstack[1024];
    unsigned char   isinclude;
    char            *path;
} PRE_PARM;

typedef struct parse_parm_s {
    void            *yyscanner;
    uint64_t        locn;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
    uint64_t        iline;      /* Line number for start of instrument */
    uint64_t        ilocn;      /* and location */
} PARSE_PARM;

void    cs_init_math_constants_macros(CSOUND*);
void    cs_init_omacros(CSOUND*, NAMES*);

uint64_t make_location(PRE_PARM *);
extern uint8_t file_to_int(CSOUND*, const char*);

extern void csound_orcput_ilocn(void *, uint64_t, uint64_t);
extern uint64_t csound_orcget_iline(void *);
extern uint64_t csound_orcget_ilocn(void *);
#endif
