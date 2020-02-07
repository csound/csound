/*
    ftest.c:

    Copyright (C) 2004,2008 John ffitch, Victor Lazzarini
                  2012 Gleb Rogozinsky

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

//#include "csdl.h"
#include "csoundCore.h"
#include <math.h>

static int32_t tanhtable(FGDATA *ff, FUNC *ftp)
{
    CSOUND *csound =ff->csound;
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    if (ftp->flen <= 0) return csound->ftError(ff, Str("Illegal zero table size"));
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int32_t     i;
    for (i = 0, x = start; i <= (int32_t) ftp->flen; i++, x += step)
      fp[i] = TANH(x);

    if (resc!=FL(0.0)) ff->e.p[4] *= -1;
    /*else ff->e.p[4] = 1;*/
    return OK;
}

static int32_t exptable(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    if (ftp->flen <= 0) return csound->ftError(ff, Str("Illegal zero table size"));
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int32_t     i;

    for (i = 0, x = start; i <= (int32_t) ftp->flen; i++, x += step)
      fp[i] = EXP(x);

    if (resc!=FL(0.0)) ff->e.p[4] *= -1;
    /*else ff->e.p[4] = 1;*/
    return OK;
}

/* Translation table from perceived to actual amplitude */
static int32_t sonetable(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   eqlp  = ff->e.p[7];
    MYFLT   resc  = ff->e.p[8];
    if (ftp->flen <= 0) return csound->ftError(ff, Str("Illegal zero table size"));
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int32_t     i;

    if (eqlp==FL(0.0)) eqlp = FL(0.001);
    /* printf("Sone: %f %f %f %f\n",
       ff->e.p[5], ff->e.p[6], ff->e.p[7], ff->e.p[8]); */
    for (i = 0, x = start; i <= (int32_t) ftp->flen; i++, x += step) {
      fp[i] = x*POWER(x/eqlp, FL(33.0)/FL(78.0));
      //printf("%f -> %f\n", x, fp[i]);
    }

    if (resc!=FL(0.0)) ff->e.p[4] *= -1;
    /*else ff->e.p[4] = 1;*/
    return OK;
}

/* GENwave by Gleb Rogozinsky 2012 */
typedef struct {
  MYFLT        *pWF, *pSF;
  MYFLT        *pFil[2];
  uint32_t *size;
} WAVELET;

static int32_t deconvolve(MYFLT *pInp, WAVELET *pwaveS, uint32_t *pnewLen,
                      MYFLT *pBuf, int32_t *pOrder)
{
    uint32_t i, j;
    *pnewLen *= 2;
    for (j = 0; j < *pnewLen; j++) {
      for (i = 0; i < *pwaveS->size; i++)
        pBuf[(2*j+i) % *pnewLen] += pInp[j]*pwaveS->pFil[*pOrder][i];
    }
    for (i = 0; i < *pnewLen; i++) {
      pInp[i] = pBuf[i];
      pBuf[i] = FL(0.0);
    }
    return OK;
}

static int32_t wavetable(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable;
    MYFLT   *fp_filter, *pInp, *pBuf;
    MYFLT   order = ff->e.p[6];
    MYFLT   resc = ff->e.p[7];
    int32_t ffilno = (int32_t)ff->e.p[5];
    uint32_t     i;
    uint32_t     steps, newLen, *pnewLen;
    int32_t     nargs = ff->e.pcnt - 4;
    int32_t     *pOrder, *xfree;
    FUNC    *srcfil;
    MYFLT   *mirr;
    WAVELET wave, *pwaveS;

    if (ftp->flen <= 0)
      return csound->ftError(ff, Str("Illegal zero table size %d"));
    if (ffilno >csound->maxfnum || csound->flist[ffilno]==NULL)
      return csound->InitError(csound, Str("ftable number does not exist\n"));
    srcfil = csound->flist[ffilno];
    if (UNLIKELY(nargs < 3))
      csound->Warning(csound, Str("insufficient arguments"));
    fp_filter = srcfil->ftable;
    newLen  = srcfil->flen;
    mirr = (MYFLT*) csound->Malloc(csound, sizeof(MYFLT)*srcfil->flen);
    pnewLen = &newLen;
    pwaveS  = &wave;
    pwaveS->pSF  = fp_filter;
    pwaveS->size = &srcfil->flen;
    pwaveS->pWF  = mirr;
    /* create QMF */
    for (i = 0; i < srcfil->flen; i++)
      pwaveS->pWF[i] = POWER(FL(-1.0),i)*pwaveS->pSF[srcfil->flen-1-i];
    pwaveS->pFil[0] = pwaveS->pSF;
    pwaveS->pFil[1] = pwaveS->pWF;
    pInp = (MYFLT*) csound->Calloc(csound, ftp->flen* sizeof(MYFLT));
    pBuf = (MYFLT*) csound->Calloc(csound, ftp->flen* sizeof(MYFLT));
    *pInp = FL(1.0);
    steps = (int32_t)LOG2(ftp->flen/srcfil->flen);
    xfree = pOrder = (int32_t*)csound->Malloc(csound, sizeof(int32_t)*steps);
    /* DEC to BIN */
    for (i = 0; i < steps; i++)
      pOrder[i] = ((int32_t
                    )order>>i) & 0x1;
    /* main loop */
    for (i = 0; i < steps; i++)
      deconvolve(pInp, pwaveS, pnewLen, pBuf, pOrder++);
    for (i = 0; i < *pnewLen; i++)
      fp[i] = pInp[i];
    csound->Free(csound,pBuf); csound->Free(csound,pInp);
    csound->Free(csound,xfree); csound->Free(csound,mirr);
    if (resc!=FL(0.0)) ff->e.p[4] *= -1;
    /*else ff->e.p[4] = 1;*/
    return OK;
}

static NGFENS ftest_fgens[] = {
   { "tanh", tanhtable },
   { "exp", exptable },
   { "sone", sonetable },
   { "wave", wavetable },
   { NULL, NULL }
};

FLINKAGE_BUILTIN(ftest_fgens)
