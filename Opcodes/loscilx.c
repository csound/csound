/*
    loscilx.c:

    Copyright (C) 2006 Istvan Varga

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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"
#include "soundio.h"

typedef struct SNDLOAD_OPCODE_ {
    OPDS    h;
    /* -------- */
    MYFLT   *Sfname, *iFormat, *iChannels, *iSampleRate;
    MYFLT   *iBaseFreq, *iAmpScale;
    MYFLT   *iStartOffset;
    MYFLT   *iLoopMode1, *iLoopStart1, *iLoopEnd1;
} SNDLOAD_OPCODE;

static int sndload_opcode_init_(CSOUND *csound, SNDLOAD_OPCODE *p, int isstring)
{
    char        *fname;
    SNDMEMFILE  *sf;
    SF_INFO     sfinfo;
    int         sampleFormat, loopMode;

    if (isstring) fname = ((STRINGDAT *)p->Sfname)->data;
    else {
      if(ISSTRCOD(*p->Sfname))
        fname = csound->Strdup(csound, get_arg_string(csound, *p->Sfname));
      else
        fname = csound->strarg2name(csound, (char*) NULL, p->Sfname, "soundin.", 0);
    }
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sampleFormat = (int) MYFLT2LRND(*(p->iFormat));
    sfinfo.format = (int) TYPE2SF(TYP_RAW);
    switch (sampleFormat) {
    case -1: sfinfo.format = 0; break;
    case 0:  sfinfo.format |= (int) FORMAT2SF(csound->oparms->outformat); break;
    case 1:  sfinfo.format |= (int) FORMAT2SF(AE_CHAR);   break;
    case 2:  sfinfo.format |= (int) FORMAT2SF(AE_ALAW);   break;
    case 3:  sfinfo.format |= (int) FORMAT2SF(AE_ULAW);   break;
    case 4:  sfinfo.format |= (int) FORMAT2SF(AE_SHORT);  break;
    case 5:  sfinfo.format |= (int) FORMAT2SF(AE_LONG);   break;
    case 6:  sfinfo.format |= (int) FORMAT2SF(AE_FLOAT);  break;
    case 7:  sfinfo.format |= (int) FORMAT2SF(AE_UNCH);   break;
    case 8:  sfinfo.format |= (int) FORMAT2SF(AE_24INT);  break;
    case 9:  sfinfo.format |= (int) FORMAT2SF(AE_DOUBLE); break;
    default:
      csound->Free(csound, fname);
      return csound->InitError(csound, Str("invalid sample format: %d"),
                                       sampleFormat);
    }
    if (sfinfo.format) {
      int   tmp;
      tmp = (int) MYFLT2LRND(*(p->iChannels));
      sfinfo.channels = (tmp > 0 ? tmp : 1);
      tmp = (int) MYFLT2LRND(*(p->iSampleRate));
      sfinfo.samplerate = (tmp > 0 ? tmp : (int) MYFLT2LRND(CS_ESR));
    }
    sf = csound->LoadSoundFile(csound, fname,  &sfinfo);
    if (UNLIKELY(sf == NULL)) {
      int xx = csound->InitError(csound, Str("could not load '%s'"), fname);
      csound->Free(csound, fname);
      return xx;
    }
    csound->Free(csound, fname);
    if (*(p->iBaseFreq) > FL(0.0))
      sf->baseFreq = (double) *(p->iBaseFreq);
    if (*(p->iAmpScale) != FL(0.0))
      sf->scaleFac = (double) *(p->iAmpScale);
    if (*(p->iStartOffset) >= FL(0.0))
      sf->startOffs = (double) *(p->iStartOffset);
    loopMode = (int) MYFLT2LRND(*(p->iLoopMode1));
    if (loopMode >= 0) {
      if (UNLIKELY(loopMode > 3))
        return csound->InitError(csound, Str("invalid loop mode: %d"),
                                         loopMode);
      sf->loopMode = loopMode + 1;
      sf->loopStart = *(p->iLoopStart1);
      sf->loopEnd = *(p->iLoopEnd1);
    }
    if (sf->loopMode < 2 || sf->loopStart == sf->loopEnd) {
      sf->loopStart = 0.0;
      sf->loopEnd = (double) ((int32) sf->nFrames);
    }
    else if (sf->loopStart > sf->loopEnd) {
      double  tmp = sf->loopStart;
      sf->loopStart = sf->loopEnd;
      sf->loopEnd = tmp;
    }

    return OK;
}

