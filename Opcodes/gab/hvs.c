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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <float.h>


/* -------------------------------------------------------------------- */

/*
The iConfigTab is made up of the following parameters:
f #  time size -2  inactive_flag1 inactive_flag2 ... inactiveflagN
a -1 value means that corresponding parameter is left unchanged by the HVS opcode


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

/* Keep both interpolation vertices inside the grid, including at coordinate 1. */
#define HVS_COORDINATE(value, count, pos, frac) do {                       \
    cs_float scaled_ = (value) * ((count) - 1);                              \
    if (scaled_ <= FL(0.0)) {                                            \
      (pos) = 0; (frac) = FL(0.0);                                       \
    } else if (scaled_ >= (count) - 1) {                                  \
      (pos) = (count) - 2; (frac) = FL(1.0);                              \
    } else {                                                            \
      (pos) = (int32_t)scaled_; (frac) = scaled_ - (pos);                  \
    }                                                                   \
} while (0)

typedef struct {
        OPDS    h;
        cs_float   *kx,  *inumParms, *inumPointsX, *iOutTab, *iPositionsTab,
                *iSnapTab, *iConfigTab;
        cs_float   *outTable, *posTable, *snapTable, *confTable;
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
      p->confTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs1(CSOUND *csound, HVS1 *p)
{
    IGN(csound);
    int32_t posX;
    cs_float fracX;
    int32_t noc = (int32_t) *p->inumParms;
    int32_t pointsX = (int32_t) *p->inumPointsX;
    HVS_COORDINATE(*p->kx, pointsX, posX, fracX);

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
            cs_float val1 = p->snapTable[ndx1 * noc + j];
            cs_float val2 = p->snapTable[ndx2 * noc + j];
            cs_float valu = (1 - fracX) * val1 + fracX * val2;
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
        cs_float val1 = p->snapTable[ndx1 * noc + j];
        cs_float val2 = p->snapTable[ndx2 * noc + j];
        cs_float valu = (1 - fracX) * val1 + fracX * val2;
        p->outTable[j] = valu;
      }
    }
    return OK;
}

/* -------------------------------------------------------------------- */

typedef struct {
        OPDS    h;
        cs_float   *kx, *ky, *inumParms, *inumlinesX, *inumlinesY,
                *iOutTab, *iPositionsTab, *iSnapTab, *iConfigTab;
        cs_float   *outTable, *posTable, *snapTable, *confTable;
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
      p->confTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs2(CSOUND *csound, HVS2 *p)
{
    IGN(csound);
    int32_t posX, posY;
    cs_float fracX, fracY;
    int32_t noc = (int32_t) *p->inumParms;
    int32_t linesX = (int32_t) *p->inumlinesX;
    int32_t linesY = (int32_t) *p->inumlinesY;
    HVS_COORDINATE(*p->kx, linesX, posX, fracX);
    HVS_COORDINATE(*p->ky, linesY, posY, fracY);

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
            cs_float val1 = p->snapTable[ndx1 * noc + j];
            cs_float val2 = p->snapTable[ndx2 * noc + j];
            cs_float val3 = p->snapTable[ndx3 * noc + j];
            cs_float val4 = p->snapTable[ndx4 * noc + j];
            cs_float valX1 = (1 - fracX) * val1 + fracX * val2;
            cs_float valX2 = (1 - fracX) * val3 + fracX * val4;
            cs_float valu  = (1 - fracY) * valX1 + fracY * valX2;
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
        cs_float val1 = p->snapTable[ndx1 * noc + j];
        cs_float val2 = p->snapTable[ndx2 * noc + j];
        cs_float val3 = p->snapTable[ndx3 * noc + j];
        cs_float val4 = p->snapTable[ndx4 * noc + j];
        cs_float valX1 = (1 - fracX) * val1 + fracX * val2;
        cs_float valX2 = (1 - fracX) * val3 + fracX * val4;
        cs_float valu  = (1 - fracY) * valX1 + fracY * valX2;
        p->outTable[j] = valu;
      }
    }

    return OK;
}

/* -------------------------------------------------------------------- */


