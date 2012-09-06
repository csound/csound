/*
    metro.c:

    Copyright (C) 2000 Gabriel Maldonado

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

#include "csdl.h"
#include <math.h>

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *iphs;
        double  curphs;
        int flag;
} METRO;

typedef struct  {
        OPDS    h;
        MYFLT   *trig, *ndx, *maxtics, *ifn, *outargs[VARGMAX];
        int             numouts, currtic, old_ndx;
        MYFLT *table;
} SPLIT_TRIG;

typedef struct  {
        OPDS    h;
        MYFLT   *ktrig, *kphs, *ifn, *args[VARGMAX];
        MYFLT endSeq, *table, oldPhs;
        int numParm, endIndex, prevIndex, nextIndex ;
        MYFLT prevActime, nextActime;
        int initFlag;

} TIMEDSEQ;

static int metro_set(CSOUND *csound, METRO *p)
{
    double phs = *p->iphs;
    int32  longphs;

    if (phs >= 0.0) {
      if (UNLIKELY((longphs = (int32)phs)))
        csound->Warning(csound, Str("metro:init phase truncation"));
      p->curphs = (MYFLT)phs - (MYFLT)longphs;
    }
    p->flag=1;
    return OK;
}

static int metro(CSOUND *csound, METRO *p)
{
    double      phs= p->curphs;
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * csound->onedkr) >= 1.0) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      *p->sr = FL(0.0);
    p->curphs = phs;
    return OK;
}

static int split_trig_set(CSOUND *csound,   SPLIT_TRIG *p)
{

    /* syntax of each table element:
       numtics_elem1,
       tic1_out1, tic1_out2, ... , tic1_outN,
       tic2_out1, tic2_out2, ... , tic2_outN,
       tic3_out1, tic3_out2, ... , tic3_outN,
       .....
       ticN_out1, ticN_out2, ... , ticN_outN,

       numtics_elem2,
       tic1_out1, tic1_out2, ... , tic1_outN,
       tic2_out1, tic2_out2, ... , tic2_outN,
       tic3_out1, tic3_out2, ... , tic3_outN,
       .....
       ticN_out1, ticN_out2, ... , ticN_outN,

    */

    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) == NULL)) {
      return csound->InitError(csound, Str("splitrig: incorrect table number"));
    }
    p->table = ftp->ftable;
    p->numouts =  p->INOCOUNT-4;
    p->currtic = 0;
    return OK;
}

static int split_trig(CSOUND *csound, SPLIT_TRIG *p)
{
    int j;
    int numouts =  p->numouts;
    MYFLT **outargs = p->outargs;

    if (*p->trig) {
      int ndx = (int) *p->ndx * (numouts * (int) *p->maxtics + 1);
      int numtics =  (int) p->table[ndx];
      MYFLT *table = &(p->table[ndx+1]);
      int kndx = (int) *p->ndx;
      int currtic;

      if (kndx != p->old_ndx) {
        p->currtic = 0;
        p->old_ndx = kndx;
      }
      currtic = p->currtic;

      for (j = 0; j < numouts; j++)
        *outargs[j] = table[j +  currtic * numouts ];

      p->currtic = (currtic +1) % numtics;

    }

    else {
      for(j =0; j< numouts; j++)
        *outargs[j] = FL(0.0);
    }
    return OK;
}

static int timeseq_set(CSOUND *csound, TIMEDSEQ *p)
{
    FUNC *ftp;
    MYFLT *table;
    int j;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) == NULL))  return NOTOK;
    table = p->table = ftp->ftable;
    p->numParm = p->INOCOUNT-2; /* ? */
    for (j = 0; j < ftp->flen; j+= p->numParm) {
      if (table[j] < 0) {
        p->endSeq = table[j+1];
        p->endIndex = j/p->numParm;
        break;
      }
    }
    p->initFlag = 1;
    return OK;
}

static int timeseq(CSOUND *csound, TIMEDSEQ *p)
{
    MYFLT *table = p->table, minDist = csound->onedkr;
    MYFLT phs = *p->kphs, endseq = p->endSeq;
    int  j,k, numParm = p->numParm, endIndex = p->endIndex;
    while (phs > endseq)
      phs -=endseq;
    while (phs < 0 )
      phs +=endseq;

    if (p->initFlag) {
    prev:
      for (j=0,k=endIndex; j < endIndex; j++, k--) {
        if (table[j*numParm + 1] > phs ) {
          p->nextActime = table[j*numParm + 1];
          p->nextIndex = j;
          p->prevActime = table[(j-1)*numParm + 1];
          p->prevIndex = j-1;
          break;
        }
        if (table[k*numParm + 1] < phs ) {
          p->nextActime = table[(k+1)*numParm + 1];
          p->nextIndex = k+1;
          p->prevActime = table[k*numParm + 1];
          p->prevIndex = k;
          break;
        }
      }
      if (phs == p->prevActime&& p->prevIndex != -1 )  {
        *p->ktrig = 1;
        for (j=0; j < numParm; j++) {
          *p->args[j]=table[p->prevIndex*numParm + j];
        }
      }
      else if (phs == p->nextActime && p->nextIndex != -1 )  {
        *p->ktrig = 1;
        for (j=0; j < numParm; j++) {
          *p->args[j]=table[p->nextIndex*numParm + j];
        }
      }
      /*p->oldPhs = phs; */
      p->initFlag=0;
    }
    else {
      if (phs > p->nextActime || phs < p->prevActime) {
        for (j=0; j < numParm; j++) {
          *p->args[j]=table[p->nextIndex*numParm + j];
        }
        if (table[p->nextIndex*numParm] != -1) /* if it is not end locator */
          /**p->ktrig = 1; */
          *p->ktrig = table[p->nextIndex*numParm + 3];
        if (phs > p->nextActime) {
          if (p->prevIndex > p->nextIndex && p->oldPhs < phs) {
            /* there is a phase jump */
            *p->ktrig = 0;
            goto fine;
          }
          if (fabs(phs-p->nextActime) > minDist)
            goto prev;

          p->prevActime = table[p->nextIndex*numParm + 1];
          p->prevIndex = p->nextIndex;
          p->nextIndex = (p->nextIndex + 1) % endIndex;
          p->nextActime = table[p->nextIndex*numParm + 1];
        }
        else {
          if (fabs(phs-p->nextActime) > minDist)
            goto prev;

          p->nextActime = table[p->prevIndex*numParm + 1]; /*p->nextActime+1; */
          p->nextIndex = p->prevIndex;
          p->prevIndex = (p->prevIndex - 1);
          if (p->prevIndex < 0) {
            p->prevIndex += p->endIndex;
          }
          p->prevActime = table[p->prevIndex*numParm + 1]; /*p->nextActime+1; */
        }
      }
      else
        *p->ktrig = 0;
    fine:
      p->oldPhs = phs;
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "metro",  S(METRO),   3,      "k", "ko",  (SUBR)metro_set, (SUBR)metro     },
  { "splitrig", S(SPLIT_TRIG), 3, "",  "kkiiz",
                                        (SUBR)split_trig_set, (SUBR)split_trig },
  { "timedseq",S(TIMEDSEQ), TR|3, "k", "kiz", (SUBR)timeseq_set, (SUBR)timeseq }
};

int metro_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

