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

#include "cs.h"                                 /*      WINDOW.C        */
#include "cwindow.h"                            /*  graph window mgr    */
#include "winEPS.h"                             /* PostSCript routines  */
                                                /*  dpwe 16may90        */
extern OPARMS O;

static void (*makeFn)(WINDAT*, char*);  /* pointer to window make fn - */
extern void MakeAscii(WINDAT *, char *);/*     either teletype         */
extern void MakeGraph(WINDAT *,char *); /*     or some graphics system */

static void (*drawFn)(WINDAT *);        /* pointer to appropriate drawing fn */
extern void DrawAscii(WINDAT *);
extern void DrawGraph(WINDAT *);

static void (*killFn)(WINDAT *);        /* pointer to window destroy fn */
extern void KillAscii(WINDAT *);
extern void KillGraph(WINDAT *);

       void (*mkxyFn)(XYINDAT *, MYFLT, MYFLT); /* pointer to xyinput window creator */
static void MkXYDummy(XYINDAT *, MYFLT, MYFLT);
extern void MakeXYin(XYINDAT *, MYFLT, MYFLT);

       void (*rdxyFn)(XYINDAT *);       /* pointer to xyinput window reader */
static void RdXYDummy(XYINDAT *);
extern void ReadXYin(XYINDAT *);

static int (*exitFn)(void) = NULL; /* pointer to window last exit fn - returns
                                      0 to exit immediately */
extern int ExitGraph(void);

static void DummyFn2(WINDAT *p, char *s) /* somewhere to invoke for no display */
{
    IGN(p); IGN(s);
}

static void DummyFn1(WINDAT *p)          /* somewhere to invoke for no display */
{
    IGN(p);
}

static int DummyRFn(void) /* somewhere to invoke that returns 1 (!) for */
{                         /* dummy exit fn */
    return(0);            /* Used to be 1 but seems silly (MR/JPff) */
}

static void MkXYDummy(XYINDAT *wdptr, MYFLT x, MYFLT y) /* initial proportions */
{
    wdptr->windid = 0;  /* xwin = MakeWindow(1);        */
    wdptr->down = 0;    /* by def released after Make */

    wdptr->m_x = 0;
    wdptr->m_y = 0;
    wdptr->x = x;       wdptr->y = y;
}

static void RdXYDummy(XYINDAT *wdptr)
{
    IGN(wdptr);
/*      wdptr->m_x = m_x;       wdptr->m_y = m_y;
        wdptr->x = ((MYFLT)m_x-gra_x)/(MYFLT)gra_w;
        wdptr->y = ((MYFLT)m_y-gra_y)/(MYFLT)gra_h;
        */
}

void dispinit(void)     /* called once on initialisation of program to  */
{                       /*   choose between teletype or bitmap graphics */
    if (!O.displays) {
      printf(Str(X_705,"displays suppressed\n"));
      makeFn = DummyFn2;
      drawFn = DummyFn1;
      killFn = DummyFn1;
      mkxyFn = MkXYDummy;
      rdxyFn = RdXYDummy;
      exitFn = DummyRFn;
    }
#ifdef WINDOWS
    else if (!O.graphsoff && Graphable()) {
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
      printf(Str(X_820,"graphics %s, ascii substituted\n"),
             (O.graphsoff)? Str(X_1250,"suppressed") :
             Str(X_1078,"not supported on this terminal"));
      makeFn = MakeAscii;
      drawFn = DrawAscii;
      killFn = KillAscii;
      mkxyFn = MkXYDummy;
      rdxyFn = RdXYDummy;
      exitFn = DummyRFn;
    }
}

void dispset(                            /* setup a new window */
     WINDAT *wdptr,                      /*   & init the data struct */
     MYFLT  *fdata,
     long   npts,
     char   *caption,
     int    waitflg,
     char   *label)
{
    char *s = caption;
    char *t = wdptr->caption;
    char *tlim = t + CAPSIZE - 1;

    if (!O.displays) return;            /* return if displays disabled */
    if (!wdptr->windid) {               /* if no window defined for this str  */
      (*makeFn)(wdptr,label);           /*    create one  */
      if (O.postscript)
        PS_MakeGraph(wdptr,label);      /* open PS file + write header        */
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

void dispkill(WINDAT *wdptr)
{
    /**
     * Fixed bug - used to throw an exception if killFn was null.
     */
    if (killFn) {
      (*killFn)(wdptr);
    }
}

int dispexit(void)
{
    if (O.postscript) PS_ExitGraph(); /* Write trailer to PostScript file  */
    if (exitFn!=NULL)
      return (*exitFn)();         /* prompt for exit from last active window */
    else
      return 0;
}

void display(WINDAT *wdptr)     /* prepare a MYFLT array, then call the */
                                /* graphing fn */
{
    MYFLT *fp, *fplim;
    MYFLT       max, min, absmax, fval;
    int         pol;

    if (!O.displays)  return;              /* displays disabled? return */
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

    (*drawFn)(wdptr);                    /* now graph the function */
    if (O.postscript) PS_DrawGraph(wdptr);/* Write postscript code  */
}