typedef struct {
        OPDS    h;
        cs_float   *kx, *ky, *kz, *inumParms, *inumlinesX, *inumlinesY,
                *inumlinesZ, *iOutTab, *iPositionsTab, *iSnapTab, *iConfigTab;
        cs_float   *outTable, *posTable, *snapTable, *confTable;
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
    if (UNLIKELY(*p->inumlinesX < 2 || *p->inumlinesY < 2 ||
                 *p->inumlinesZ < 2))
      return csound->InitError(csound, "%s",
                              Str("hvs3: each axis must have at least 2 points"));


    if (LIKELY(*p->iConfigTab == 0))
      p->iconfFlag = 0;
    else {
      if ((ftp = csound->FTFind(csound, p->iConfigTab)) == NULL)
        return csound->InitError(csound, "%s", Str("hvs: no config table"));
      p->confTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int32_t hvs3(CSOUND *csound, HVS3 *p)
{
    IGN(csound);
    int32_t posX, posY, posZ;
    cs_float fracX, fracY, fracZ;
    int32_t noc = (int32_t) *p->inumParms;
    int32_t linesX = (int32_t) *p->inumlinesX;
    int32_t linesY = (int32_t) *p->inumlinesY;
    int32_t linesZ = (int32_t) *p->inumlinesZ;
    int32_t linesXY = linesX * linesY;
    HVS_COORDINATE(*p->kx, linesX, posX, fracX);
    HVS_COORDINATE(*p->ky, linesY, posY, fracY);
    HVS_COORDINATE(*p->kz, linesZ, posZ, fracZ);

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
            cs_float   val1 = p->snapTable[ndx1 * noc + j];
            cs_float   val2 = p->snapTable[ndx2 * noc + j];
            cs_float   val3 = p->snapTable[ndx3 * noc + j];
            cs_float   val4 = p->snapTable[ndx4 * noc + j];
            cs_float   valX1 = (1 - fracX) * val1 + fracX * val2;
            cs_float   valX2 = (1 - fracX) * val3 + fracX * val4;
            cs_float   valY1 = (1 - fracY) * valX1 + fracY * valX2;
            cs_float   valY2, valu;

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
        cs_float   val1 = p->snapTable[ndx1 * noc + j];
        cs_float   val2 = p->snapTable[ndx2 * noc + j];
        cs_float   val3 = p->snapTable[ndx3 * noc + j];
        cs_float   val4 = p->snapTable[ndx4 * noc + j];
        cs_float   valX1 = (1 - fracX) * val1 + fracX * val2;
        cs_float   valX2 = (1 - fracX) * val3 + fracX * val4;
        cs_float   valY1 = (1 - fracY) * valX1 + fracY * valX2;
        cs_float   valY2, valu;

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

#undef HVS_COORDINATE

/* -------------------------------------------------------------------- */

typedef struct {
        FUNC *function, *nxtfunction;
        cs_double d;
} TSEG2;

typedef struct {
        OPDS    h;
        cs_float   *kphase, *ioutfunc, *ielements,*argums[VARGMAX];
        TSEG2    *cursegp;
        cs_float *vector;
        int32_t     elements;
        int64_t    nsegs;
        AUXCH   auxch;
} VPSEG;

static int32_t vphaseseg_set(CSOUND *csound, VPSEG *p)
{
    TSEG2 *segp;
    int32_t nsegs, j;
    cs_float **argp = p->argums;
    cs_double durtot = 0.0, position = 0.0;
    FUNC *nxtfunc, *curfunc, *ftp;
    size_t bytes;

    if (UNLIKELY(p->INOCOUNT < 6 || (p->INOCOUNT & 1)))
      return csound->InitError(csound, "%s",
                              Str("vphaseseg: expected table/distance pairs and a final table"));
    nsegs = (p->INOCOUNT - 4) / 2;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ioutfunc)) == NULL))
      return csound->InitError(csound, "%s", Str("vphaseseg: invalid output table"));
    if (UNLIKELY(!(*p->ielements >= FL(0.0) &&
                   (cs_double)*p->ielements <= (INT32_MAX + 0.0) &&
                   (cs_double)*p->ielements <= ftp->flen)))
      return csound->InitError(csound, "%s", Str("vphaseseg: invalid number of elements"));
    p->elements = (int32_t)*p->ielements;
    p->vector = ftp->ftable;

    bytes = (size_t)(nsegs + 1) * sizeof(TSEG2);
    if (p->auxch.auxp == NULL || p->auxch.size < bytes)
      csound->AuxAlloc(csound, bytes, &p->auxch);
    p->cursegp = segp = (TSEG2 *)p->auxch.auxp;
    if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
      return csound->InitError(csound, "%s", Str("vphaseseg: invalid source table"));
    if (UNLIKELY(nxtfunc->flen < (uint32_t)p->elements))
      return csound->InitError(csound, "%s", Str("vphaseseg: source table too short"));
    for (j = 0; j < nsegs; ++j) {
      cs_double dur = **argp++;
      curfunc = nxtfunc;
      if (UNLIKELY(!(dur > 0.0)))
        return csound->InitError(csound, "%s", Str("vphaseseg: distances must be positive"));
      if (UNLIKELY((nxtfunc = csound->FTFind(csound, *argp++)) == NULL))
        return csound->InitError(csound, "%s", Str("vphaseseg: invalid source table"));
      if (UNLIKELY(nxtfunc->flen < (uint32_t)p->elements))
        return csound->InitError(csound, "%s", Str("vphaseseg: source table too short"));
      durtot += dur;
      segp[j].d = dur;
      segp[j].function = curfunc;
      segp[j].nxtfunction = nxtfunc;
    }
    if (UNLIKELY(durtot > DBL_MAX))
      return csound->InitError(csound, "%s", Str("vphaseseg: total distance must be finite"));

    for (j = 0; j < nsegs; ++j) {
      cs_double dur = segp[j].d;
      segp[j].d = position / durtot;
      position += dur;
    }
    /* Avoid leaving a rounding gap below the end of the phase range. */
    segp[nsegs].d = 1.0;
    segp[nsegs].function = segp[nsegs].nxtfunction = nxtfunc;
    p->nsegs = nsegs;
    memset(p->vector, 0, sizeof(cs_float) * p->elements);
    return OK;
}

static int32_t vphaseseg(CSOUND *csound, VPSEG *p)
{
    TSEG2       *segp = p->cursegp;
    cs_double phase = *p->kphase, partialPhase = 0.0;
    int32_t j, flength;
    cs_float   *curtab = NULL, *nxttab = NULL, curval, nxtval, *vector;

    if (phase >= 1.0) phase -= floor(phase);
    else if (phase < 0.0) phase = 0.0;

    for (j = 0; j < p->nsegs; j++) {
      TSEG2 *seg = &segp[j], *seg1 = &segp[j+1];
      if (phase < seg1->d) {
        curtab = seg->function->ftable;
        nxttab = seg1->function->ftable;
        partialPhase = (phase - seg->d) / (seg1->d - seg->d);
        break;
      }
    }
    if (UNLIKELY(curtab == NULL))
      return csound->PerfError(csound, &p->h, "%s", Str("vphaseseg: invalid phase"));

    flength = p->elements;
    vector = p->vector;
    for (j = 0; j < flength; ++j) {
      curval = *curtab++;
      nxtval = *nxttab++;
      *vector++ = (cs_float) (curval + ((nxtval - curval) * partialPhase));
    }
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