static int sndload_opcode_init(CSOUND *csound, SNDLOAD_OPCODE *p)
{
    return sndload_opcode_init_(csound,p,0);
}

static int sndload_opcode_init_S(CSOUND *csound, SNDLOAD_OPCODE *p)
{
    return sndload_opcode_init_(csound,p,1); /* JPff: surely wrong as 0 */
}


 /* ------------------------------------------------------------------------ */

#define LOSCILX_MAXOUTS         (16)
#define LOSCILX_MAX_INTERP_SIZE (256)
#define LOSCILX_PHASE_SCALE     (4294967296.0)

typedef struct LOSCILX_OPCODE_ {
    OPDS    h;
    /* -------- */
    MYFLT   *ar[LOSCILX_MAXOUTS];
    MYFLT   *xamp, *kcps, *ifn, *iwsize, *ibas, *istrt;
    MYFLT   *imod1, *ibeg1, *iend1;
    /* -------- */
    int_least64_t   curPos, curPosInc;
    int             curLoopDir, curLoopMode;
    int_least64_t   curLoopStart, curLoopEnd;
    MYFLT   prvKcps, frqScale, ampScale, warpFact, winFact;
    void    *dataPtr;
    int32   nFrames;
    int     nChannels;
    int     winSize;
    int     enableWarp;         /* non-zero when downsampling */
    int     usingFtable;
    int     arateXamp;
    int     loopingWholeFile;
} LOSCILX_OPCODE;

static inline int_least64_t loscilx_convert_phase(double phs)
{
    double  tmp = phs * LOSCILX_PHASE_SCALE;
#ifdef HAVE_C99
    return (int_least64_t) llrint(tmp);
#else
    return (int_least64_t) (tmp + (tmp < 0.5 ? -0.5 : 0.5));
#endif
}

static inline int32 loscilx_phase_int(int_least64_t phs)
{
    int32_t retval = (int32_t) ((uint32_t) ((uint_least64_t) phs >> 32));
#ifndef __i386__
    if ((int32_t) 0x80000000U >= (int32_t) 0)   /* for safety only */
      retval += (-((int32_t) 0x40000000) << 2);
#endif
    return retval;
}

static inline double loscilx_phase_frac(int_least64_t phs)
{
    return ((double) ((int) (((uint32_t) ((uint_least64_t) phs)
                             & (uint32_t) 0xFFFFFFFFU) >> 1))
            * (1.0 / 2147483648.0));
}

