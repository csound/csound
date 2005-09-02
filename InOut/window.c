/*
    window.c:

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

#include "csoundCore.h"                         /*      WINDOW.C        */
#include "cwindow.h"                            /*  graph window mgr    */
#include "winEPS.h"                             /* PostSCript routines  */
                                                /*  dpwe 16may90        */
/* pointer to window make fn - */
/*     either teletype         */
/*     or some graphics system */
static void (*makeFn)(CSOUND *, WINDAT *, char *);
extern void MakeAscii(CSOUND *, WINDAT *, char *);
extern void MakeGraph(CSOUND *, WINDAT *, char *);
/* pointer to appropriate drawing fn */
static void (*drawFn)(CSOUND *, WINDAT *);
extern void DrawAscii(CSOUND *, WINDAT *);
extern void DrawGraph(CSOUND *, WINDAT *);
/* pointer to window destroy fn */
static void (*killFn)(CSOUND *, WINDAT *);
extern void KillAscii(CSOUND *, WINDAT *);
extern void KillGraph(CSOUND *, WINDAT *);
/* pointer to xyinput window creator */
       void (*mkxyFn)(CSOUND *, XYINDAT *, MYFLT, MYFLT) = NULL;
static void MkXYDummy(CSOUND *, XYINDAT *, MYFLT, MYFLT);
extern void MakeXYin(CSOUND *, XYINDAT *, MYFLT, MYFLT);
/* pointer to xyinput window reader */
       void (*rdxyFn)(CSOUND *, XYINDAT *) = NULL;
static void RdXYDummy(CSOUND *, XYINDAT *);
extern void ReadXYin(CSOUND *, XYINDAT *);

static int (*exitFn)(CSOUND *) = NULL; /* pointer to window last exit fn
                                           - returns 0 to exit immediately */
extern int ExitGraph(CSOUND *);

/* somewhere to invoke for no display */

static void DummyFn2(CSOUND *csound, WINDAT *p, char *s)
{
    IGN(csound); IGN(p); IGN(s);
}

/* somewhere to invoke for no display */

static void DummyFn1(CSOUND *csound, WINDAT *p)
{
    IGN(csound); IGN(p);
}

/* somewhere to invoke that returns 1 (!) for */
/* dummy exit fn */
/* Used to be 1 but seems silly (MR/JPff) */

static int DummyRFn(CSOUND *csound)
{
    IGN(csound); return(0);
}

/* initial proportions */

static void MkXYDummy(CSOUND *csound, XYINDAT *wdptr, MYFLT x, MYFLT y)
{
    IGN(csound);
    wdptr->windid = 0;  /* xwin = MakeWindow(1);        */
    wdptr->down = 0;    /* by def released after Make */

    wdptr->m_x = 0;
    wdptr->m_y = 0;
    wdptr->x = x;       wdptr->y = y;
}

static void RdXYDummy(CSOUND *csound, XYINDAT *wdptr)
{
    IGN(csound);
    IGN(wdptr);
/*      wdptr->m_x = m_x;       wdptr->m_y = m_y;
        wdptr->x = ((MYFLT)m_x-gra_x)/(MYFLT)gra_w;
        wdptr->y = ((MYFLT)m_y-gra_y)/(MYFLT)gra_h;
        */
}

void dispinit(CSOUND *csound) /* called once on initialisation of program to */
{                              /*  choose between teletype or bitmap graphics */
    if (!csound->oparms->displays) {
      csound->Message(csound, Str("displays suppressed\n"));
      makeFn = DummyFn2;
      drawFn = DummyFn1;
      killFn = DummyFn1;
      mkxyFn = MkXYDummy;
      rdxyFn = RdXYDummy;
      exitFn = DummyRFn;
    }
#ifdef WINDOWS
    else if (!csound->oparms->graphsoff && Graphable(csound)) {
                       /* provided by window driver: is this session able? */
      makeFn = MakeGraph;
      drawFn = DrawGraph;
      killFn = KillGraph;
      mkxyFn = MakeXYin;
      rdxyFn = ReadXYin;
      exitFn = ExitGraph;
    }
#endif
    else {
      csound->Message(csound, Str("graphics %s, ascii substituted\n"),
                              (csound->oparms->graphsoff ?
                               Str("suppressed")
                               : Str("not supported on this terminal")));
      makeFn = MakeAscii;
      drawFn = DrawAscii;
      killFn = KillAscii;
      mkxyFn = MkXYDummy;
      rdxyFn = RdXYDummy;
      exitFn = DummyRFn;
    }
}

