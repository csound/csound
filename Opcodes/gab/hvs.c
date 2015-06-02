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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

//#include "csdl.h"
#include "csoundCore.h"
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
        int iconfFlag;
} HVS1;

static int hvs1_set(CSOUND *csound, HVS1 *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTnp2Find(csound, p->iOutTab)) != NULL)
      p->outTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iPositionsTab)) != NULL)
      p->posTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iSnapTab)) != NULL)
      p->snapTable = ftp->ftable;
    if (*p->inumPointsX < 2 )
      return csound->InitError(csound, Str("hvs1: a line segment must be "
                                           "delimited by 2 points at least"));

    if (*p->iConfigTab == 0)
      p->iconfFlag = 0;
    else {
      if ((ftp = csound->FTnp2Find(csound, p->iConfigTab)) != NULL)
        p->outTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int hvs1(CSOUND *csound, HVS1 *p)
{
    MYFLT x = *p->kx * (*p->inumPointsX-1);
    int posX = (int) x;

    MYFLT fracX = x - posX;

    int noc = (int) *p->inumParms;
//      int linesX = (int) *p->inumPointsX;

    int ndx1 = (int) p->posTable[posX];
    int ndx2 = (int) p->posTable[posX+1];

    int j;

    if (p->iconfFlag) {
      for ( j =0; j< noc; j++) {
        switch ((int) p->confTable[j]) {
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
        int iconfFlag;
} HVS2;



static int hvs2_set(CSOUND *csound, HVS2 *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTnp2Find(csound, p->iOutTab)) != NULL)
      p->outTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iPositionsTab)) != NULL)
      p->posTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iSnapTab)) != NULL)
      p->snapTable = ftp->ftable;
    if (*p->inumlinesX < 2 || *p->inumlinesY < 2)
      return csound->InitError(csound, Str("hvs2: a square area must be "
                                           "delimited by 2 lines at least"));

    if (*p->iConfigTab == 0)
      p->iconfFlag = 0;
    else {
      if ((ftp = csound->FTnp2Find(csound, p->iConfigTab)) != NULL) {
        p->outTable = ftp->ftable;
      }
      p->iconfFlag = 1;
    }
    return OK;
}


static int hvs2(CSOUND *csound, HVS2 *p)
{
    MYFLT x = *p->kx * (*p->inumlinesX-1);
    MYFLT y = *p->ky * (*p->inumlinesY-1);
    int posX = (int) x;
    int posY = (int) y;

    MYFLT fracX = x - posX;
    MYFLT fracY = y - posY;

    int noc = (int) *p->inumParms;
    int linesX = (int) *p->inumlinesX;

    int ndx1 = (int) p->posTable[posX   + posY     * linesX];
    int ndx2 = (int) p->posTable[posX+1 + posY     * linesX];
    int ndx3 = (int) p->posTable[posX   + (posY+1) * linesX];
    int ndx4 = (int) p->posTable[posX+1 + (posY+1) * linesX];

    int j;

    if (p->iconfFlag) {
      for ( j =0; j< noc; j++) {
        switch ((int) p->confTable[j]) {
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
        int iconfFlag;
} HVS3;


static int hvs3_set(CSOUND *csound, HVS3 *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTnp2Find(csound, p->iOutTab)) != NULL)
      p->outTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iPositionsTab)) != NULL)
      p->posTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->iSnapTab)) != NULL)
      p->snapTable = ftp->ftable;
    if (*p->inumlinesX < 2 || *p->inumlinesY < 2)
      return csound->InitError(csound, Str("hvs3: a square area must be "
                                           "delimited by 2 lines at least"));


    if (*p->iConfigTab == 0)
      p->iconfFlag = 0;
    else {
      if ((ftp = csound->FTnp2Find(csound, p->iConfigTab)) != NULL)
        p->outTable = ftp->ftable;
      p->iconfFlag = 1;
    }
    return OK;
}


