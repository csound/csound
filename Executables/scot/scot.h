/*
    scot.h:

    Copyright (C) 1991 Alan deLespinasse

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

/*                                                      SCOT.H       */
/* aldel Jul 91 */

#pragma once

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define PITCHCLASSES 7
#define NOTESPEROCT 12
#define MAXLINE 300
#define NEXTP "!&"

typedef struct
{
  unsigned long num,denom;
} Rat;

typedef struct strlist          /* used for p-fields */
{
  struct strlist *next;
  char str[32];
} Strlist;

typedef struct note
{
  struct note *next;
  double instrum;
  Rat start,                    /* when note starts */
      dur,                      /* length of note (mul by grpmul) */
      lastdur;                  /* duration to carry to next note */
  int octave,                   /* 8=middle c-b */
      pitchclass,               /* c=0, d=1, e=2... */
      accid,                    /* -1=flat, +1=sharp, etc. */
      accmod,                   /* TRUE if note had #,-,n modifiers */
      slur,                     /* 0,1,2,3 */
      tie,                      /* TRUE if note has __ after it */
      written;                  /* TRUE when written to score */
  Strlist *p,                   /* pfields local to this note */
          *carryp;              /* pfields to carry to next note */
} Note;

typedef struct nextp            /* keeps track of !next keywords */
{
  struct nextp *next;
  int src,dst;                  /* !next psrc "pdst" */
} Nextp;

typedef struct macro
{
  struct macro *next;
  char name[33],text[128];      /* [name="text..."] */
} Macro;

typedef struct inst
{
  struct inst *next;
  char *name;
  unsigned number;
  Macro *lmac;                  /* local macros */
} Inst;

typedef struct tempo
{
  struct tempo *next;
  Rat time;
  int val;
} Tempo;

#include <stdlib.h>

static void readinstsec(Inst *,Nextp **,
                        Rat *,Rat *,Rat *,Rat *,Rat *,
                        Note **,Note **,Tempo **,
                        int *,int *,int *,int *,int *,int*,
                        char *);
static void addparam(int,char *,Strlist **);
static char *findparam(int,Strlist **);
static char *readparams(Inst *);
static int applymacs(char **,Inst *);
static char *macval(char *,Inst *);
static void initnote(Note *);
static void readorch(Inst **);
static void readmacros(Macro **);
static void readfunctions(void);
static void readscore(Inst *);
static int expectchar(int);
static int findint(int *);
static int findchar(int *);
static int findonoff(int *);
static int findword(char *);
static void efindword(char *);
static int letterval(int);
static double pitchval(int,int,int,int);
static void writenote(Note *);
static void freenote(Note *);
static void freeps(Strlist *);
static void strlistcopy(Strlist **,Strlist **);
static int getccom(void);
static double ratval(Rat *);
static void ratreduce(Rat *);
static void ratadd(Rat *,Rat *,Rat *);
static void ratmul(Rat *,Rat *,Rat *);
static void ratdiv(Rat *,Rat *,Rat *);
static int ratcmp(Rat *,Rat *);
static void ratass(Rat *,Rat *);
static void scoterror(char *);
static void scotferror(char *);
static void initf(FILE *,FILE *,char *);
static int scotgetc(void);
static void scotungetc(void);
static void reporterrcount(void);

