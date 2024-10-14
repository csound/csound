/*  Copyright (C) 2007 Gabriel Maldonado

  Csound is free software; you can redistribute it
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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"


/* -------------------------------------------------------------------- */

/*
The iConfigTab is made up of the following parameters:
f #  time size -2  inactive_flag1 inactive_flag2 ... inactiveflagN
a 1 value means that corresponding parameter is left unchanged by the HVS opcode


The iPositionsTab is made up of the positions of the snapshots
(contained in the iSnapTab) in the two-dimensional grid. Each
subsequent element is actually a pointer representing the position in
the iSnapTab. For example, in an HVS grid such as the following:

  5 2 1 3
  7 9 6 2
  0 4 1 3

This represents the snapshot position within the grid (in this case a
4x3 grid). So, the first element 5, has index zero and represents the
sixth (element zero is the first) element of the iSnapTab, the second
element 2 represents the third element of iSnapTab and so on.


Obviously, iOutTab size must be >= inumParms.

*/

typedef struct {
        OPDS    h;
        MYFLT   *kx,  *inumParms, *inumPointsX, *iOutTab, *iPositionsTab,
                *iSnapTab, *iConfigTab;
        MYFLT   *outTable, *posTable, *snapTable, *confTable;
        int32_t iconfFlag;
} HVS1;

