/*
    metro.c:

    Copyright (C) 2000 Gabriel Maldonado, (C) 2019 Gleb Rogozinsky

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

#include "stdopcod.h"
#include <math.h>

typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *iphs, *kgate;
        double  curphs;
        double  gate;
        int32_t flag;
} METRO;

// METRO2 ADDED BY GLEB ROGOZINSKY Oct 2019
typedef struct {
        OPDS    h;
        MYFLT   *sr, *xcps, *kswng, *iamp, *iphs;
        double  amp2, curphs, curphs2, swng_init;
        int32_t flag, flag2;
} METRO2;
//

typedef struct  {
        OPDS    h;
        MYFLT   *trig, *ndx, *maxtics, *ifn, *outargs[VARGMAX];
        int32_t             numouts, currtic, old_ndx;
        MYFLT *table;
} SPLIT_TRIG;

typedef struct  {
        OPDS    h;
        MYFLT   *ktrig, *kphs, *ifn, *args[VARGMAX];
        MYFLT endSeq, *table, oldPhs;
        int32_t numParm, endIndex, prevIndex, nextIndex ;
        MYFLT prevActime, nextActime;
        int32_t initFlag;

} TIMEDSEQ;

static int32_t metro_set(CSOUND *csound, METRO *p)
{
    double phs = *p->iphs;
    int32  longphs;

    if (phs >= 0.0) {
      if (UNLIKELY((longphs = (int32)phs)))
        csound->Warning(csound, "%s", Str("metro:init phase truncation"));
      p->curphs = (MYFLT)phs - (MYFLT)longphs;
    }
    p->flag=1;
    return OK;
}

static int32_t metro(CSOUND *csound, METRO *p)
{
    double      phs= p->curphs;
    IGN(csound);
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR) >= 1.0) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      *p->sr = FL(0.0);
    p->curphs = phs;
    return OK;
}

/* John ffitch Oct 2021; for beginers */
static int32_t metrobpm(CSOUND *csound, METRO *p)
{
    double      phs= p->curphs;
    IGN(csound);
    p->gate = *p->kgate;
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR/60) >= 1.0) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else if (phs>= p->gate)
      *p->sr = FL(0.0);
    p->curphs = phs;
    return OK;
}

/* GLEB ROGOZINSKY Oct 2019
   Opcode metro2 in addition to 'classic' metro opcode,
   allows swinging with possibiliy of setting its own amplitude value
*/
static int32_t metro2_set(CSOUND *csound, METRO2 *p)
{
    double phs = *p->iphs;
    double swng = *p->kswng;
    int32  longphs;
    p->amp2 = *p->iamp;

    if (phs >= 0.0) {
      if (UNLIKELY((longphs = (int32)phs)))
        csound->Warning(csound, "%s", Str("metro2:init phase truncation"));
      p->curphs = (MYFLT)phs - (MYFLT)longphs;
      p->curphs2 = (MYFLT)phs - (MYFLT)longphs + 1.0 - (MYFLT)swng;
    }
    p->flag = 1;
    p->flag2 = 1;
    p->swng_init = (MYFLT)swng;
    return OK;
}

static int32_t metro2(CSOUND *csound, METRO2 *p)
{
    double      phs= p->curphs;
    double      phs2= p->curphs2;
    double      phs2_init = p->swng_init;
    double      amp2= p->amp2;
    double      swng= *p->kswng;
    IGN(csound);
// MAIN TICK
    if (phs == 0.0 && p->flag) {
      *p->sr = FL(1.0);
      p->flag = 0;
    }
    else if ((phs += *p->xcps * CS_ONEDKR * 0.5) >= 1.0 ) {
      *p->sr = FL(1.0);
      phs -= 1.0;
      p->flag = 0;
    }
    else
      *p->sr = FL(0.0);
    p->curphs = phs;

// SWINGING TICK
    if (phs2 == 0.0 && p->flag2) {
      *p->sr = FL(amp2);
      p->flag2 = 0;
    }
    else if ((phs2 += *p->xcps * CS_ONEDKR * 0.5) >= (1.0 + swng - phs2_init) ) {
      *p->sr = FL(amp2);
      phs2 -= 1.0;
      p->flag2 = 0;
    }
    p->curphs2 = phs2;

    return OK;
}
//

static int32_t split_trig_set(CSOUND *csound,   SPLIT_TRIG *p)
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
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL)) {
      return csound->InitError(csound, "%s", Str("splitrig: incorrect table number"));
    }
    p->table = ftp->ftable;
    p->numouts =  p->INOCOUNT-4;
    p->currtic = 0;
    return OK;
}

static int32_t split_trig(CSOUND *csound, SPLIT_TRIG *p)
{
     IGN(csound);
    int32_t j;
    int32_t numouts =  p->numouts;
    MYFLT **outargs = p->outargs;

    if (*p->trig) {
      int32_t ndx = (int32_t) *p->ndx * (numouts * (int32_t) *p->maxtics + 1);
      int32_t numtics =  (int32_t) p->table[ndx];
      MYFLT *table = &(p->table[ndx+1]);
      int32_t kndx = (int32_t) *p->ndx;
      int32_t currtic;

      if (kndx != p->old_ndx) {
        p->currtic = 0;
        p->old_ndx = kndx;
      }
      currtic = p->currtic;

      for (j = 0; j < numouts; j++)
        *outargs[j] = table[j +  currtic * numouts ];

      p->currtic = (currtic +1) % numtics;

    }

    else { // Maybe a memset?
      for(j =0; j< numouts; j++)
        *outargs[j] = FL(0.0);
    }
    return OK;
}

static int32_t timeseq_set(CSOUND *csound, TIMEDSEQ *p)
{
    FUNC *ftp;
    MYFLT *table;
    uint32_t j;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))  return NOTOK;
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

static int32_t timeseq(CSOUND *csound, TIMEDSEQ *p)
{
     IGN(csound);
    MYFLT *table = p->table, minDist = CS_ONEDKR;
    MYFLT phs = *p->kphs, endseq = p->endSeq;
    int32_t  j,k, numParm = p->numParm, endIndex = p->endIndex;
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
  { "metro",  S(METRO),  0,        "k", "ko",  (SUBR)metro_set, (SUBR)metro    },
  { "metro2", S(METRO2), 0,        "k", "kkpo", (SUBR)metro2_set, (SUBR)metro2  },
  { "metrobpm",S(METRO), 0,        "k", "koO",  (SUBR)metro_set, (SUBR)metrobpm },
  { "splitrig", S(SPLIT_TRIG), 0,  "",  "kkiiz",
                                        (SUBR)split_trig_set, (SUBR)split_trig },
  { "timedseq",S(TIMEDSEQ), TR,  "k", "kiz", (SUBR)timeseq_set, (SUBR)timeseq }
};

int32_t metro_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}