void dispset(CSOUND *csound,            /* setup a new window       */
             WINDAT *wdptr,             /*   & init the data struct */
             MYFLT  *fdata,
             long   npts,
             char   *caption,
             int    waitflg,
             char   *label)
{
    char *s = caption;
    char *t = wdptr->caption;
    char *tlim = t + CAPSIZE - 1;

    if (!csound->oparms->displays) return;  /* return if displays disabled */
    if (!wdptr->windid) {               /* if no window defined for this str  */
      (*makeFn)(csound, wdptr, label);  /*    create one  */
      if (csound->oparms->postscript)
        PS_MakeGraph(csound, wdptr,label); /* open PS file + write header     */
    }
    wdptr->fdata    = fdata;            /* init remainder of data structure   */
    wdptr->npts     = npts;
    while (*s != '\0' && t < tlim)
        *t++ = *s++;                    /*  (copy the caption) */
    *t = '\0';
    wdptr->waitflg  = waitflg;
    wdptr->polarity = (short)NOPOL;
    wdptr->max      = FL(0.0);
    wdptr->min      = FL(0.0);
    wdptr->absmax   = FL(0.0);
    wdptr->oabsmax  = FL(0.0);
    wdptr->danflag  = 0;
}

int dispexit(CSOUND *csound)
{
    if (csound->oparms->postscript)
      PS_ExitGraph();   /* Write trailer to PostScript file  */
    if (exitFn != NULL)
      /* prompt for exit from last active window */
      return (*exitFn)(csound);
    else
      return 0;
}

void display(CSOUND *csound, WINDAT *wdptr)   /* prepare a MYFLT array, then  */
                                              /*   call the graphing fn       */
{
    MYFLT   *fp, *fplim;
    MYFLT   max, min, absmax, fval;
    int     pol;

    if (!csound->oparms->displays)  return;     /* displays disabled? return */
    fp = wdptr->fdata;
    fplim = fp + wdptr->npts;
    for (max = *fp++, min = max; fp < fplim; ) { /* find max & min values */
      if ((fval = *fp++) > max)       max = fval;
      else if (fval < min)            min = fval;
    }
    absmax = (-min > max )? (-min):max;
    wdptr->max    = max;                 /* record most pos and most */
    wdptr->min    = min;                 /*  neg this array of data  */
    wdptr->absmax = absmax;              /* record absmax this data  */
    if (absmax > wdptr->oabsmax)
      wdptr->oabsmax = absmax;           /* & absmax over life of win */

    pol = wdptr->polarity;     /* adjust polarity flg for life of win */
    if (pol == (short)NOPOL)  {
      if (max > FL(0.0) && min < FL(0.0))      pol = (short)BIPOL;
      else if (max <= FL(0.0) && min <FL(0.0)) pol = (short)NEGPOL;
      else                                     pol = (short)POSPOL;
    }
    else if (pol == (short)POSPOL && min < FL(0.0)) pol = (short)BIPOL;
    else if (pol == (short)NEGPOL && max > FL(0.0)) pol = (short)BIPOL;
    wdptr->polarity = pol;

    (*drawFn)(csound, wdptr);           /* now graph the function */
    if (csound->oparms->postscript)
      PS_DrawGraph(csound, wdptr);      /* Write postscript code  */
}

