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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"                               /*    TWARP.C  */

typedef struct {
    MYFLT   betbas;
    MYFLT   durslp;
    MYFLT   durbas;
    MYFLT   timbas;
} TSEG;

int     realtset(CSOUND *, SRTBLK *);
MYFLT   realt(CSOUND *, MYFLT);

void twarp(CSOUND *csound) /* time-warp a score section acc to T-statement */
{
    SRTBLK  *bp;
    MYFLT   absp3;
    MYFLT   endtime;
    int     negp3;

    if (UNLIKELY((bp = csound->frstbp) == NULL))      /* if null file,         */
      return;
    while (bp->text[0] != 't')              /*  or cannot find a t,  */
      if (UNLIKELY((bp = bp->nxtblk) == NULL))
        return;                             /*      we are done      */
    bp->text[0] = 'w';                      /* else mark the t used  */
    if (!realtset(csound, bp))              /*  and init the t-array */
      return;                               /* (done if t0 60 or err) */
    bp  = csound->frstbp;
    negp3 = 0;
    do {
      switch (bp->text[0]) {                /* else warp all timvals */
      case 'i':
        absp3 = bp->newp3;
        if (UNLIKELY(absp3 < 0)) {
          absp3 = -absp3;
          negp3++;
        }
        endtime = bp->newp2 + absp3;
        bp->newp2 = realt(csound, bp->newp2);
        bp->newp3 = realt(csound, endtime) - bp->newp2;
        if (negp3) {
          bp->newp3 = -bp->newp3;
          negp3--;
        }
        break;
      case 'a':
        endtime = bp->newp2 + bp->newp3;
        bp->newp2 = realt(csound, bp->newp2);
        bp->newp3 = realt(csound, endtime) - bp->newp2;
        break;
      case 'f':
      case 'q':
        bp->newp2 = realt(csound, bp->newp2);
        break;
      case 't':
      case 'w':
        break;
      case 's':
      case 'e':
        if (bp->pcnt > 0)
          bp->newp2 = realt(csound, bp->p2val);
        break;
      default:
        csound->Message(csound, Str("twarp: illegal opcode\n"));
        break;
      }
    } while ((bp = bp->nxtblk) != NULL);
}

int realtset(CSOUND *csound, SRTBLK *bp)
{
    char    *p;
    char    c;
    MYFLT   tempo, betspan, durbas, avgdur, stof(CSOUND *, char *);
    TSEG    *tp, *prvtp;
    TSEG    *tseg = (TSEG*)csound->tseg;

    csound->tseg =
      tseg = (TSEG*)csound->ReAlloc(csound,
                                    tseg, (1+bp->pcnt/2) * sizeof(TSEG));
    //tplim = &tseg[(bp->pcnt/2)];
    //csound->tseglen = 1+bp->pcnt/2;
    tp = (TSEG*) (csound->tpsave = tseg);
    if (UNLIKELY(bp->pcnt < 2))
      goto error1;
    p = bp->text;                             /* first go to p1        */
    p += 2;
    if (UNLIKELY((tp->betbas = stof(csound, p)) != 0))  /* betbas1 must be zero  */
      goto error1;
    while ((c = *p++) != SP) {}
    if (UNLIKELY((tempo = stof(csound, p)) <= 0))       /* durbas = 60/tempo     */
      goto error2;
    if (bp->pcnt == 2 && tempo == FL(60.0))   /* just t0 60 means done */
      return(0);
    tp->durbas = FL(60.0)/tempo;
    tp->timbas = FL(0.0);                     /* timbas1 = 0           */
    while ((c = *p++) != SP && c != LF) {}
    while (c != LF) {                         /* for each time-tempo pair: */
      prvtp = tp;
      ++tp;
      tp->betbas = stof(csound, p);           /* betbas = time         */
      while ((c = *p++) != SP && c != LF) {}
      if (UNLIKELY(c == LF))
        goto error1;
      if (UNLIKELY((tempo = stof(csound, p)) <= 0))     /* get tempo             */
        goto error2;
      if ((betspan = tp->betbas - prvtp->betbas) <= 0) {
        if (UNLIKELY(betspan < 0))            /* if time = lastime */
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
      while ((c = *p++) != SP && c != LF) {}
    }
    tp->durslp = FL(0.0);                     /* clear last durslp */
    ++tp;
    tp->betbas = FL(9223372036854775807.0);   /* and cap with large betval */
    return(1);

 error1:
    csound->Message(csound,Str("twarp: t has extra or disordered beat value\n"));
    return(0);
 error2:
    csound->Message(csound,Str("twarp: t has non-positive tempo\n"));
    return(0);
}

MYFLT realt(CSOUND *csound, MYFLT srctim)
{
    TSEG *tp;
    MYFLT diff;

    tp = (TSEG*) csound->tpsave;
    while (srctim >= (tp+1)->betbas)
      tp++;
    while ((diff = srctim - tp->betbas) < FL(0.0))
      tp--;
    csound->tpsave = tp;
    return ((tp->durslp * diff + tp->durbas) * diff + tp->timbas);
}

