/*
    test.c: for cscore

    Copyright (C) 19xx Barry Vercoe, John ffitch

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "cscore.h"                                  /* CSCORE_TEST.C */

void cscore(CSOUND* cs)  /* callable from Csound or standalone cscore */
{
        EVENT *e, **p;
        EVLIST *a, *b, *c;
        int  n;

        e = cscoreCreateEvent(cs,5);            /* alloc p0-5 and init pcnt */
        e->op = 'f';
        e->p[1] = 1;                            /* construct an event */
        e->p[2] = 0;
        e->p[3] = 256;
        e->p[4] = 10;
        e->p[5] = 1;
        cscorePutEvent(cs,e);                   /* and write it out */
        cscoreFreeEvent(cs,e);
        cscorePutString(cs,"s");

        a = cscoreListGetSection(cs);           /* read sect from score */
        cscoreListPut(cs,a);                    /* write it out as is */
        cscorePutString(cs,"s");

        cscoreListSort(cs,a);
        cscoreListPut(cs,a);                    /* write out a sorted version */
        cscorePutString(cs,"s");

        c = cscoreListSeparateTWF(cs,a);        /* separate t,w & f's from notes */
        b = cscoreListCopyEvents(cs,a);         /* duplicate the notes */
        n = b->nevents;
        p = &b->e[1];                           /* point at first one */
        while (n--)
           (*p++)->p[2] += 2.0;                 /* delay each by two beats */
        c = cscoreListAppendList(cs,c,a);
        c = cscoreListAppendList(cs,c,b);       /* combine and sort all 3 sets */
        cscoreListSort(cs,c);
        cscoreListPut(cs,c);                    /* and write out */
        cscoreListFreeEvents(cs,c);             /* release all of these notes */
        cscorePutString(cs,"s");

        e = cscoreDefineEvent(cs,"f 2 0 256 10 5 6 ");
        a = cscoreListCreate(cs,1);
        a = cscoreListAppendEvent(cs,a,e);
        cscoreListPut(cs,a);
        cscoreListFreeEvents(cs,a);
        cscorePutString(cs,"e");

        return;
}