static int loscilx_opcode_init(CSOUND *csound, LOSCILX_OPCODE *p)
{
    void    *dataPtr = NULL;
    int     nChannels, loopMode;
    double  frqScale = 1.0;

    p->dataPtr = NULL;
    nChannels = csound->GetOutputArgCnt(p);
    if (UNLIKELY(nChannels < 1 || nChannels > LOSCILX_MAXOUTS))
      return csound->InitError(csound,
                               Str("loscilx: invalid number of output arguments"));
    p->nChannels = nChannels;
    if (ISSTRCOD(*p->ifn)) {
      SNDMEMFILE  *sf;

      p->usingFtable = 0;
      sf = csound->LoadSoundFile(csound,
                                 (char*) get_arg_string(csound, *p->ifn),
                                 (SF_INFO *) NULL);
      if (UNLIKELY(sf == NULL))
        return csound->InitError(csound, Str("could not load '%s'"),
                                         (char*) p->ifn);
      if (sf->loopMode < 2 || sf->loopStart == sf->loopEnd) {
        sf->loopStart = 0.0;
        sf->loopEnd = (double) ((int32) sf->nFrames);
      }
      else if (sf->loopStart > sf->loopEnd) {
        double  tmp = sf->loopStart;
        sf->loopStart = sf->loopEnd;
        sf->loopEnd = tmp;
      }
      if (UNLIKELY(sf->nChannels != nChannels))
        return csound->InitError(csound, Str("number of output arguments "
                                             "inconsistent with number of "
                                             "sound file channels"));
      dataPtr = (void*) &(sf->data[0]);
      p->curPos = loscilx_convert_phase(sf->startOffs);
      p->curLoopMode = sf->loopMode - 1;
      if (p->curLoopMode < 1 || p->curLoopMode > 3)
        p->curLoopMode = 0;
      else {
        p->curLoopStart = loscilx_convert_phase(sf->loopStart);
        p->curLoopEnd = loscilx_convert_phase(sf->loopEnd);
      }
      if (*(p->ibas) > FL(0.0)) {
        frqScale = sf->sampleRate
                   / ((double) CS_ESR * (double) *(p->ibas));
      }
      else
        frqScale = sf->sampleRate / ((double) CS_ESR * sf->baseFreq);
      p->ampScale = (MYFLT) sf->scaleFac * csound->e0dbfs;
      p->nFrames = (int32) sf->nFrames;
    }
    else {
      FUNC  *ftp;

      p->usingFtable = 1;
      ftp = csound->FTnp2Find(csound, p->ifn);
      if (ftp == NULL)
        return NOTOK;
      if (UNLIKELY((int) ftp->nchanls != nChannels))
        return csound->InitError(csound, Str("number of output arguments "
                                             "inconsistent with number of "
                                             "sound file channels"));
      dataPtr = (void*) &(ftp->ftable[0]);
      p->curPos = (int_least64_t) 0;
      switch ((int) ftp->loopmode1) {
      case 1:
        p->curLoopMode = 1;
        break;
      case 2:
        p->curLoopMode = 3;
        break;
      default:
        p->curLoopMode = 0;
      }
      p->curLoopStart = (int_least64_t) ftp->begin1 << 32;
      p->curLoopEnd = (int_least64_t) ftp->end1 << 32;
      if (*(p->ibas) > FL(0.0)) {
        if (ftp->gen01args.sample_rate > FL(0.0))
          frqScale = (double) ftp->gen01args.sample_rate
                     / ((double) CS_ESR * (double) *(p->ibas));
        else
          frqScale = 1.0 / (double) *(p->ibas);
      }
      else if (ftp->cpscvt > FL(0.0)) {
        frqScale = (double) ftp->cpscvt * (1.0 / (double) LOFACT);
      }
      else if (ftp->gen01args.sample_rate > FL(0.0))
        frqScale = (double) ftp->gen01args.sample_rate / (double) CS_ESR;
      p->ampScale = FL(1.0);
      p->nFrames = ftp->flenfrms + 1L;
    }
    if (*(p->istrt) >= FL(0.0))
      p->curPos = loscilx_convert_phase((double) *(p->istrt));
    p->curPosInc = (int_least64_t) 0;
    p->curLoopDir = 1;
    loopMode = (int) MYFLT2LRND(*(p->imod1));
    if (loopMode >= 0) {
      if (UNLIKELY(loopMode > 3))
        return csound->InitError(csound, Str("invalid loop mode: %d"),
                                         loopMode);
      p->curLoopMode = loopMode;
      p->curLoopStart = loscilx_convert_phase((double) *(p->ibeg1));
      p->curLoopEnd = loscilx_convert_phase((double) *(p->iend1));
    }
    if (p->curLoopMode <= 0 || p->curLoopStart == p->curLoopEnd) {
      p->curLoopStart = (int_least64_t) 0;
      p->curLoopEnd = (int_least64_t) p->nFrames << 32;
    }
    else if (p->curLoopStart > p->curLoopEnd) {
      int_least64_t tmp = p->curLoopStart;
      p->curLoopStart = p->curLoopEnd;
      p->curLoopEnd = tmp;
    }
    p->prvKcps = FL(0.0);
    p->frqScale = (MYFLT) (frqScale * LOSCILX_PHASE_SCALE);
    p->warpFact = FL(1.0);
    p->winSize = (int) MYFLT2LRND(*(p->iwsize));
    if (p->winSize < 1)
      p->winSize = 4;                   /* default to cubic interpolation */
    else if (p->winSize > 2) {
      if (p->winSize > LOSCILX_MAX_INTERP_SIZE)
        p->winSize = LOSCILX_MAX_INTERP_SIZE;
      else
        p->winSize = (p->winSize + 2) & (~((int) 3));
      if (p->winSize > 4) {
        /* constant for window calculation */
        p->winFact =
          (FL(1.0) - POWER(p->winSize * FL(0.85172), -FL(0.89624)))
          / ((p->winSize * p->winSize) >> 2);
      }
    }
    p->enableWarp = 0;
    if (IS_ASIG_ARG(p->xamp))
      p->arateXamp = ~((int) 0);        /* used as a bit mask */
    else
      p->arateXamp = 0;
    p->loopingWholeFile = 0;
    if (p->curLoopMode == 1) {
      if (loscilx_phase_int(p->curLoopStart + (int_least64_t) 0x80000000U)
          == 0L) {
        if (loscilx_phase_int(p->curLoopEnd + (int_least64_t) 0x80000000U)
            == p->nFrames)
          p->loopingWholeFile = 1;
      }
    }
    p->dataPtr = dataPtr;

    return OK;
}

