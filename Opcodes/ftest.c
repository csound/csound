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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include <math.h>

static int32_t tanhtable(FGDATA *ff, FUNC *ftp)
{
  CSOUND *csound =ff->csound;
  cs_float   *fp   = ftp->ftable;
  cs_float   start = ff->e.p[5];
  cs_float   end   = ff->e.p[6];
  cs_float   resc  = ff->e.p[7];

  if (ftp->flen <= 0) return csound->FtError(ff, "%s", Str("Illegal zero table size"));
  cs_float   step  = (end - start) / (cs_float) ftp->flen;
  cs_float   x;
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
  cs_float   *fp   = ftp->ftable;
  cs_float   start = ff->e.p[5];
  cs_float   end   = ff->e.p[6];
  cs_float   resc  = ff->e.p[7];

  if (ftp->flen <= 0) return csound->FtError(ff, "%s", Str("Illegal zero table size"));
  cs_float   step  = (end - start) / (cs_float) ftp->flen;
  cs_float   x;
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
  cs_float   *fp   = ftp->ftable;
  cs_float   start = ff->e.p[5];
  cs_float   end   = ff->e.p[6];
  cs_float   eqlp  = ff->e.p[7];
  cs_float   resc  = ff->e.p[8];

  if (ftp->flen <= 0) return csound->FtError(ff, "%s", Str("Illegal zero table size"));
  cs_float   step  = (end - start) / (cs_float) ftp->flen;
  cs_float   x;
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
  cs_float        *pWF, *pSF;
  cs_float        *pFil[2];
  uint32_t *size;
} WAVELET;

static int32_t deconvolve(cs_float *pInp, WAVELET *pwaveS, uint32_t *pnewLen,
                          cs_float *pBuf, int32_t *pOrder)
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
  cs_float   *fp = ftp->ftable;
  cs_float   *fp_filter, *pInp, *pBuf;
  cs_float   order = ff->e.p[6];
  cs_float   resc = ff->e.p[7];
  uint32_t     i;
  uint32_t     steps, newLen, *pnewLen;
  int32_t     nargs = ff->e.pcnt - 4;
  int32_t     *pOrder, *xfree;
  FUNC    *srcfil = csound->FTFind(csound, &(ff->e.p[5]));
  cs_float   *mirr;
  WAVELET wave, *pwaveS;
    

  if (ftp->flen <= 0)
    return csound->FtError(ff, "%s", Str("Illegal zero table size %d"));
  srcfil = csound->FTFind(csound, &ff->e.p[5]);
  if (srcfil==NULL)
    return csound->InitError(csound, "%s", Str("ftable number does not exist\n"));
  if (UNLIKELY(ftp->flen < srcfil->flen))
    return csound->FtError(ff, "%s",
                           Str("wave table size is smaller than source table size"));
  if (UNLIKELY(nargs < 3))
    csound->Warning(csound, "%s", Str("insufficient arguments"));
  fp_filter = srcfil->ftable;
  newLen  = srcfil->flen;
  mirr = (cs_float*) csound->Malloc(csound, sizeof(cs_float)*srcfil->flen);
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
  pInp = (cs_float*) csound->Calloc(csound, ftp->flen* sizeof(cs_float));
  pBuf = (cs_float*) csound->Calloc(csound, ftp->flen* sizeof(cs_float));
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