static int hvs3(CSOUND *csound, HVS3 *p)
{
    MYFLT x = *p->kx * (*p->inumlinesX-1);
    MYFLT y = *p->ky * (*p->inumlinesY-1);
    MYFLT z = *p->kz * (*p->inumlinesZ-1);
    int posX = (int) x;
    int posY = (int) y;
    int posZ = (int) z;


    MYFLT fracX = x - posX;
    MYFLT fracY = y - posY;
    MYFLT fracZ = z - posZ;

    int noc         = (int) *p->inumParms;
    int linesX      = (int) *p->inumlinesX;
    int linesXY  = linesX * (int) *p->inumlinesY;

    int ndx1 = (int) p->posTable[posX   + posY     * linesX + posZ * linesXY];
    int ndx2 = (int) p->posTable[posX+1 + posY     * linesX + posZ * linesXY];
    int ndx3 = (int) p->posTable[posX   + (posY+1) * linesX + posZ * linesXY];
    int ndx4 = (int) p->posTable[posX+1 + (posY+1) * linesX + posZ * linesXY];

    int ndx5 = (int) p->posTable[posX   + posY     * linesX + (posZ+1) * linesXY];
    int ndx6 = (int) p->posTable[posX+1 + posY     * linesX + (posZ+1) * linesXY];
    int ndx7 = (int) p->posTable[posX   + (posY+1) * linesX + (posZ+1) * linesXY];
    int ndx8 = (int) p->posTable[posX+1 + (posY+1) * linesX + (posZ+1) * linesXY];


    int j;

    if (p->iconfFlag) {
      for ( j =0; j< noc; j++) {
        switch ((int) p->confTable[j]) {
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
        int     elements;
        long    nsegs;
        AUXCH   auxch;
} VPSEG;

static int vphaseseg_set(CSOUND *csound, VPSEG *p)
{
    TSEG2       *segp;
    int nsegs,j;
    MYFLT       **argp;
    double dur, durtot = 0.0, prevphs;
    FUNC *nxtfunc, *curfunc, *ftp;

    nsegs = p->nsegs =((p->INCOUNT-3) >> 1);    /* count segs & alloc if nec */

    if ((segp = (TSEG2 *) p->auxch.auxp) == NULL) {
      csound->AuxAlloc(csound, (long)(nsegs+1)*sizeof(TSEG2), &p->auxch);
      p->cursegp = segp = (TSEG2 *) p->auxch.auxp;
      //(segp+nsegs)->cnt = MAXPOS;
    }
    argp = p->argums;
    if ((nxtfunc = csound->FTnp2Find(csound, *argp++)) == NULL)
      return csound->InitError(csound,
                               Str("vphaseseg: the first function is "
                                   "invalid or missing"));
    if ((ftp = csound->FTnp2Find(csound, p->ioutfunc)) != NULL) {
      p->vector = ftp->ftable;
      p->elements = (int) *p->ielements;
    }
    else return csound->InitError(csound, Str("Failed to find ftable"));
    if ( p->elements > (int)ftp->flen )
      return csound->InitError(csound,
                               Str("vphaseseg: invalid num. of elements"));
    /* vector = p->vector; */
    /* flength = p->elements; */

    memset(p->vector, 0, sizeof(MYFLT)*p->elements);
    /* do      *vector++ = FL(0.0); */
    /* while (--flength); */

    if (**argp <= 0.0)  return NOTOK;           /* if idur1 <= 0, skip init  */
    //p->cursegp = tempsegp =segp;              /* else proceed from 1st seg */

    segp--;
    do {
      segp++;                 /* init each seg ..  */
      curfunc = nxtfunc;
      dur = **argp++;
      if ((nxtfunc = csound->FTnp2Find(csound, *argp++)) == NULL)
        return csound->InitError(csound,
                                 Str("vphaseseg: function invalid or missing"));
      if (dur > 0.0f) {
        durtot+=dur;
        segp->d = dur; //* ekr;
        segp->function = curfunc;
        segp->nxtfunction = nxtfunc;
        //segp->cnt = (long) (segp->d + .5);
      }
      else break;             /*  .. til 0 dur or done */
    } while (--nsegs);
    segp++;

    segp->function =  nxtfunc;
    segp->nxtfunction = nxtfunc;
    nsegs = p->nsegs;

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

static int vphaseseg(CSOUND *csound, VPSEG *p)
{
    TSEG2       *segp = p->cursegp;
    double phase = *p->kphase, partialPhase = 0.0;
    int j, flength;
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
    if (curtab==NULL) return NOTOK;

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
  { "hvs1",  S(HVS1), TB, 3,  "",  "kiiiiio",
    (SUBR)hvs1_set, (SUBR)hvs1, (SUBR)NULL },
  { "hvs2",  S(HVS2), TB, 3,  "",  "kkiiiiiio",
    (SUBR)hvs2_set, (SUBR)hvs2, (SUBR)NULL },
  { "hvs3",  S(HVS3), TB, 3,  "",  "kkkiiiiiiio",
    (SUBR)hvs3_set, (SUBR)hvs3, (SUBR)NULL },
  { "vphaseseg", S(VPSEG), TB, 3,  "",  "kiim",
    (SUBR)vphaseseg_set, (SUBR)vphaseseg }
};


int hvs_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(hvs_localops[0]),
                                 (int) (sizeof(hvs_localops) / sizeof(OENTRY)));
}

