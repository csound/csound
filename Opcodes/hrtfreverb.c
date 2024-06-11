/*
  Copyright #(c) 2010 Brian Carty
  PhD Code August 2010
  binaural reverb: diffuse field

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

#define SQUARE(X) ((X)*(X))

/* endian issues: swap bytes for ppc */
#ifdef WORDS_BIGENDIAN
static int32_t swap4bytes(CSOUND* csound, MEMFIL* mfp)
{
  char c1, c2, c3, c4;
  char *p = mfp->beginp;
  int32_t  size = mfp->length;

  while (size >= 4)
    {
      c1 = p[0]; c2 = p[1]; c3 = p[2]; c4 = p[3];
      p[0] = c4; p[1] = c3; p[2] = c2; p[3] = c1;
      size -= 4; p +=4;
    }

  return OK;
}
#else
static int32_t (*swap4bytes)(CSOUND*, MEMFIL*) = NULL;
#endif

/* matrices for feedback delay network (fdn) */
#define mthird (-FL(1.0) / 3)
#define tthird (FL(2.0) / 3)
#define msix (-FL(1.0) / 6)
#define fsix (FL(5.0) / 6)
#define mtw (-FL(1.0) / 12)
#define etw (FL(11.0) / 12)

static const MYFLT matrix6[36] =
  {tthird,mthird,mthird,mthird,mthird,mthird,
   mthird,tthird,mthird,mthird,mthird,mthird,
   mthird,mthird,tthird,mthird,mthird,mthird,
   mthird,mthird,mthird,tthird,mthird,mthird,
   mthird,mthird,mthird,mthird,tthird,mthird,
   mthird,mthird,mthird,mthird,mthird,tthird};

static const MYFLT matrix12[144] =
  {fsix,msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,
   msix,fsix,msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,
   msix,msix,fsix,msix,msix,msix,msix,msix,msix,msix,msix,msix,
   msix,msix,msix,fsix,msix,msix,msix,msix,msix,msix,msix,msix,
   msix,msix,msix,msix,fsix,msix,msix,msix,msix,msix,msix,msix,
   msix,msix,msix,msix,msix,fsix,msix,msix,msix,msix,msix,msix,
   msix,msix,msix,msix,msix,msix,fsix,msix,msix,msix,msix,msix,
   msix,msix,msix,msix,msix,msix,msix,fsix,msix,msix,msix,msix,
   msix,msix,msix,msix,msix,msix,msix,msix,fsix,msix,msix,msix,
   msix,msix,msix,msix,msix,msix,msix,msix,msix,fsix,msix,msix,
   msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,fsix,msix,
   msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,msix,fsix};

static const MYFLT matrix24[576] =
  {etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,
   mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,mtw,etw};

/* for delay line lengths */
static const int32_t primes[229] =
  {
    17,      23,     59,     71,    113,    127,    163,    191,    211,    229,
    271,    283,    313,    337,    359,    373,    409,    461,    541,    587,
    631,    691,    709,    773,    829,    863,    919,    971,    1039,   1069,
    1123,   1171,   1217,   1259,   1303,   1373,   1423,   1483,   1511,   1597,
    1627,   1669,   1733,   1787,   1847,   1867,   1913,   1951,   2027,   2081,
    2131,   2179,   2213,   2269,   2333,   2383,   2423,   2467,   2531,   2579,
    2617,   2671,   2729,   2789,   2837,   2861,   2917,   2999,   3011,   3083,
    3121,   3169,   3209,   3259,   3331,   3389,   3449,   3469,   3533,   3571,
    3613,   3671,   3727,   3779,   3821,   3889,   3917,   3989,   4001,   4051,
    4111,   4177,   4231,   4271,   4337,   4391,   4447,   4483,   4517,   4567,
    4621,   4691,   4733,   4787,   4817,   4861,   4919,   4967,   5023,   5077,
    5113,   5167,   5233,   5297,   5309,   5351,   5441,   5483,   5507,   5563,
    5641,   5683,   5711,   5783,   5821,   5857,   5927,   5981,   6011,   6067,
    6121,   6173,   6217,   6271,   6317,   6361,   6421,   6473,   6529,   6581,
    6607,   6661,   6733,   6793,   6841,   6883,   6911,   6961,   7027,   7057,
    7109,   7177,   7211,   7297,   7349,   7393,   7417,   7481,   7523,   7561,
    7607,   7673,   7717,   7789,   7841,   7879,   7919,   7963,   8017,   8081,
    8111,   8167,   8209,   8287,   8317,   8377,   8443,   8467,   8521,   8563,
    8623,   8677,   8713,   8761,   8831,   8867,   8923,   8963,   9013,   9059,
    9109,   9187,   9221,   9257,   9323,   9371,   9413,   9461,   9511,   9587,
    9631,   9679,   9721,   9781,   9803,   9859,   9949,   9973,   10039,  10079,
    10111,  10177,  10211,  10259,  10333,  10391,  10429,  10459,  10513,  10589,
    10607,  10663,  10711,  10799,  10831,  10859,  10909,  10979,  11003
  };

typedef struct
{
  OPDS h;
  /* in / out */
  /* outputs l/r and delay required for late del...*/
  MYFLT *outsigl, *outsigr, *idel;
  /* mean free path and order are optional, meanfp defaults to medium
     room, opcode can be used as stand alone binaural reverb, or
     spatially accurate taking meanfp and order from earlies opcode */
  MYFLT *insig, *ilowrt60, *ihighrt60;
  STRINGDAT *ifilel, *ifiler;
  MYFLT *osr, *omeanfp, *porder;

  /* internal data / class variables */
  MYFLT delaytime;
  int32_t delaytimeint, basedelay;

  /* number of delay lines */
  int32_t M;

  /* delay line iterators */
  int32_t u, v, w, x, y, z;
  int32_t ut, vt, wt, xt, yt, zt;
  int32_t utf1, vtf1, wtf1, xtf1, ytf1, ztf1;
  int32_t utf2, vtf2, wtf2, xtf2, ytf2, ztf2;

  /* buffer lengths, change for different sr */
  int32_t irlength;
  int32_t irlengthpad;
  int32_t overlapsize;

  /* memory buffers: delays */
  AUXCH delays;
  /* filter coeffs */
  AUXCH gi, ai;
  /* matrix manipulations */
  AUXCH inmat, inmatlp, dellp, outmat;
  /* delays */
  AUXCH del1, del2, del3, del4, del5, del6;
  AUXCH del1t, del2t, del3t, del4t, del5t, del6t;
  AUXCH del1tf, del2tf, del3tf, del4tf, del5tf, del6tf,
    del7tf, del8tf, del9tf, del10tf, del11tf, del12tf;
  /* filter variables, spectral manipulations */
  AUXCH power, HRTFave, num, denom, cohermags, coheru, coherv;
  AUXCH filtout, filtuout, filtvout, filtpad, filtupad, filtvpad;

  /* output of matrix cycle, with IIRs in combs and FIR tone, then l and
     r o/p processed with u and v coherence filters */
  /* with overlap buffers for overlap add convolution */
  AUXCH matrixlu, matrixrv;
  AUXCH olmatrixlu, olmatrixrv;
  /* above processed with hrtf l and r filters */
  /* with overlap buffers for overlap add convolution */
  AUXCH hrtfl, hrtfr;
  AUXCH olhrtfl, olhrtfr;
  /* filter coeff */
  MYFLT b;
  /* 1st order FIR mem */
  MYFLT inoldl, inoldr;
  /* for storing hrtf data used to create filters */
  AUXCH buffl, buffr;

  /* counter */
  int32_t counter;
  void *setup, *setup_pad, *isetup, *isetup_pad;
  MYFLT sr;

}hrtfreverb;

