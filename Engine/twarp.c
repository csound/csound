/*  
    twarp.c:

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

#include "cs.h"                                       /*    TWARP.C  */

#define TSEGMAX (PMAX/2)

typedef struct {
        MYFLT betbas;
        MYFLT durslp;
        MYFLT durbas;
        MYFLT timbas;
} TSEG;

static TSEG  *tseg, *tpsave, *tplim;
int realtset(SRTBLK *);

void twarp(void)        /* time-warp a score section acc to T-statement */
{
    SRTBLK *bp;
    MYFLT absp3, endtime, realt(MYFLT);
    int negp3;

    if ((bp = frstbp) == NULL)              /* if null file,        */
      return;
    while (bp->text[0] != 't')              /*  or cannot find a t, */
      if ((bp = bp->nxtblk) == NULL)
        return;                             /*      we're done      */
    bp->text[0] = 'w';                      /* else mark the t used  */
    if (!realtset(bp))                      /*  and init the t-array */
      return;                               /* (done if t0 60 or err) */
    bp  = frstbp;
    negp3 = 0;
    do {
      switch(bp->text[0]) {                 /* else warp all timvals */
      case 'i':
        if ((absp3 = bp->newp3) < 0) {
          absp3 = -absp3;
          negp3++;
        }
        endtime = bp->newp2 + absp3;
        bp->newp2 = realt(bp->newp2);
        bp->newp3 = realt(endtime) - bp->newp2;
        if (negp3) {
          bp->newp3 = -bp->newp3;
          negp3--;
        }
        break;
      case 'a':
        endtime = bp->newp2 + bp->newp3;
        bp->newp2 = realt(bp->newp2);
        bp->newp3 = realt(endtime) - bp->newp2;
        break;
      case 'f':
        bp->newp2 = realt(bp->newp2);
        break;
      case 't':
      case 'w':
      case 's':
      case 'e':
        break;
      default:
        err_printf(Str(X_1300,"twarp: illegal opcode\n"));
        break;
      }
    } while ((bp = bp->nxtblk) != NULL);
}

int realtset(SRTBLK *bp)
{
    char *p;
    char c;
    MYFLT tempo, betspan, durbas, avgdur, stof(char*);
    TSEG *tp, *prvtp;

    if (tseg == NULL) {                       /* if no space yet, alloc */
      tseg = (TSEG *) mmalloc((long)TSEGMAX * sizeof(TSEG));
      tplim = tseg + TSEGMAX-1;
    }
    tp = tpsave = tseg;
    if (bp->pcnt < 2)
      goto error1;
    p = bp->text;                             /* first go to p1       */
    p += 2;
    if ((tp->betbas = stof(p)) != 0)          /* betbas1 must be zero */
      goto error1;
    while ((c = *p++) != SP)
      ;
    if ((tempo = stof(p)) <= 0)               /* durbas = 60/tempo    */
      goto error2;
    if (bp->pcnt == 2 && tempo == FL(60.0))   /* just t0 60 means done */
      return(0);
    tp->durbas = FL(60.0)/tempo;
    tp->timbas = FL(0.0);                     /* timbas1 = 0          */
    while ((c = *p++) != SP && c != LF)
      ;
    while (c != LF) {                         /* for each time-tempo pair: */
      prvtp = tp;
      if (++tp > tplim)
        goto error3;
      tp->betbas = stof(p);                   /* betbas = time    */
      while ((c = *p++) != SP && c != LF)
        ;
      if (c == LF)
        goto error1;
      if ((tempo = stof(p)) <= 0)             /* get tempo         */
        goto error2;
      if ((betspan = tp->betbas - prvtp->betbas) <= 0) {
        if (betspan < 0)                      /* if time = lastime */
          goto error1;
        tp--;                                 /* overwrit prvdurbas*/
        tp->durbas = FL(60.0)/tempo;          /*   with 60/tempo   */
        goto align;                           /* and reloop        */
      }
      tp->durbas = durbas = FL(60.0)/tempo;   /* else this durbas  */
      avgdur = (durbas + prvtp->durbas)*FL(0.5);
      tp->timbas = avgdur*betspan + prvtp->timbas;    /* set timbas*/
      prvtp->durslp = (avgdur - prvtp->durbas)/betspan;/* prvdurslp*/
    align:
      while ((c = *p++) != SP && c != LF)
        ;
    }
    tp->durslp = FL(0.0);                     /* clear last durslp */
    if (++tp > tplim)
      goto error3;
    tp->betbas = FL(999999.9);                /* and cap with large betval */
    return(1);

 error1:
    err_printf(Str(X_1301,"twarp: t has extra or disordered beat value\n"));
    return(0);
 error2:
    err_printf(Str(X_1302,"twarp: t has non-positive tempo\n"));
    return(0);
 error3:
    err_printf(Str(X_1303,"twarp: t segments exceed twarp array\n"));
    return(0);
}


MYFLT realt(MYFLT srctim)
{
    TSEG *tp;
    MYFLT diff;

    tp = tpsave;
    while (srctim >= (tp+1)->betbas)
      tp++;
    while ((diff = srctim - tp->betbas) < FL(0.0))
      tp--;
    tpsave = tp;
    return((tp->durslp * diff + tp->durbas) * diff + tp->timbas);
}
