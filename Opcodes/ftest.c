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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

//#include "csdl.h"
#include "csoundCore.h"
#include <math.h>

static int tanhtable(FGDATA *ff, FUNC *ftp)
{
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;
    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step)
      fp[i] = TANH(x);

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    else ff->e.p[4] = 1;
    return OK;
}

static int exptable(FGDATA *ff, FUNC *ftp)
{
 /* CSOUND  *csound = ff->csound; */
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   resc  = ff->e.p[7];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;

    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step)
      fp[i] = EXP(x);

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    else ff->e.p[4] = 1;
    return OK;
}

/* Translation table from perceived to actual amplitude */
static int sonetable(FGDATA *ff, FUNC *ftp)
{
 /* CSOUND  *csound = ff->csound; */
    MYFLT   *fp   = ftp->ftable;
    MYFLT   start = ff->e.p[5];
    MYFLT   end   = ff->e.p[6];
    MYFLT   eqlp  = ff->e.p[7];
    MYFLT   resc  = ff->e.p[8];
    MYFLT   step  = (end - start) / (MYFLT) ftp->flen;
    MYFLT   x;
    int     i;

    if (eqlp==FL(0.0)) eqlp = FL(0.001);
    /* printf("Sone: %f %f %f %f\n",
       ff->e.p[5], ff->e.p[6], ff->e.p[7], ff->e.p[8]); */
    for (i = 0, x = start; i <= (int) ftp->flen; i++, x += step) {
      fp[i] = x*POWER(x/eqlp, FL(33.0)/FL(78.0));
       printf("%f -> %f\n", x, fp[i]); 
    }

    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    else ff->e.p[4] = 1;
    return OK;
}

/* GENwave by Gleb Rogozinsky 2012 */
typedef struct {
  MYFLT        *pWF, *pSF;
  MYFLT        *pFil[2];
  unsigned int *size;
} WAVELET;

static int deconvolve(MYFLT *pInp, WAVELET *pwaveS, unsigned int *pnewLen,
                      MYFLT *pBuf, int *pOrder)
{
    unsigned int i, j;
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

static int wavetable(FGDATA *ff, FUNC *ftp)
{
    CSOUND  *csound = ff->csound;
    MYFLT   *fp = ftp->ftable;
    MYFLT   *fp_filter, *pInp, *pBuf;
    MYFLT   order = ff->e.p[6];
    MYFLT   resc = ff->e.p[7];
    int     ffilno = (int)ff->e.p[5];
    unsigned int     i;
    unsigned int     steps, newLen, *pnewLen;
    int     nargs = ff->e.pcnt - 4;
    int     *pOrder, *xfree;
    FUNC    *srcfil = csound->flist[ffilno];
    MYFLT   *mirr;
    WAVELET wave, *pwaveS;

    if (UNLIKELY(nargs < 3))
      csound->Warning(csound, Str("insufficient arguments"));
    fp_filter = srcfil->ftable;
    newLen  = srcfil->flen;
    mirr = (MYFLT*)malloc(sizeof(MYFLT)*srcfil->flen);
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
    pInp = (MYFLT*)calloc(ftp->flen, sizeof(MYFLT));
    pBuf = (MYFLT*)calloc(ftp->flen, sizeof(MYFLT));
    *pInp = FL(1.0);
    steps = (int)LOG2(ftp->flen/srcfil->flen);
    xfree = pOrder = (int*)malloc(sizeof(int)*steps);
    /* DEC to BIN */
    for (i = 0; i < steps; i++)
      pOrder[i] = ((int)order>>i) & 0x1;
    /* main loop */
    for (i = 0; i < steps; i++)
      deconvolve(pInp, pwaveS, pnewLen, pBuf, pOrder++);
    for (i = 0; i < *pnewLen; i++)
      fp[i] = pInp[i];
    free(pBuf); free(pInp);
    free(xfree); free(mirr);
    if (resc!=FL(0.0)) ff->e.p[4] = -1;
    else ff->e.p[4] = 1;
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