int32_t hrtfreverb_init(CSOUND *csound, hrtfreverb *p)
{
  /* left and right data files: spectral mag, phase format */
  MEMFIL *fpl = NULL, *fpr = NULL;
  char filel[MAXNAME],filer[MAXNAME];
  /* files contain floats */
  float *fpindexl, *fpindexr;

  /* processing sizes */
  int32_t irlength=0, irlengthpad=0, overlapsize=0;

  /* pointers used to fill buffers in data structure */
  int32_t *delaysp;
  MYFLT *gip, *aip;
  MYFLT *powerp, *HRTFavep, *nump, *denomp, *cohermagsp, *coherup, *cohervp;
  MYFLT *filtoutp, *filtuoutp, *filtvoutp, *filtpadp, *filtupadp, *filtvpadp;
  MYFLT *bufflp, *buffrp;

  /* iterators, file skip */
  int32_t i, j;
  int32_t skip = 0;
  int32_t skipdouble = 0;

  /* used in choice of delay line lengths */
  int32_t basedelay=0;

  /* local filter variables for spectral manipulations */
  MYFLT rel, rer, retemp, iml, imr, imtemp;

  /* setup filters */
  MYFLT T, alpha, aconst, exp;
  int32_t clipcheck = 0;

  MYFLT sr = (MYFLT)*p->osr;
  MYFLT meanfp = (MYFLT)*p->omeanfp;
  int32_t order = (int32_t)*p->porder;

  /* delay line variables */
  MYFLT delaytime, meanfporder;
  int32_t delaytimeint;
  int32_t Msix, Mtwelve, Mtwentyfour;
  int32_t meanfpsamps, meanfpordersamps;
  int32_t test;

  MYFLT rt60low = (MYFLT)*p->ilowrt60;
  MYFLT rt60high = (MYFLT)*p->ihighrt60;

  int32_t M;

  /* sr, defualt 44100 */
  if(sr != 44100 && sr != 48000 && sr != 96000)
    sr = 44100;
  p->sr = sr;

  if (UNLIKELY(CS_ESR != sr))
    csound->Message(csound,
                    Str("\n\nWARNING!!:\nOrchestra SR not compatible with"
                        " HRTF processing SR of: %.0f\n\n"), sr);

  /* meanfp: defaults to doom size 10 * 10 * 4 (max of 1: v. large room,
     min according to min room dimensions in early: 2 * 2 * 2) */
  if(meanfp <= 0.003876 || meanfp > 1)
    meanfp = FL(0.01292);

  /* order: defaults to 1 (4 is max for earlies) */
  if(order < 0 || order > 4)
    order = 1;

  /* rt60 values must be positive and non zero */
  if(rt60low <= 0)
    rt60low = FL(0.01);

  if(rt60high <= 0)
    rt60high = FL(0.01);

  /* setup as per sr */
  if(sr == 44100 || sr == 48000)
    {
      irlength = 128;
      irlengthpad = 256;
      overlapsize = (irlength - 1);
    }
  else if(sr == 96000)
    {
      irlength = 256;
      irlengthpad = 512;
      overlapsize = (irlength - 1);
    }

  /* copy in string name... */
  strncpy(filel, (char*) p->ifilel->data, MAXNAME-1);
  strncpy(filer, (char*) p->ifiler->data, MAXNAME-1);

  /* reading files, with byte swap */
  fpl = csound->LoadMemoryFile(csound, filel,
                               CSFTYPE_FLOATS_BINARY, swap4bytes);
  if (UNLIKELY(fpl == NULL))
    return
      csound->InitError(csound, "%s",
                        Str("\n\n\nCannot load left data file, exiting\n\n"));

  fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,swap4bytes);
  if (UNLIKELY(fpr == NULL))
    return
      csound->InitError(csound, "%s",
                        Str("\n\n\nCannot load right data file, exiting\n\n"));

  /* do not need to be in p, as only used in init */
  fpindexl = (float *)fpl->beginp;
  fpindexr = (float *)fpr->beginp;

  /* setup structure values */
  p->irlength = irlength;
  p->irlengthpad = irlengthpad;
  p->overlapsize = overlapsize;

  /* allocate memory */
  if (!p->power.auxp || p->power.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->power);
  if (!p->HRTFave.auxp || p->HRTFave.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->HRTFave);
  if (!p->num.auxp || p->num.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->num);
  if (!p->denom.auxp || p->denom.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->denom);
  if (!p->cohermags.auxp || p->cohermags.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->cohermags);
  if (!p->coheru.auxp || p->coheru.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->coheru);
  if (!p->coherv.auxp || p->coherv.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->coherv);

  if (!p->filtout.auxp || p->filtout.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->filtout);
  if (!p->filtuout.auxp || p->filtuout.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->filtuout);
  if (!p->filtvout.auxp || p->filtvout.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->filtvout);
  if (!p->filtpad.auxp || p->filtpad.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->filtpad);
  if (!p->filtupad.auxp || p->filtupad.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->filtupad);
  if (!p->filtvpad.auxp || p->filtvpad.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->filtvpad);

  /* zero numerator and power buffer, as they accumulate */
  memset(p->power.auxp, 0, irlength * sizeof(MYFLT));
  memset(p->num.auxp, 0, irlength * sizeof(MYFLT));
  /* no need to zero other above mem, as it will be filled in init */

  if (!p->matrixlu.auxp || p->matrixlu.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->matrixlu);
  if (!p->matrixrv.auxp || p->matrixrv.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->matrixrv);
  if (!p->olmatrixlu.auxp || p->olmatrixlu.size < overlapsize * sizeof(MYFLT))
    csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->olmatrixlu);
  if (!p->olmatrixrv.auxp || p->olmatrixrv.size < overlapsize * sizeof(MYFLT))
    csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->olmatrixrv);
  if (!p->hrtfl.auxp || p->hrtfl.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtfl);
  if (!p->hrtfr.auxp || p->hrtfr.size < irlengthpad * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtfr);
  if (!p->olhrtfl.auxp || p->olhrtfl.size < overlapsize * sizeof(MYFLT))
    csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->olhrtfl);
  if (!p->olhrtfr.auxp || p->olhrtfr.size < overlapsize * sizeof(MYFLT))
    csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->olhrtfr);

  memset(p->matrixlu.auxp, 0, irlengthpad * sizeof(MYFLT));
  memset(p->matrixrv.auxp, 0, irlengthpad * sizeof(MYFLT));
  memset(p->olmatrixlu.auxp, 0, overlapsize * sizeof(MYFLT));
  memset(p->olmatrixrv.auxp, 0, overlapsize * sizeof(MYFLT));
  memset(p->hrtfl.auxp, 0, irlengthpad * sizeof(MYFLT));
  memset(p->hrtfr.auxp, 0, irlengthpad * sizeof(MYFLT));
  memset(p->olhrtfl.auxp, 0, overlapsize * sizeof(MYFLT));
  memset(p->olhrtfr.auxp, 0, overlapsize * sizeof(MYFLT));

  if (!p->buffl.auxp || p->buffl.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->buffl);
  if (!p->buffr.auxp || p->buffr.size < irlength * sizeof(MYFLT))
    csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->buffr);

  memset(p->buffl.auxp, 0, irlength * sizeof(MYFLT));
  memset(p->buffr.auxp, 0, irlength * sizeof(MYFLT));

  /* buffers to store hrtf data */
  bufflp = (MYFLT *)p->buffl.auxp;
  buffrp = (MYFLT *)p->buffr.auxp;

  /* 0 delay iterators */
  p->u = p->v = p->w = p->x = p->y = p->z = 0;
  p->ut = p->vt = p->wt = p->xt = p->yt = p->zt = 0;
  p->utf1 = p->vtf1 = p->wtf1 = p->xtf1 = p->ytf1 = p->ztf1 = 0;
  p->utf2 = p->vtf2 = p->wtf2 = p->xtf2 = p->ytf2 = p->ztf2 = 0;

  /* calculate delayline lengths */
  meanfporder = meanfp * (order + 1);
  meanfpsamps = (int32_t)(meanfp * sr);
  meanfpordersamps = (int32_t)(meanfporder * sr);

  /* setup reverb time */
  delaytime = rt60low > rt60high ? rt60low : rt60high;

  /* in samples */
  delaytime *= sr;
  /* schroeder suggests 0.15 modes per Hz, so M should be > 0.15 t60 */
  delaytime /= 7;

  /* which no. of delay lines implies ave delay nearest to mfp(which is an
     appropriate ave)? */
  Msix = abs((int32_t)(delaytime / 6) - meanfpsamps);
  Mtwelve = abs((int32_t)(delaytime / 12) - meanfpsamps);
  Mtwentyfour = abs((int32_t)(delaytime / 24) - meanfpsamps);
  M = Mtwelve < Mtwentyfour ? (Msix < Mtwelve ? 6 : 12) : 24;

  csound->Message(csound, "%d\n", M);

  delaytime /= M;
  delaytimeint= (int32_t)delaytime;

  if(delaytimeint < meanfpsamps)
    delaytimeint = meanfpsamps;

  /*csound->Message(csound, "%d %d %d\n", M, delaytimeint, meanfpsamps);*/

  /* maximum value, according to primes array and delay line allocation */
  if(delaytimeint > 10112)
    delaytimeint = 10112;

  /* minimum values, according to primes array and delay line allocation */
  if(M==6)
    {
      if(delaytimeint < 164)
        delaytimeint = 164;
    }
  else if(M==12)
    {
      if(delaytimeint < 374)
        delaytimeint = 374;
    }
  else if(M==24)
    {
      if(delaytimeint < 410)
        delaytimeint = 410;
    }

  /* allocate memory based on M: number of delays */
  if (!p->delays.auxp || p->delays.size < M * sizeof(int32_t))
    csound->AuxAlloc(csound, M * sizeof(int32_t), &p->delays);
  if (!p->gi.auxp || p->gi.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->gi);
  if (!p->ai.auxp || p->ai.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->ai);
  if (!p->inmat.auxp || p->inmat.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->inmat);
  if (!p->inmatlp.auxp || p->inmatlp.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->inmatlp);
  if (!p->dellp.auxp || p->dellp.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->dellp);
  if (!p->outmat.auxp || p->outmat.size < M * sizeof(MYFLT))
    csound->AuxAlloc(csound, M * sizeof(MYFLT), &p->outmat);

  memset(p->delays.auxp, 0, M * sizeof(int32_t));
  memset(p->gi.auxp, 0, M * sizeof(MYFLT));
  memset(p->ai.auxp, 0, M * sizeof(MYFLT));
  memset(p->inmat.auxp, 0, M * sizeof(MYFLT));
  memset(p->inmatlp.auxp, 0, M * sizeof(MYFLT));
  memset(p->dellp.auxp, 0, M * sizeof(MYFLT));
  memset(p->outmat.auxp, 0, M * sizeof(MYFLT));

  /* choose appropriate base delay times */
  for(i = 0; i < 212; i++)
    {
      if(M == 6)
        test = (i > 6 ? i : 6) - 6;
      else if(M == 12)
        test = (i > 15 ? i : 15) - 15;
      else
        test = (i > 16 ? i : 16) - 16;

      if(primes[i] > delaytimeint || primes[test] > meanfpordersamps)
        {
          basedelay = i - 1;
          if(primes[test] > meanfpordersamps)
            csound->Message(csound, "%s", Str("\nfdn delay > earlies del..., fixed!"));
          *p->idel = (meanfpordersamps - primes[test - 1]) / sr;
          break;
        }
    }

  delaysp = (int32_t *)p->delays.auxp;

  /* fill delay data, note this data can be filled locally */
  delaysp[0] = primes[basedelay];
  delaysp[1] = primes[basedelay + 3];
  delaysp[2] = primes[basedelay - 3];
  delaysp[3] = primes[basedelay + 6];
  delaysp[4] = primes[basedelay - 6];
  delaysp[5] = primes[basedelay + 9];
  if(M ==12 || M==24)
    {
      delaysp[6] = primes[basedelay - 9];
      delaysp[7] = primes[basedelay + 12];
      delaysp[8] = primes[basedelay - 12];
      delaysp[9] = primes[basedelay + 15];
      delaysp[10] = primes[basedelay - 15];
      delaysp[11] = primes[basedelay + 18];
    }
  if(M ==24)
    {
      /* fill in gaps... */
      delaysp[12] = primes[basedelay + 1];
      delaysp[13] = primes[basedelay - 1];
      delaysp[14] = primes[basedelay + 4];
      delaysp[15] = primes[basedelay - 4];
      delaysp[16] = primes[basedelay + 7];
      delaysp[17] = primes[basedelay - 7];
      delaysp[18] = primes[basedelay + 10];
      delaysp[19] = primes[basedelay - 10];
      delaysp[20] = primes[basedelay + 13];
      delaysp[21] = primes[basedelay - 13];
      delaysp[22] = primes[basedelay + 16];
      delaysp[23] = primes[basedelay - 16];
    }

  /* setup and zero delay lines */
  if (!p->del1.auxp || p->del1.size < delaysp[0] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[0] * sizeof(MYFLT), &p->del1);
  if (!p->del2.auxp || p->del2.size < delaysp[1] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[1] * sizeof(MYFLT), &p->del2);
  if (!p->del3.auxp || p->del3.size < delaysp[2] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[2] * sizeof(MYFLT), &p->del3);
  if (!p->del4.auxp || p->del4.size < delaysp[3] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[3] * sizeof(MYFLT), &p->del4);
  if (!p->del5.auxp || p->del5.size < delaysp[4] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[4] * sizeof(MYFLT), &p->del5);
  if (!p->del6.auxp || p->del6.size < delaysp[5] * sizeof(MYFLT))
    csound->AuxAlloc(csound, delaysp[5] * sizeof(MYFLT), &p->del6);

  memset(p->del1.auxp, 0, delaysp[0] * sizeof(MYFLT));
  memset(p->del2.auxp, 0, delaysp[1] * sizeof(MYFLT));
  memset(p->del3.auxp, 0, delaysp[2] * sizeof(MYFLT));
  memset(p->del4.auxp, 0, delaysp[3] * sizeof(MYFLT));
  memset(p->del5.auxp, 0, delaysp[4] * sizeof(MYFLT));
  memset(p->del6.auxp, 0, delaysp[5] * sizeof(MYFLT));

  /* if 12 delay lines required */
  if(M == 12 || M==24)
    {
      if (!p->del1t.auxp || p->del1t.size < delaysp[6] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[6] * sizeof(MYFLT), &p->del1t);
      if (!p->del2t.auxp || p->del2t.size < delaysp[7] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[7] * sizeof(MYFLT), &p->del2t);
      if (!p->del3t.auxp || p->del3t.size < delaysp[8] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[8] * sizeof(MYFLT), &p->del3t);
      if (!p->del4t.auxp || p->del4t.size < delaysp[9] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[9] * sizeof(MYFLT), &p->del4t);
      if (!p->del5t.auxp || p->del5t.size < delaysp[10] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[10] * sizeof(MYFLT), &p->del5t);
      if (!p->del6t.auxp || p->del6t.size < delaysp[11] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[11] * sizeof(MYFLT), &p->del6t);

      memset(p->del1t.auxp, 0, delaysp[6] * sizeof(MYFLT));
      memset(p->del2t.auxp, 0, delaysp[7] * sizeof(MYFLT));
      memset(p->del3t.auxp, 0, delaysp[8] * sizeof(MYFLT));
      memset(p->del4t.auxp, 0, delaysp[9] * sizeof(MYFLT));
      memset(p->del5t.auxp, 0, delaysp[10] * sizeof(MYFLT));
      memset(p->del6t.auxp, 0, delaysp[11] * sizeof(MYFLT));
    }
  if(M==24)
    {
      if (!p->del1tf.auxp || p->del1tf.size < delaysp[12] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[12] * sizeof(MYFLT), &p->del1tf);
      if (!p->del2tf.auxp || p->del2tf.size < delaysp[13] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[13] * sizeof(MYFLT), &p->del2tf);
      if (!p->del3tf.auxp || p->del3tf.size < delaysp[14] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[14] * sizeof(MYFLT), &p->del3tf);
      if (!p->del4tf.auxp || p->del4tf.size < delaysp[15] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[15] * sizeof(MYFLT), &p->del4tf);
      if (!p->del5tf.auxp || p->del5tf.size < delaysp[16] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[16] * sizeof(MYFLT), &p->del5tf);
      if (!p->del6tf.auxp || p->del6tf.size < delaysp[17] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[17] * sizeof(MYFLT), &p->del6tf);
      if (!p->del7tf.auxp || p->del7tf.size < delaysp[18] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[18] * sizeof(MYFLT), &p->del7tf);
      if (!p->del8tf.auxp || p->del8tf.size < delaysp[19] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[19] * sizeof(MYFLT), &p->del8tf);
      if (!p->del9tf.auxp || p->del9tf.size < delaysp[20] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[20] * sizeof(MYFLT), &p->del9tf);
      if (!p->del10tf.auxp || p->del10tf.size < delaysp[21] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[21] * sizeof(MYFLT), &p->del10tf);
      if (!p->del11tf.auxp || p->del11tf.size < delaysp[22] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[22] * sizeof(MYFLT), &p->del11tf);
      if (!p->del12tf.auxp || p->del12tf.size < delaysp[23] * sizeof(MYFLT))
        csound->AuxAlloc(csound, delaysp[23] * sizeof(MYFLT), &p->del12tf);

      memset(p->del1tf.auxp, 0, delaysp[12] * sizeof(MYFLT));
      memset(p->del2tf.auxp, 0, delaysp[13] * sizeof(MYFLT));
      memset(p->del3tf.auxp, 0, delaysp[14] * sizeof(MYFLT));
      memset(p->del4tf.auxp, 0, delaysp[15] * sizeof(MYFLT));
      memset(p->del5tf.auxp, 0, delaysp[16] * sizeof(MYFLT));
      memset(p->del6tf.auxp, 0, delaysp[17] * sizeof(MYFLT));
      memset(p->del7tf.auxp, 0, delaysp[18] * sizeof(MYFLT));
      memset(p->del8tf.auxp, 0, delaysp[19] * sizeof(MYFLT));
      memset(p->del9tf.auxp, 0, delaysp[20] * sizeof(MYFLT));
      memset(p->del10tf.auxp, 0, delaysp[21] * sizeof(MYFLT));
      memset(p->del11tf.auxp, 0, delaysp[22] * sizeof(MYFLT));
      memset(p->del12tf.auxp, 0, delaysp[23] * sizeof(MYFLT));
    }

  powerp = (MYFLT *)p->power.auxp;
  HRTFavep = (MYFLT *)p->HRTFave.auxp;
  nump = (MYFLT *)p->num.auxp;
  denomp = (MYFLT *)p->denom.auxp;
  cohermagsp = (MYFLT *)p->cohermags.auxp;
  coherup = (MYFLT *)p->coheru.auxp;
  cohervp = (MYFLT *)p->coherv.auxp;

  /* usually, just go through all files; in this case, just doubled due
     to symmetry (with exceptions, as below) */
  for(i = 0; i < 368; i ++)
    {
      /* if at a measurement where no doubling for symmetry necessary... */
      if(i == 0 || i == 28 || i == 29 || i == 59 || i == 60 || i == 96  ||
         i == 97 || i == 133 || i == 134 || i == 170 || i == 171 ||
         i == 207 || i == 208 || i == 244  || i == 245 || i == 275 ||
         i == 276 || i == 304  || i == 305 || i == 328 || i == 346 ||
         i == 347 || i == 359 || i == 360 || i == 366 || i == 367)
        skipdouble = 1;
      else
        skipdouble = 0;

      for(j = 0; j < irlength; j ++)
        {
          bufflp[j] = fpindexl[skip + j];
          buffrp[j] = fpindexr[skip + j];
        }

      /* deal with 0 hz and nyq: may be a negative real val, no need for
         fabs() as squaring anyway! */
      /* skipdouble: l = r */
      if(skipdouble)
        {
          powerp[0] = powerp[0] + SQUARE(bufflp[0]);
          powerp[1] = powerp[1] + SQUARE(bufflp[1]);
        }
      /* include both */
      else
        {
          powerp[0] = powerp[0] + SQUARE(bufflp[0]) + SQUARE(buffrp[0]);
          powerp[1] = powerp[1] + SQUARE(bufflp[1]) + SQUARE(buffrp[1]);
        }

      for(j = 2; j < irlength; j += 2)
        {
          if(skipdouble)
            powerp[j] = powerp[j] + (MYFLT)SQUARE(bufflp[j]);
          else
            powerp[j] = powerp[j] + (MYFLT)SQUARE(bufflp[j]) +
              (MYFLT)SQUARE(buffrp[j]);
          powerp[j + 1] = FL(0.0);
        }
      skip += irlength;
    }

  for(i = 0; i < irlength; i++)
    HRTFavep[i] = SQRT(powerp[i] / FL(710.0));

  fpindexl = (float *)fpl->beginp;
  fpindexr = (float *)fpr->beginp;
  skip = 0;

  /* coherence values */
  for(i = 0; i< 368; i++)
    {
      /* if at a measurement where no doubling for symmetry necessary... */
      if(i == 0 || i == 28 || i == 29 || i == 59 || i == 60 || i == 96  ||
         i == 97 || i == 133 || i == 134 || i == 170 || i == 171 ||
         i == 207 || i == 208 || i == 244  || i == 245 || i == 275 ||
         i == 276 || i == 304  || i == 305 || i == 328 || i == 346 ||
         i == 347 || i == 359 || i == 360 || i == 366 || i == 367)
        skipdouble = 1;
      else
        skipdouble = 0;

      for(j = 0; j < irlength; j ++)
        {
          bufflp[j] = fpindexl[skip + j];
          buffrp[j] = fpindexr[skip + j];
        }

      /* back to rectangular to find numerator: need complex nos */
      /* 0Hz and Nyq ok as real */
      if(skipdouble)
        {
          nump[0] = nump[0] + (bufflp[0] * buffrp[0]);
          nump[1] = nump[1] + (bufflp[1] * buffrp[1]);
        }
      else
        {
          nump[0] = nump[0] + (bufflp[0] * buffrp[0]) + (buffrp[0] * bufflp[0]);
          nump[1] = nump[1] + (bufflp[1] * buffrp[1]) + (buffrp[1] * bufflp[1]);
        }

      /* complex multiplication */
      /* (a + i b)(c + i d) */
      /* = (a c - b d) + i(a d + b c) */
      /* conjugate: d becomes -d ->
         = (a c + b d) + i(- a d + b c) */
      /* doing l * conj r and r * conj l here, as dataset symmetrical...
         for non symmetrical, just go through all and do l * conj r */
      for(j = 2; j < irlength; j += 2)
        {
          rel = bufflp[j] * COS(bufflp[j + 1]);
          iml = bufflp[j] * SIN(bufflp[j + 1]);
          rer = buffrp[j] * COS(buffrp[j + 1]);
          imr = buffrp[j] * SIN(buffrp[j + 1]);
          if(skipdouble)
            {
              nump[j] = nump[j] + ((rel * rer) + (iml * imr));
              nump[j + 1] = nump[j + 1] + ((rel * -imr) + (iml * rer));
            }
          else
            {
              nump[j] = nump[j] + ((rel * rer) + (iml * imr)) +
                ((rer * rel) + (imr * iml));
              nump[j + 1] = nump[j + 1] + ((rel * -imr) + (iml * rer)) +
                ((rer * -iml) + (imr * rel));
            }
        }
      skip += irlength;
    }

  /* 0 & nyq = fabs() for mag... */
  nump[0] = FABS(nump[0]);
  nump[1] = FABS(nump[1]);

  /* magnitudes of sum of conjugates */
  for(i = 2; i < irlength; i += 2)
    {
      retemp = nump[i];
      imtemp = nump[i + 1];
      nump[i] = SQRT(SQUARE(retemp) + SQUARE(imtemp));
      nump[i + 1] = FL(0.0);
    }

  /* sqrt (powl * powr) powl = powr in symmetric case, so just power[] needed */
  for(i = 0; i < irlength; i++)
    denomp[i] = powerp[i];

  /* coherence values */
  cohermagsp[0] = nump[0] / denomp[0];
  cohermagsp[1] = nump[1] / denomp[1];

  for(i = 2; i < irlength; i += 2)
    {
      cohermagsp[i] = nump[i] / denomp[i];
      cohermagsp[i+1] = FL(0.0);
    }

  /* coherence formula */
  coherup[0] = SQRT((FL(1.0) + cohermagsp[0]) / FL(2.0));
  coherup[1] = SQRT((FL(1.0) + cohermagsp[1]) / FL(2.0));
  cohervp[0] = SQRT((FL(1.0) - cohermagsp[0]) / FL(2.0));
  cohervp[1] = SQRT((FL(1.0) - cohermagsp[1]) / FL(2.0));

  for(i = 2; i < irlength; i += 2)
    {
      coherup[i] = SQRT((FL(1.0) + cohermagsp[i]) / FL(2.0));
      cohervp[i] = SQRT((FL(1.0) - cohermagsp[i]) / FL(2.0));
      coherup[i + 1] = FL(0.0);
      cohervp[i + 1] = FL(0.0);
    }

  /* no need to go back to rectangular for fft, as phase = 0, so same */
  csound->RealFFT(csound, p->isetup, HRTFavep);
  csound->RealFFT(csound, p->isetup, coherup);
  csound->RealFFT(csound, p->isetup, cohervp);

  filtoutp = (MYFLT *)p->filtout.auxp;
  filtuoutp = (MYFLT *)p->filtuout.auxp;
  filtvoutp = (MYFLT *)p->filtvout.auxp;
  filtpadp = (MYFLT *)p->filtpad.auxp;
  filtupadp = (MYFLT *)p->filtupad.auxp;
  filtvpadp = (MYFLT *)p->filtvpad.auxp;

  /* shift */
  for(i = 0; i < irlength; i++)
    {
      filtoutp[i] = HRTFavep[(i + (irlength / 2)) % irlength];
      filtuoutp[i] = coherup[(i + (irlength / 2)) % irlength];
      filtvoutp[i] = cohervp[(i + (irlength / 2)) % irlength];
    }

  for(i = 0; i < irlength; i++)
    {
      filtpadp[i] = filtoutp[i];
      filtupadp[i] = filtuoutp[i];
      filtvpadp[i] = filtvoutp[i];
    }
  for(i = irlength; i < irlengthpad; i++)
    {
      filtpadp[i] = FL(0.0);
      filtupadp[i] = FL(0.0);
      filtvpadp[i] = FL(0.0);
    }

  csound->RealFFT(csound, p->setup_pad,filtpadp);
  csound->RealFFT(csound, p->setup_pad,filtupadp);
  csound->RealFFT(csound, p->setup_pad,filtvpadp);

  T = FL(1.0) / sr;

  gip = (MYFLT *)p->gi.auxp;
  aip = (MYFLT *)p->ai.auxp;

  do
    {
      double alphsq;
      alpha = rt60high / rt60low;
      clipcheck = 0;
      alphsq = SQUARE(alpha);
      p->b = (FL(1.0) - alpha) / (FL(1.0) + alpha);
      aconst = (LOG(FL(10.0)) / FL(4.0)) * (FL(1.0) - (FL(1.0) / alphsq));
      for(i = 0; i < M; i++)
        {
          exp = (-FL(3.0) * delaysp[i] * T) / rt60low;
          gip[i] = POWER(FL(10.0), exp);
          aip[i] =  exp * aconst;

          if(aip[i] > FL(0.99) || aip[i] < -FL(0.99))
            {
              csound->Message(csound, "%s",
                              Str("\nwarning, approaching instability, "
                                  "fixed with a flat late reverb!"));
              clipcheck = 1;
              if(aip[i] > 0.99)
                rt60high = rt60low;
              else
                rt60low = rt60high;
              break;
            }

        }
    }while(clipcheck);

  /* initialise counter and filter delays */
  p->counter = 0;
  p->inoldl = 0;
  p->inoldr = 0;
  p->M = M;
  p->setup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_FWD);
  p->setup = csound->RealFFTSetup(csound, p->irlength, FFT_FWD);
  p->isetup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_INV);
  p->isetup = csound->RealFFTSetup(csound, p->irlength, FFT_INV);

  return OK;
}