static int32_t hvs1_set(CSOUND *csound, HVS1 *p)
{
    FUNC        *ftp;

    if (UNLIKELY((ftp = csound->FTFind(csound, p->iOutTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No out table"));
    p->outTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iPositionsTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No positions table"));
    p->posTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iSnapTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No snap table"));
    p->snapTable = ftp->ftable;
    if (UNLIKELY(*p->inumPointsX < 2 ))
      return csound->InitError(csound, "%s", Str("hvs1: a line segment must be "
                                           "delimited by 2 points at least"));

    if (*p->iConfigTab == 0)
      p->iconfFlag = 0;
    else {
      if (UNLIKELY((ftp = csound->FTFind(csound, p->iConfigTab)) == NULL))
        return csound->InitError(csound, "%s", Str("hvs: no config table"));
      p->outTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs1(CSOUND *csound, HVS1 *p)
{
    IGN(csound);
    MYFLT x = *p->kx * (*p->inumPointsX-1);
    int32_t posX = (int32_t) x;

    MYFLT fracX = x - posX;

    int32_t noc = (int32_t) *p->inumParms;
//      int32_t linesX = (int32_t) *p->inumPointsX;

    int32_t ndx1 = (int32_t) p->posTable[posX];
    int32_t ndx2 = (int32_t) p->posTable[posX+1];

    int32_t j;

    if (p->iconfFlag) {
      for (j =0; j< noc; j++) {
        switch ((int32_t) p->confTable[j]) {
        case -1: // ignore parameter
          break;
        case 0: // linear interpolation
          {
            MYFLT val1 = p->snapTable[ndx1 * noc + j];
            MYFLT val2 = p->snapTable[ndx2 * noc + j];
            MYFLT valu = (1 - fracX) * val1 + fracX * val2;
            p->outTable[j] = valu;
          }
          break;
        default:        // special table=shaped interpolations
          ;       // to be implemented...
        }

      }
    }
    else {
      for ( j =0; j< noc; j++) {
        MYFLT val1 = p->snapTable[ndx1 * noc + j];
        MYFLT val2 = p->snapTable[ndx2 * noc + j];
        MYFLT valu = (1 - fracX) * val1 + fracX * val2;
        p->outTable[j] = valu;
      }
    }
    return OK;
}

/* -------------------------------------------------------------------- */

typedef struct {
        OPDS    h;
        MYFLT   *kx, *ky, *inumParms, *inumlinesX, *inumlinesY,
                *iOutTab, *iPositionsTab, *iSnapTab, *iConfigTab;
        MYFLT   *outTable, *posTable, *snapTable, *confTable;
        int32_t iconfFlag;
} HVS2;



static int32_t hvs2_set(CSOUND *csound, HVS2 *p)
{
    FUNC        *ftp;

    if (UNLIKELY((ftp = csound->FTFind(csound, p->iOutTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No out table"));
    p->outTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iPositionsTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No positions table"));
    p->posTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iSnapTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No snap table"));
    p->snapTable = ftp->ftable;
    if (UNLIKELY(*p->inumlinesX < 2 || *p->inumlinesY < 2))
      return csound->InitError(csound, "%s", Str("hvs2: a square area must be "
                                           "delimited by 2 lines at least"));

    if (*p->iConfigTab == 0)
      p->iconfFlag = 0;
    else {
      if (UNLIKELY((ftp = csound->FTFind(csound, p->iConfigTab)) == NULL))
        return csound->InitError(csound, "%s", Str("hvs: no config table"));
      p->outTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs2(CSOUND *csound, HVS2 *p)
{
    IGN(csound);
    MYFLT x = *p->kx * (*p->inumlinesX-1);
    MYFLT y = *p->ky * (*p->inumlinesY-1);
    int32_t posX = (int32_t) x;
    int32_t posY = (int32_t) y;

    MYFLT fracX = x - posX;
    MYFLT fracY = y - posY;

    int32_t noc = (int32_t) *p->inumParms;
    int32_t linesX = (int32_t) *p->inumlinesX;

    int32_t ndx1 = (int32_t) p->posTable[posX   + posY     * linesX];
    int32_t ndx2 = (int32_t) p->posTable[posX+1 + posY     * linesX];
    int32_t ndx3 = (int32_t) p->posTable[posX   + (posY+1) * linesX];
    int32_t ndx4 = (int32_t) p->posTable[posX+1 + (posY+1) * linesX];

    int32_t j;

    if (p->iconfFlag) {
      for ( j =0; j< noc; j++) {
        switch ((int32_t) p->confTable[j]) {
        case -1: // ignore parameter
          break;
        case 0: // linear interpolation
          {
            MYFLT val1 = p->snapTable[ndx1 * noc + j];
            MYFLT val2 = p->snapTable[ndx2 * noc + j];
            MYFLT val3 = p->snapTable[ndx3 * noc + j];
            MYFLT val4 = p->snapTable[ndx4 * noc + j];
            MYFLT valX1 = (1 - fracX) * val1 + fracX * val2;
            MYFLT valX2 = (1 - fracX) * val3 + fracX * val4;
            MYFLT valu  = (1 - fracY) * valX1 + fracY * valX2;
            p->outTable[j] = valu;
          }
          break;
        default:        // special table=shaped interpolations
          ;       // to be implemented...
        }
      }
    }
    else {
      for ( j =0; j< noc; j++) {
        MYFLT val1 = p->snapTable[ndx1 * noc + j];
        MYFLT val2 = p->snapTable[ndx2 * noc + j];
        MYFLT val3 = p->snapTable[ndx3 * noc + j];
        MYFLT val4 = p->snapTable[ndx4 * noc + j];
        MYFLT valX1 = (1 - fracX) * val1 + fracX * val2;
        MYFLT valX2 = (1 - fracX) * val3 + fracX * val4;
        MYFLT valu  = (1 - fracY) * valX1 + fracY * valX2;
        p->outTable[j] = valu;
      }
    }

    return OK;
}

/* -------------------------------------------------------------------- */


typedef struct {
        OPDS    h;
        MYFLT   *kx, *ky, *kz, *inumParms, *inumlinesX, *inumlinesY,
                *inumlinesZ, *iOutTab, *iPositionsTab, *iSnapTab, *iConfigTab;
        MYFLT   *outTable, *posTable, *snapTable, *confTable;
        int32_t iconfFlag;
} HVS3;


static int32_t hvs3_set(CSOUND *csound, HVS3 *p)
{
    FUNC        *ftp;

    if (UNLIKELY((ftp = csound->FTFind(csound, p->iOutTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No out table"));
    p->outTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iPositionsTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No positions table"));
    p->posTable = ftp->ftable;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->iSnapTab)) == NULL))
      return csound->InitError(csound, "%s", Str("hvs: No snap table"));
    p->snapTable = ftp->ftable;
    if (UNLIKELY(*p->inumlinesX < 2 || *p->inumlinesY < 2))
      return csound->InitError(csound, "%s", Str("hvs3: a square area must be "
                                           "delimited by 2 lines at least"));


    if (LIKELY(*p->iConfigTab == 0))
      p->iconfFlag = 0;
    else {
      if ((ftp = csound->FTFind(csound, p->iConfigTab)) == NULL)
        return csound->InitError(csound, "%s", Str("hvs: no config table"));
      p->outTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs3(CSOUND *csound, HVS3 *p)
{
    IGN(csound);
    MYFLT x = *p->kx * (*p->inumlinesX-1);
    MYFLT y = *p->ky * (*p->inumlinesY-1);
    MYFLT z = *p->kz * (*p->inumlinesZ-1);
    int32_t posX = (int32_t) x;
    int32_t posY = (int32_t) y;
    int32_t posZ = (int32_t) z;


    MYFLT fracX = x - posX;
    MYFLT fracY = y - posY;
    MYFLT fracZ = z - posZ;

    int32_t noc         = (int32_t) *p->inumParms;
    int32_t linesX      = (int32_t) *p->inumlinesX;
    int32_t linesXY  = linesX * (int32_t) *p->inumlinesY;

    int32_t ndx1 = (int32_t) p->posTable[posX  +posY    *linesX+posZ*linesXY];
    int32_t ndx2 = (int32_t) p->posTable[posX+1+posY    *linesX+posZ*linesXY];
    int32_t ndx3 = (int32_t) p->posTable[posX  +(posY+1)*linesX+posZ*linesXY];
    int32_t ndx4 = (int32_t) p->posTable[posX+1+(posY+1)*linesX+posZ*linesXY];

    int32_t ndx5 = (int32_t) p->posTable[posX  +posY    *linesX+(posZ+1)*linesXY];
    int32_t ndx6 = (int32_t) p->posTable[posX+1+posY    *linesX+(posZ+1)*linesXY];
    int32_t ndx7 = (int32_t) p->posTable[posX  +(posY+1)*linesX+(posZ+1)*linesXY];
    int32_t ndx8 = (int32_t) p->posTable[posX+1+(posY+1)*linesX+(posZ+1)*linesXY];


    int32_t j;

    if (p->iconfFlag) {
      for ( j =0; j< noc; j++) {
        switch ((int32_t) p->confTable[j]) {
        case -1: // ignore parameter
          break;
        case 0: // linear interpolation
          {
            MYFLT   val1 = p->snapTable[ndx1 * noc + j];
            MYFLT   val2 = p->snapTable[ndx2 * noc + j];
            MYFLT   val3 = p->snapTable[ndx3 * noc + j];
            MYFLT   val4 = p->snapTable[ndx4 * noc + j];
            MYFLT   valX1 = (1 - fracX) * val1 + fracX * val2;
            MYFLT   valX2 = (1 - fracX) * val3 + fracX * val4;
            MYFLT   valY1 = (1 - fracY) * valX1 + fracY * valX2;
            MYFLT   valY2, valu;

            val1 = p->snapTable[ndx5 * noc + j];
            val2 = p->snapTable[ndx6 * noc + j];
            val3 = p->snapTable[ndx7 * noc + j];
            val4 = p->snapTable[ndx8 * noc + j];
            valX1 = (1 - fracX) * val1 + fracX * val2;
            valX2 = (1 - fracX) * val3 + fracX * val4;
            valY2 = (1 - fracY) * valX1 + fracY * valX2;

            valu = (1-fracZ) * valY1 + fracZ * valY2;

            p->outTable[j] = valu;
          }
          break;
        default:        // special table=shaped interpolations
          ;               // to be implemented...
        }
      }
    }
    else {
      for ( j =0; j< noc; j++) {
        MYFLT   val1 = p->snapTable[ndx1 * noc + j];
        MYFLT   val2 = p->snapTable[ndx2 * noc + j];
        MYFLT   val3 = p->snapTable[ndx3 * noc + j];
        MYFLT   val4 = p->snapTable[ndx4 * noc + j];
        MYFLT   valX1 = (1 - fracX) * val1 + fracX * val2;
        MYFLT   valX2 = (1 - fracX) * val3 + fracX * val4;
        MYFLT   valY1 = (1 - fracY) * valX1 + fracY * valX2;
        MYFLT   valY2, valu;

        val1 = p->snapTable[ndx5 * noc + j];
        val2 = p->snapTable[ndx6 * noc + j];
        val3 = p->snapTable[ndx7 * noc + j];
        val4 = p->snapTable[ndx8 * noc + j];
        valX1 = (1 - fracX) * val1 + fracX * val2;
        valX2 = (1 - fracX) * val3 + fracX * val4;
        valY2 = (1 - fracY) * valX1 + fracY * valX2;

        valu = (1-fracZ) * valY1 + fracZ * valY2;

        p->outTable[j] = valu;
      }
    }
    return OK;
}

/* -------------------------------------------------------------------- */

typedef struct {
        FUNC *function, *nxtfunction;
        double d;
} TSEG2;

typedef struct {
        OPDS    h;
        MYFLT   *kphase, *ioutfunc, *ielements,*argums[VARGMAX];
        TSEG2    *cursegp;
        MYFLT *vector;
        int32_t     elements;
        int64_t    nsegs;
        AUXCH   auxch;
} VPSEG;

static int32_t vphaseseg_set(CSOUND *csound, VPSEG *p)
{
    TSEG2       *segp;
    int32_t nsegs,j;
    MYFLT       **argp;
    double dur, durtot = 0.0, prevphs;
    FUNC *nxtfunc, *curfunc, *ftp;

    nsegs = p->nsegs =((p->INCOUNT-3) >> 1);    /* count segs & alloc if nec */

    if ((segp = (TSEG2 *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (int64_t)(nsegs+1)*sizeof(TSEG2), &p->auxch);
      p->cursegp = segp = (TSEG2 *) p->auxch.auxp;
      //(segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
      return csound->InitError(csound,
                               "%s", Str("vphaseseg: the first function is "
                                   "invalid or missing"));
    if (LIKELY((ftp = csound->FTFind(csound, p->ioutfunc)) != NULL)) {
      p->vector = ftp->ftable;
      p->elements = (int32_t) *p->ielements;
    }
    else return csound->InitError(csound, "%s", Str("Failed to find ftable"));
    if (UNLIKELY(p->elements > (int32_t)ftp->flen))
      return csound->InitError(csound,
                               "%s", Str("vphaseseg: invalid num. of elements"));
    /* vector = p->vector; */
    /* flength = p->elements; */

    memset(p->vector, 0, sizeof(MYFLT)*p->elements);
    /* do      *vector++ = FL(0.0); */
    /* while (--flength); */

    if (UNLIKELY(**argp <= 0.0))  return NOTOK; /* if idur1 <= 0, skip init  */
    //p->cursegp = tempsegp =segp;              /* else proceed from 1st seg */

    segp--;
    do {
      segp++;                 /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
        return csound->InitError(csound,
                                 "%s", Str("vphaseseg: function invalid or missing"));
      if (LIKELY(dur > 0.0f)) {
        durtot+=dur;
        segp->d = dur; // ekr;
        segp->function = curfunc;
        segp->nxtfunction = nxtfunc;
        //segp->cnt = (int64_t) (segp->d + .5);
      }
      else break;             /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;

    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    nsegs = (int32_t) p->nsegs;

    segp = p->cursegp;

    for (j=0; j< nsegs; j++)
      segp[j].d /= durtot;

    /* This could be a memmove */
    for (j=nsegs-1; j>= 0; j--)
      segp[j+1].d = segp[j].d;

    segp[0].d = prevphs = 0.0;

    for (j=0; j<= nsegs; j++) {
      segp[j].d += prevphs;
      prevphs = segp[j].d;
    }
    return OK;

}

static int32_t vphaseseg(CSOUND *csound, VPSEG *p)
{
    IGN(csound);
    TSEG2       *segp = p->cursegp;
    double phase = *p->kphase, partialPhase = 0.0;
    int32_t j, flength;
    MYFLT   *curtab = NULL, *nxttab = NULL, curval, nxtval, *vector;

    while (phase >= 1.0) phase -= 1.0;
    while (phase < 0.0) phase = 0.0;

    for (j = 0; j < p->nsegs; j++) {
      TSEG2 *seg = &segp[j], *seg1 = &segp[j+1];
      if (phase < seg1->d) {
        curtab = seg->function->ftable;
        nxttab = seg1->function->ftable;
        partialPhase = (phase - seg->d) / (seg1->d - seg->d);
        break;
      }
    }
    if (UNLIKELY(curtab==NULL)) return NOTOK;

    flength = p->elements;
    vector = p->vector;
    do {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (MYFLT) (curval + ((nxtval - curval) * partialPhase));
    } while (--flength);
    return OK;
}
/* -------------------------------------------------------------------- */


#define S(x)    sizeof(x)

OENTRY hvs_localops[] = {
  { "hvs1",  S(HVS1), TB,   "",  "kiiiiio",
    (SUBR)hvs1_set, (SUBR)hvs1, (SUBR)NULL },
  { "hvs2",  S(HVS2), TB,   "",  "kkiiiiiio",
    (SUBR)hvs2_set, (SUBR)hvs2, (SUBR)NULL },
  { "hvs3",  S(HVS3), TB,   "",  "kkkiiiiiiio",
    (SUBR)hvs3_set, (SUBR)hvs3, (SUBR)NULL },
  { "vphaseseg", S(VPSEG), TB,   "",  "kiim",
    (SUBR)vphaseseg_set, (SUBR)vphaseseg }
};


int32_t hvs_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(hvs_localops[0]),
                                 (int32_t) (sizeof(hvs_localops) / sizeof(OENTRY)));
}