/* ------------- set up fast sine generator ------------- */
/* Input args:                                            */
/*   a: amplitude                                         */
/*   f: frequency (-PI - PI)                              */
/*   p: initial phase (0 - PI/2)                          */
/*   c: 2.0 * cos(f) - 2.0                                */
/* Output args:                                           */
/*  *x: first output sample                               */
/*  *v: coefficients for calculating next sample as       */
/*      shown below:                                      */
/*            v = v + c * x                               */
/*            x = x + v                                   */
/*          These values are calculated as follows:       */
/*            x = y[0]                                    */
/*            v = y[1] - (c + 1.0) * y[0]                 */
/*          where y[0], and y[1] are the first, and       */
/*          second sample of the sine wave to be          */
/*          generated, respectively.                      */
/* -------- written by Istvan Varga, Jan 28 2002 -------- */

static inline void init_sine_gen(double a, double f, double p, double c,
                                 double *x, double *v)
{
    double  y0, y1;             /* these should be doubles */

    y0 = sin(p);
    y1 = sin(p + f);
    *x = y0;
    *v = y1 - (c * y0) - y0;
    /* amp. scale */
    *x *= a; *v *= a;
}

static int loscilx_opcode_perf(CSOUND *csound, LOSCILX_OPCODE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int     j;
    double  frac_d, pidwarp_d = 0.0, c = 0.0;
    MYFLT   frac, ampScale, winFact = p->winFact;
    int32   ndx;
    int     winSmps;
    float   winBuf[LOSCILX_MAX_INTERP_SIZE];

    if (UNLIKELY(p->dataPtr == NULL)) goto err1;

    if (*(p->kcps) != p->prvKcps) {
      double  f;
      p->prvKcps = *(p->kcps);
      f = (double) p->prvKcps * (double) p->frqScale;
#ifdef HAVE_C99
      p->curPosInc = (int_least64_t) llrint(f);
#else
      p->curPosInc = (int_least64_t) (f + (f < 0.0 ? -0.5 : 0.5));
#endif
      if (p->winSize > 4) {
        int_least64_t   nn = ((int_least64_t) 0x00010001 << 16);
        /* calculate window "warp" parameter for sinc interpolation */
        if (p->curPosInc > nn || p->curPosInc < (-nn)) {
          if (p->curPosInc >= (int_least64_t) 0)
            p->warpFact = (MYFLT) nn / (MYFLT) p->curPosInc;
          else
            p->warpFact = (MYFLT) (-nn) / (MYFLT) p->curPosInc;
          if (p->warpFact < (FL(2.0) / (MYFLT) p->winSize))
            p->warpFact = (FL(2.0) / (MYFLT) p->winSize);
          p->enableWarp = 1;
        }
        else
          p->enableWarp = 0;
      }
    }
    if (p->enableWarp) {
      double  tmp1, tmp2;

      pidwarp_d = PI * (double) p->warpFact;
      c = 2.0 * cos(pidwarp_d) - 2.0;
      /* correct window for kwarp */
      tmp1 = tmp2 = (double) (p->winSize >> 1);
      tmp1 *= tmp1;
      tmp1 = 1.0 / tmp1;
      tmp2 *= (double) p->warpFact;
      tmp2 -= (double) ((int) tmp2) + 0.5;
      tmp2 *= (4.0 * tmp2);
      winFact = (MYFLT) (((double) p->winFact - tmp1) * tmp2 + tmp1);
    }
    ampScale = *(p->xamp) * p->ampScale;
    if (UNLIKELY(offset)) memset(p->ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i = offset; i<nsmps; i++) {

      frac_d = loscilx_phase_frac(p->curPos);
      frac = (MYFLT) frac_d;
      ndx = loscilx_phase_int(p->curPos);
      if (i & p->arateXamp)
        ampScale = p->xamp[i] * p->ampScale;

      /* calculate interpolation window */

      winSmps = p->winSize;
      switch (p->winSize) {
      case 1:                                   /* no interpolation */
        winBuf[0] = 1.0f;
        break;
      case 2:                                   /* linear interpolation */
        winBuf[0] = (float) (FL(1.0) - frac);
        winBuf[1] = (float) frac;
        break;
      case 4:                                   /* cubic interpolation */
        {
          MYFLT   a0, a1, a2, a3;

          ndx--;
          a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
          a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
          a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
          winBuf[0] = (float) (a0 * frac);
          winBuf[1] = (float) (a1 * frac + FL(1.0));
          winBuf[2] = (float) (a2 * frac);
          winBuf[3] = (float) (a3 * frac);
        }
        break;
      default:                                  /* sinc interpolation */
        {
          double  d, x, v;
          MYFLT   a0, a1;
          int     wsized2 = winSmps >> 1;

          ndx += (int32) (1 - wsized2);
          d = (double) (1 - wsized2) - frac_d;
          j = 0;
          if (p->enableWarp) {              /* ...with window warp enabled */
            init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
            /* samples -(window size / 2 - 1) to -1 */
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              winBuf[j++] = (float) a1;
              d += 1.0; v += c * x; x += v;
            } while (j < (wsized2 - 1));
            /* sample 0 */
            /* avoid division by zero */
            if (frac_d < 0.00003) {
              a1 = p->warpFact;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
            }
            winBuf[j++] = (float) a1;
            d += 1.0; v += c * x; x += v;
            /* sample 1 */
            /* avoid division by zero */
            if (frac_d > 0.99997) {
              a1 = p->warpFact;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
            }
            winBuf[j++] = (float) a1;
            d += 1.0; v += c * x; x += v;
            /* samples 2 to (window size / 2) */
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              winBuf[j++] = (float) a1;
              d += 1.0; v += c * x; x += v;
            } while (j < p->winSize);
          }
          else {                            /* ...with window warp disabled */
            /* avoid division by zero */
            if (frac_d < 0.00001 || frac_d > 0.99999) {
              ndx += (int32) (wsized2 - (frac_d < 0.5 ? 1 : 0));
              winSmps = 1;
              winBuf[0] = 1.0f;
            }
            else {
              a0 = (MYFLT) (sin(PI * frac_d) / PI);
              do {
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = a0 * a1 * a1 / (MYFLT) d;
                winBuf[j++] = (float) a1;
                d += 1.0;
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = -(a0 * a1 * a1 / (MYFLT) d);
                winBuf[j++] = (float) a1;
                d += 1.0;
              } while (j < p->winSize);
            }
          }
        }
      }

      /* generate sound output */

      ndx--;
      j = 0;
      if (p->nChannels == 1) {                  /* mono */
        MYFLT   ar = FL(0.0);
        do {
          ndx++;
          if ((uint32) ndx >= (uint32) p->nFrames) {
            if (!p->loopingWholeFile)
              continue;
            if (ndx < 0L) {
              do {
                ndx += p->nFrames;
              } while (ndx < 0L);
            }
            else {
              do {
                ndx -= p->nFrames;
              } while (ndx >= p->nFrames);
            }
          }
#ifdef USE_DOUBLE
          if (p->usingFtable) {
            ar += (((MYFLT*) p->dataPtr)[ndx] * (MYFLT) winBuf[j]);
          }
          else
#endif
          {
            ar += ((MYFLT) ((float*) p->dataPtr)[ndx] * (MYFLT) winBuf[j]);
          }
        } while (++j < winSmps);
        /* scale output */
        p->ar[0][i] = ar * ampScale;
      }
      else if (p->nChannels == 2) {             /* stereo */
        MYFLT   ar1 = FL(0.0), ar2 = FL(0.0);
        do {
          ndx++;
          if ((uint32) ndx >= (uint32) p->nFrames) {
            if (!p->loopingWholeFile)
              continue;
            if (ndx < 0L) {
              do {
                ndx += p->nFrames;
              } while (ndx < 0L);
            }
            else {
              do {
                ndx -= p->nFrames;
              } while (ndx >= p->nFrames);
            }
          }
#ifdef USE_DOUBLE
          if (p->usingFtable) {
            ar1 += (((MYFLT*) p->dataPtr)[ndx + ndx] * (MYFLT) winBuf[j]);
            ar2 += (((MYFLT*) p->dataPtr)[ndx + ndx + 1L] * (MYFLT) winBuf[j]);
          }
          else
#endif
          {
            ar1 += ((MYFLT) ((float*) p->dataPtr)[ndx + ndx]
                    * (MYFLT) winBuf[j]);
            ar2 += ((MYFLT) ((float*) p->dataPtr)[ndx + ndx + 1L]
                    * (MYFLT) winBuf[j]);
          }
        } while (++j < winSmps);
        /* scale output */
        p->ar[0][i] = ar1 * ampScale;
        p->ar[1][i] = ar2 * ampScale;
      }
      else {                                    /* generic multichannel code */
        int     k = 0;
        do {
          p->ar[k][i] = FL(0.0);
        } while (++k < p->nChannels);
        do {
          ndx++;
          if ((uint32) ndx >= (uint32) p->nFrames) {
            if (!p->loopingWholeFile)
              continue;
            if (ndx < 0L) {
              do {
                ndx += p->nFrames;
              } while (ndx < 0L);
            }
            else {
              do {
                ndx -= p->nFrames;
              } while (ndx >= p->nFrames);
            }
          }
#ifdef USE_DOUBLE
          if (p->usingFtable) {
            MYFLT *fp = &(((MYFLT*) p->dataPtr)[ndx * (int32) p->nChannels]);

            k = 0;
            do {
              p->ar[k][i] += (fp[k] * (MYFLT) winBuf[j]);
            } while (++k < p->nChannels);
          }
          else
#endif
          {
            float *fp = &(((float*) p->dataPtr)[ndx * (int32) p->nChannels]);

            k = 0;
            do {
              p->ar[k][i] += ((MYFLT) fp[k] * (MYFLT) winBuf[j]);
            } while (++k < p->nChannels);
          }
        } while (++j < winSmps);
        /* scale output */
        k = 0;
        do {
          p->ar[k][i] *= ampScale;
        } while (++k < p->nChannels);
      }

      /* update playback position */

      if (p->curLoopMode) {
        int_least64_t   prvPos = p->curPos;

        if (p->curLoopDir > 0)
          p->curPos += p->curPosInc;
        else
          p->curPos -= p->curPosInc;
        if (p->curPos < p->curLoopStart) {
          if (prvPos >= p->curLoopStart) {
            switch (p->curLoopMode) {
            case 1:
            case 2:
              p->curPos = p->curPos + (p->curLoopEnd - p->curLoopStart);
              break;
            case 3:
              p->curPos = p->curLoopStart + (p->curLoopStart - p->curPos);
              p->curLoopDir = -(p->curLoopDir);
              break;
            }
          }
        }
        else if (p->curPos >= p->curLoopEnd) {
          if (prvPos < p->curLoopEnd) {
            switch (p->curLoopMode) {
            case 1:
              p->curPos = p->curPos + (p->curLoopStart - p->curLoopEnd);
              break;
            case 2:
            case 3:
              p->curPos = p->curLoopEnd + (p->curLoopEnd - p->curPos);
              p->curLoopDir = -(p->curLoopDir);
              break;
            }
          }
        }
      }
      else
        p->curPos += p->curPosInc;

    }

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("loscilx: not initialised"));
}

 /* ------------------------------------------------------------------------ */

static OENTRY loscilx_localops[] = {
  { "sndload",  sizeof(SNDLOAD_OPCODE), _QQ, 1,  "",                 "iooooojjoo",
    (SUBR) sndload_opcode_init, (SUBR) NULL, (SUBR) NULL                      },
 { "sndload.S",  sizeof(SNDLOAD_OPCODE), _QQ, 1,  "",                 "Sooooojjoo",
    (SUBR) sndload_opcode_init_S, (SUBR) NULL, (SUBR) NULL                      },
  { "loscilx",  sizeof(LOSCILX_OPCODE), TR, 5,  "mmmmmmmmmmmmmmmm", "xkioojjoo",
    (SUBR) loscilx_opcode_init, (SUBR) NULL, (SUBR) loscilx_opcode_perf       }
};

LINKAGE_BUILTIN(loscilx_localops)
