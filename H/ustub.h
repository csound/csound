/*  
    ustub.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

                                                /*  USTUB.H  */

/* header and dummy global references for main.c stub in  */
/* standalone versions of hetro, lpanal, pvanal, sndinfo  */

OPARMS O, O_;                              /* dummy global resolving */
ENVIRON cenviron, cenviron_;                     /*  for references unused */

void fdrecord(FDCH *fdchp) {}
int initerror(char *s) { return NOTOK;}
int perferror(char *s) { return NOTOK;}
void sndwrterr(int n, int nput) {}

int  Graphable_(void);           /* initialise windows.  Returns 1 if X ok */
void MakeGraph_(WINDAT *, char *);       /* create wdw for a graph */
void MakeXYin_(XYINDAT *, MYFLT, MYFLT);
                                /* create a mouse input window; init scale */
void DrawGraph_(WINDAT *);       /* update graph in existing window */
void ReadXYin_(XYINDAT *);       /* fetch latest value from ms input wdw */
void KillGraph_(WINDAT *);       /* remove a graph window */
void KillXYin_(XYINDAT *);       /* remove a ms input window */
int  ExitGraph_(void); /* print click-Exit message in most recently active window */