int32_t hrtfreverb_process(CSOUND *csound, hrtfreverb *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, nsmps = CS_KSMPS;
  int32_t j, k;

  /* signals in, out */
  MYFLT *in = p->insig, sigin;
  MYFLT *outl = p->outsigl;
  MYFLT *outr = p->outsigr;

  /* pointers to delay data */
  MYFLT *del1p, *del2p, *del3p, *del4p, *del5p, *del6p;
  MYFLT *del1tp=NULL, *del2tp=NULL, *del3tp=NULL,
    *del4tp=NULL, *del5tp=NULL, *del6tp=NULL;
  MYFLT *del1tfp=NULL, *del2tfp=NULL, *del3tfp=NULL, *del4tfp=NULL,
    *del5tfp=NULL, *del6tfp=NULL, *del7tfp=NULL, *del8tfp=NULL,
    *del9tfp=NULL, *del10tfp=NULL, *del11tfp=NULL, *del12tfp=NULL;
  int32_t *delaysp;

  /* matrix manipulation */
  MYFLT *inmatp, *inmatlpp, *dellpp, *outmatp;

  /* delay line iterators */
  int32_t u, v, w, x, y, z;
  int32_t ut=0, vt=0, wt=0, xt=0, yt=0, zt=0;
  int32_t utf1=0, vtf1=0, wtf1=0, xtf1=0, ytf1=0, ztf1=0;
  int32_t utf2=0, vtf2=0, wtf2=0, xtf2=0, ytf2=0, ztf2=0;

  /* number of delays */
  int32_t M = p->M;

  /* FIR temp variables */
  MYFLT tonall, tonalr;
  MYFLT b = p->b;

  /* IIR variables */
  MYFLT *gip, *aip;

  /* counter */
  int32_t counter = p->counter;

  /* matrix/coher and hrtf filter buffers, with overlap add buffers */
  MYFLT *matrixlup = (MYFLT *)p->matrixlu.auxp;
  MYFLT *matrixrvp = (MYFLT *)p->matrixrv.auxp;
  MYFLT *olmatrixlup = (MYFLT *)p->olmatrixlu.auxp;
  MYFLT *olmatrixrvp = (MYFLT *)p->olmatrixrv.auxp;
  MYFLT *hrtflp = (MYFLT *)p->hrtfl.auxp;
  MYFLT *hrtfrp = (MYFLT *)p->hrtfr.auxp;
  MYFLT *olhrtflp = (MYFLT *)p->olhrtfl.auxp;
  MYFLT *olhrtfrp = (MYFLT *)p->olhrtfr.auxp;

  /* processing lengths */
  int32_t irlength = p->irlength;
  int32_t irlengthpad = p->irlengthpad;
  int32_t overlapsize = p->overlapsize;

  /* 1st order FIR mem */
  MYFLT inoldl = p->inoldl;
  MYFLT inoldr = p->inoldr;

  /* filters, created in INIT */
  MYFLT *filtpadp = (MYFLT *)p->filtpad.auxp;
  MYFLT *filtupadp = (MYFLT *)p->filtupad.auxp;
  MYFLT *filtvpadp = (MYFLT *)p->filtvpad.auxp;

  MYFLT sr = p->sr;

  del1p = (MYFLT *)p->del1.auxp;
  del2p = (MYFLT *)p->del2.auxp;
  del3p = (MYFLT *)p->del3.auxp;
  del4p = (MYFLT *)p->del4.auxp;
  del5p = (MYFLT *)p->del5.auxp;
  del6p = (MYFLT *)p->del6.auxp;

  if(M==12 || M==24)
    {
      del1tp = (MYFLT *)p->del1t.auxp;
      del2tp = (MYFLT *)p->del2t.auxp;
      del3tp = (MYFLT *)p->del3t.auxp;
      del4tp = (MYFLT *)p->del4t.auxp;
      del5tp = (MYFLT *)p->del5t.auxp;
      del6tp = (MYFLT *)p->del6t.auxp;
    }
  if(M==24)
    {
      del1tfp = (MYFLT *)p->del1tf.auxp;
      del2tfp = (MYFLT *)p->del2tf.auxp;
      del3tfp = (MYFLT *)p->del3tf.auxp;
      del4tfp = (MYFLT *)p->del4tf.auxp;
      del5tfp = (MYFLT *)p->del5tf.auxp;
      del6tfp = (MYFLT *)p->del6tf.auxp;
      del7tfp = (MYFLT *)p->del7tf.auxp;
      del8tfp = (MYFLT *)p->del8tf.auxp;
      del9tfp = (MYFLT *)p->del9tf.auxp;
      del10tfp = (MYFLT *)p->del10tf.auxp;
      del11tfp = (MYFLT *)p->del11tf.auxp;
      del12tfp = (MYFLT *)p->del12tf.auxp;
    }

  delaysp = (int32_t *)p->delays.auxp;

  inmatp = (MYFLT *)p->inmat.auxp;
  inmatlpp = (MYFLT *)p->inmatlp.auxp;
  dellpp = (MYFLT *)p->dellp.auxp;
  outmatp = (MYFLT *)p->outmat.auxp;

  gip = (MYFLT *)p->gi.auxp;
  aip = (MYFLT *)p->ai.auxp;

  /* point to structure */
  u = p->u;
  v = p->v;
  w = p->w;
  x = p->x;
  y = p->y;
  z = p->z;
  if(M==12 || M==24)
    {
      ut = p->ut;
      vt = p->vt;
      wt = p->wt;
      xt = p->xt;
      yt = p->yt;
      zt = p->zt;
    }
  /* else { */
  /*   printf("Should not get here\n"); */
  /*   ut = vt = wt = xt = yt = zt = 0; */
  /* } */
  if(M==24)
    {
      utf1 = p->utf1;
      vtf1 = p->vtf1;
      wtf1 = p->wtf1;
      xtf1 = p->xtf1;
      ytf1 = p->ytf1;
      ztf1 = p->ztf1;
      utf2 = p->utf2;
      vtf2 = p->vtf2;
      wtf2 = p->wtf2;
      xtf2 = p->xtf2;
      ytf2 = p->ytf2;
      ztf2 = p->ztf2;
    }

  if (UNLIKELY(offset)) {
    memset(outl, '\0', offset*sizeof(MYFLT));
    memset(outr, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&outl[nsmps], '\0', early*sizeof(MYFLT));
    memset(&outr[nsmps], '\0', early*sizeof(MYFLT));
  }
  /* processing loop */
  for(i=offset; i < nsmps; i++)
    {
      /* tonal filter: 1 - b pow(z,-1) / 1 - b
         1/1-b in - b/1-b in(old) */
      /* dot product of l and r = 0 for uncorrelated */
      tonall = (del1p[u] - del2p[v] + del3p[w] - del4p[x] + del5p[y] - del6p[z]);
      if(M==12 || M==24)
        tonall += (del1tp[ut] - del2tp[vt] + del3tp[wt] -
                   del4tp[xt] + del5tp[yt] - del6tp[zt]);
      if(M==24)
        tonall += (del1tfp[utf1] - del2tfp[vtf1] + del3tfp[wtf1] -
                   del4tfp[xtf1] + del5tfp[ytf1] - del6tfp[ztf1] +
                   del7tfp[utf2] - del8tfp[vtf2] + del9tfp[wtf2] -
                   del10tfp[xtf2] + del11tfp[ytf2] - del12tfp[ztf2]);
      matrixlup[counter] = (((FL(1.0) / (FL(1.0) - b)) * tonall) -
                            ((b / (FL(1.0) - b)) * inoldl));
      matrixlup[counter] /= M;
      inoldl = tonall;

      tonalr = (del1p[u] + del2p[v] + del3p[w] + del4p[x] + del5p[y] + del6p[z]);
      if(M==12 || M==24)
        tonalr += (del1tp[ut] + del2tp[vt] + del3tp[wt] +
                   del4tp[xt] + del5tp[yt] + del6tp[zt]);
      if(M==24)
        tonalr += (del1tfp[utf1] - del2tfp[vtf1] + del3tfp[wtf1] -
                   del4tfp[xtf1] + del5tfp[ytf1] - del6tfp[ztf1] +
                   del7tfp[utf2] - del8tfp[vtf2] + del9tfp[wtf2] -
                   del10tfp[xtf2] + del11tfp[ytf2] - del12tfp[ztf2]);
      matrixrvp[counter] = (((FL(1.0) / (FL(1.0) - b)) * tonalr) -
                            ((b / (FL(1.0) - b)) * inoldr));
      matrixrvp[counter] /= M;
      inoldr = tonalr;

      /* inputs from del lines (need more for larger fdn) */
      inmatp[0] = del1p[u];
      inmatp[1] = del2p[v];
      inmatp[2] = del3p[w];
      inmatp[3] = del4p[x];
      inmatp[4] = del5p[y];
      inmatp[5] = del6p[z];

      if(M==12 || M==24)
        {
          inmatp[6] = del1tp[ut];
          inmatp[7] = del2tp[vt];
          inmatp[8] = del3tp[wt];
          inmatp[9] = del4tp[xt];
          inmatp[10] = del5tp[yt];
          inmatp[11] = del6tp[zt];
        }
      if(M==24)
        {
          inmatp[12] = del1tfp[utf1];
          inmatp[13] = del2tfp[vtf1];
          inmatp[14] = del3tfp[wtf1];
          inmatp[15] = del4tfp[xtf1];
          inmatp[16] = del5tfp[ytf1];
          inmatp[17] = del6tfp[ztf1];
          inmatp[18] = del7tfp[utf2];
          inmatp[19] = del8tfp[vtf2];
          inmatp[20] = del9tfp[wtf2];
          inmatp[21] = del10tfp[xtf2];
          inmatp[22] = del11tfp[ytf2];
          inmatp[23] = del12tfp[ztf2];
        }

      /* low pass each
         filter:
         gi ( 1 - ai / 1 - ai pow(z,-1)
         op = gi - gi ai x(n) + ai del
         del = op */

      for(j = 0; j < M; j++)
        {
          inmatlpp[j] = (gip[j] * (1 - aip[j]) * inmatp[j]) +
            (aip[j] * dellpp[j]);
          dellpp[j] = inmatlpp[j];
        }

      /* matrix mult: multiplying a vector by a matrix:
         embedded householders cause stability issues,
         as reported by Murphy...*/
      for(j = 0; j < M; j++)
        {
          outmatp[j] = FL(0.0);
          for(k = 0; k < M; k++)
            {
              if(M==24)
                outmatp[j] += (matrix24[j * M + k] * inmatlpp[k]);
              else if(M==12)
                outmatp[j] += (matrix12[j * M + k] * inmatlpp[k]);
              else
                outmatp[j] += (matrix6[j * M + k] * inmatlpp[k]);
            }
        }

      sigin = in[i] * (FL(32767.0) / csound->Get0dBFS(csound));

      del1p[u] = outmatp[0] + sigin;
      del2p[v] = outmatp[1] + sigin;
      del3p[w] = outmatp[2] + sigin;
      del4p[x] = outmatp[3] + sigin;
      del5p[y] = outmatp[4] + sigin;
      del6p[z] = outmatp[5] + sigin;
      if(M == 12 || M == 24)
        {
          del1tp[ut] = outmatp[6] + sigin;
          del2tp[vt] = outmatp[7] + sigin;
          del3tp[wt] = outmatp[8] + sigin;
          del4tp[xt] = outmatp[9] + sigin;
          del5tp[yt] = outmatp[10] + sigin;
          del6tp[zt] = outmatp[11] + sigin;
        }
      if(M == 24)
        {
          del1tfp[utf1] = outmatp[12] + sigin;
          del2tfp[vtf1] = outmatp[13] + sigin;
          del3tfp[wtf1] = outmatp[14] + sigin;
          del4tfp[xtf1] = outmatp[15] + sigin;
          del5tfp[ytf1] = outmatp[16] + sigin;
          del6tfp[ztf1] = outmatp[17] + sigin;
          del7tfp[utf2] = outmatp[18] + sigin;
          del8tfp[vtf2] = outmatp[19] + sigin;
          del9tfp[wtf2] = outmatp[20] + sigin;
          del10tfp[xtf2] = outmatp[21] + sigin;
          del11tfp[ytf2] = outmatp[22] + sigin;
          del12tfp[ztf2] = outmatp[23] + sigin;
        }

      u = (u != delaysp[0] - 1 ? u + 1 : 0);
      v = (v != delaysp[1] - 1 ? v + 1 : 0);
      w = (w != delaysp[2] - 1 ? w + 1 : 0);
      x = (x != delaysp[3] - 1 ? x + 1 : 0);
      y = (y != delaysp[4] - 1 ? y + 1 : 0);
      z = (z != delaysp[5] - 1 ? z + 1 : 0);

      if(M == 12 || M == 24)
        {
          ut = (ut != delaysp[6] - 1 ? ut + 1 : 0);
          vt = (vt != delaysp[7] - 1 ? vt + 1 : 0);
          wt = (wt != delaysp[8] - 1 ? wt + 1 : 0);
          xt = (xt != delaysp[9] - 1 ? xt + 1 : 0);
          yt = (yt != delaysp[10] - 1 ? yt + 1 : 0);
          zt = (zt != delaysp[11] - 1 ? zt + 1 : 0);
        }
      if(M == 24)
        {
          utf1 = (utf1 != delaysp[12] - 1 ? utf1 + 1 : 0);
          vtf1 = (vtf1 != delaysp[13] - 1 ? vtf1 + 1 : 0);
          wtf1 = (wtf1 != delaysp[14] - 1 ? wtf1 + 1 : 0);
          xtf1 = (xtf1 != delaysp[15] - 1 ? xtf1 + 1 : 0);
          ytf1 = (ytf1 != delaysp[16] - 1 ? ytf1 + 1 : 0);
          ztf1 = (ztf1 != delaysp[17] - 1 ? ztf1 + 1 : 0);
          utf2 = (utf2 != delaysp[18] - 1 ? utf2 + 1 : 0);
          vtf2 = (vtf2 != delaysp[19] - 1 ? vtf2 + 1 : 0);
          wtf2 = (wtf2 != delaysp[20] - 1 ? wtf2 + 1 : 0);
          xtf2 = (xtf2 != delaysp[21] - 1 ? xtf2 + 1 : 0);
          ytf2 = (ytf2 != delaysp[22] - 1 ? ytf2 + 1 : 0);
          ztf2 = (ztf2 != delaysp[23] - 1 ? ztf2 + 1 : 0);
        }

      /* output, increment counter */
      //                      outl[i] = hrtflp[counter];
      //                      outr[i] = hrtfrp[counter];

      outl[i] = hrtflp[counter] * (csound->Get0dBFS(csound)/ FL(32767.0));
      outr[i] = hrtfrp[counter] * (csound->Get0dBFS(csound)/ FL(32767.0));
      counter++;

      if(counter == irlength)
        {
          for(j = irlength; j < irlengthpad; j++)
            {
              matrixlup[j] = FL(0.0);
              matrixrvp[j] = FL(0.0);
            }

          /* fft result from matrices */
          csound->RealFFT(csound, p->setup_pad,matrixlup);
          csound->RealFFT(csound, p->setup_pad,matrixrvp);

          /* convolution: spectral multiplication */
          csound->RealFFTMult(csound, matrixlup, matrixlup,
                              filtupadp, irlengthpad, FL(1.0));
          csound->RealFFTMult(csound, matrixrvp, matrixrvp,
                              filtvpadp, irlengthpad, FL(1.0));

          /* ifft result */
          csound->RealFFT(csound, p->isetup_pad, matrixlup);
          csound->RealFFT(csound, p->isetup_pad, matrixrvp);

          for(j = 0; j < irlength; j++)
            {
              matrixlup[j] = matrixlup[j] + (j < overlapsize ?
                                             olmatrixlup[j] : FL(1.0));
              matrixrvp[j] = matrixrvp[j] + (j < overlapsize ?
                                             olmatrixrvp[j] : FL(1.0));
            }

          /* store overlap for next time */
          for(j = 0; j < overlapsize; j++)
            {
              olmatrixlup[j] = matrixlup[j + irlength];
              olmatrixrvp[j] = matrixrvp[j + irlength];
            }

          /* coherence formula */
          for(j = 0; j < irlength; j++)
            {
              hrtflp[j] = matrixlup[j] + matrixrvp[j];
              hrtfrp[j] = matrixlup[j] - matrixrvp[j];
            }

          for(j = irlength; j < irlengthpad; j++)
            {
              hrtflp[j] = FL(0.0);
              hrtfrp[j] = FL(0.0);
            }

          /* fft result from matrices */
          csound->RealFFT(csound, p->setup_pad,hrtflp);
          csound->RealFFT(csound, p->setup_pad,hrtfrp);

          /* convolution: spectral multiplication */
          csound->RealFFTMult(csound, hrtflp, hrtflp, filtpadp,
                              irlengthpad, FL(1.0));
          csound->RealFFTMult(csound, hrtfrp, hrtfrp, filtpadp,
                              irlengthpad, FL(1.0));

          /* ifft result */
          csound->RealFFT(csound, p->isetup_pad,hrtflp);
          csound->RealFFT(csound, p->isetup_pad,hrtfrp);

          /* scale */
          for(j = 0; j < irlengthpad; j++)
            {
              hrtflp[j] = hrtflp[j]/(sr / FL(38000.0));
              hrtfrp[j] = hrtfrp[j]/(sr / FL(38000.0));
            }

          for(j = 0; j < irlength; j++)
            {
              hrtflp[j] = hrtflp[j] + (j < overlapsize ?
                                       olhrtflp[j] : FL(0.0));
              hrtfrp[j] = hrtfrp[j] + (j < overlapsize ?
                                       olhrtfrp[j] : FL(0.0));
            }

          /* store overlap for next time */
          for(j = 0; j < overlapsize; j++)
            {
              olhrtflp[j] = hrtflp[j + irlength];
              olhrtfrp[j] = hrtfrp[j + irlength];
            }

          counter = 0;
        }       /* end of irlength loop */
    }       /* end of ksmps loop */

  /* keep for next time */
  p->counter = counter;

  p->u = u;
  p->v = v;
  p->w = w;
  p->x = x;
  p->y = y;
  p->z = z;
  if(M == 12 || M == 24)
    {
      p->ut = ut;
      p->vt = vt;
      p->wt = wt;
      p->xt = xt;
      p->yt = yt;
      p->zt = zt;
    }
  if(M == 24)
    {
      p->utf1 = utf1;
      p->vtf1 = vtf1;
      p->wtf1 = wtf1;
      p->xtf1 = xtf1;
      p->ytf1 = ytf1;
      p->ztf1 = ztf1;
      p->utf2 = utf2;
      p->vtf2 = vtf2;
      p->wtf2 = wtf2;
      p->xtf2 = xtf2;
      p->ytf2 = ytf2;
      p->ztf2 = ztf2;
    }

  p->inoldl = inoldl;
  p->inoldr = inoldr;

  return OK;
}

static OENTRY hrtfreverb_localops[] =
  {        {
      "hrtfreverb", sizeof(hrtfreverb), 0, "aai", "aiiSSoop",
      (SUBR)hrtfreverb_init, (SUBR)hrtfreverb_process
    }
  };


LINKAGE_BUILTIN(hrtfreverb_localops)
