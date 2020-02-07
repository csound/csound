/*
    score_parm.h:

    Copyright (C) 2017 John ffitch

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

#ifndef __SCORE_PARAM_H
#define __SCORE_PARAM_H


typedef struct ListItem {
  double           val;
  struct ListItem *args;
} ListItem;

typedef struct ScoreTree {
  int              op;
  ListItem         *args;
  struct ScoreTree *next;
  int              line;
  int              locn;
} ScoreTree;

#ifndef __PARSE_PARAM_H

#define MAX_INCLUDE_DEPTH 100

typedef struct IFDEFSTACK_ {
    struct IFDEFSTACK_  *prv;
    unsigned char   isDef;      /* non-zero if #ifdef is true, or #ifndef   */
                                /*   is false                               */
    unsigned char   isElse;     /* non-zero between #else and #endif        */
    unsigned char   isSkip;     /* sum of: 1: skipping code due to this     */
                                /*   #ifdef, 2: skipping due to parent      */
} IFDEFSTACK;


#endif

typedef struct prs_parm_s {
    void            *yyscanner;
    CORFIL          *cf;
    MACRO           *macros;
    MACRON          *alt_stack; //[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    unsigned int macro_stack_size;
    IFDEFSTACK      *ifdefStack;
    unsigned char   isIfndef;
    unsigned char   isString;
    unsigned char   isinclude;
    char            *path;
    uint16_t        line;
    uint32_t        locn;
    uint32_t        llocn;
    uint16_t        depth;
    uint16_t        lstack[1024];
         /* Variable for nested repeat loops */
#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */
    char    repeat_name_n[RPTDEPTH][NAMELEN];
    int     repeat_cnt_n[RPTDEPTH];
    int     repeat_indx[RPTDEPTH];
    CORFIL  *cf_stack[RPTDEPTH];
  //int     repeat_inc_n /* = 1 */;
    MACRO   *repeat_mm_n[RPTDEPTH];
    int     repeat_index;
         /* Variables for section repeat */
    int     in_repeat_sect;
    int     repeat_sect_cnt;
    int     repeat_sect_index;
    int     repeat_sect_line;
    CORFIL  *repeat_sect_cf;
    MACRO   *repeat_sect_mm;
} PRS_PARM;

typedef struct scotoken_s {
    int             type;
    int             ival;
    MYFLT           fval;
    char            *strbuff;
} SCOTOKEN;

typedef struct score_parm_s {
    void            *yyscanner;
    int             locn;
    MACRO           *macros;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
    int             ival;
    MYFLT           fval;
    SCOTOKEN        *arglist;
} SCORE_PARM;

uint64_t make_slocation(PRS_PARM *);
extern uint8_t file_to_int(CSOUND*, const char*);

#endif
