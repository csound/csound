/*
    diskin2.c:

    Copyright (C) 2005 Istvan Varga

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

#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"
#include "diskin2.h"

#include <sndfile.h>
#include <math.h>

static void diskin2_read_buffer(DISKIN2 *p, int bufReadPos)
{
    MYFLT *tmp;
    long  nsmps;
    int   i;

    /* swap buffer pointers */
    tmp = p->buf;
    p->buf = p->prvBuf;
    p->prvBuf = tmp;
    /* check if requested data can be found in previously used buffer */
    i = (int) ((long) bufReadPos + (p->bufStartPos - p->prvBufStartPos));
    if (i >= 0 && i < p->bufSize) {
      long  tmp2;
      /* yes, only need to swap buffers and return */
      tmp2 = p->bufStartPos;
      p->bufStartPos = p->prvBufStartPos;
      p->prvBufStartPos = tmp2;
      return;
    }
    /* save buffer position */
    p->prvBufStartPos = p->bufStartPos;
    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (long) bufReadPos;
    p->bufStartPos &= (~((long) (p->bufSize - 1)));
    if (p->bufStartPos < 0L) {
      /* before beginning of file ? just fill buffer with zero samples */
      for (i = 0; i < (p->bufSize * p->nChannels); i++)
        p->buf[i] = FL(0.0);
    }
    else {
      /* number of sample frames to read */
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0L) {         /* if there is anything to read: */
        if (nsmps > (long) p->bufSize)
          nsmps = (long) p->bufSize;
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        nsmps *= (long) p->nChannels;
        i = (int) sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (i < 0)  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
      else
        i = 0;      /* nothing to read: clear entire buffer to zero */
      /* fill rest of buffer with zero samples */
      while (i < (p->bufSize * p->nChannels))
        p->buf[i++] = FL(0.0);
    }
}

/* Mix one sample frame from input file at location 'pos' to outputs    */
/* of opcode 'p', at sample index 'n' (0 <= n < ksmps), with amplitude  */
/* scale 'scl'.                                                         */

#define DISKIN2_GET_SAMPLE(p,pos,n,scl)                     \
{                                                           \
    long filePos_;                                          \
    int  bufPos_, i_;                                       \
                                                            \
    filePos_ = (long) (pos);                                \
    if (p->wrapMode) {                                      \
      if (filePos_ >= p->fileLength)                        \
        filePos_ -= p->fileLength;                          \
      else if (filePos_ < 0L)                               \
        filePos_ += p->fileLength;                          \
    }                                                       \
    bufPos_ = (int) (filePos_ - p->bufStartPos);            \
    if (bufPos_ < 0 || bufPos_ >= p->bufSize) {             \
      /* not in current buffer frame, need to read file */  \
      diskin2_read_buffer(p, bufPos_);                      \
      /* recalculate buffer position */                     \
      bufPos_ = (int) (filePos_ - p->bufStartPos);          \
    }                                                       \
    /* copy all channels from buffer */                     \
    bufPos_ *= p->nChannels;                                \
    i_ = 0;                                                 \
    do {                                                    \
      p->aOut[i_][n] += (MYFLT) (scl) * p->buf[bufPos_++];  \
    } while (++i_ < p->nChannels);                          \
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

#define INIT_SINE_GEN(a,f,p,c,x,v)                              \
{                                                               \
    double  y0_, y1_;           /* these should be doubles */   \
                                                                \
    y0_ = sin(p);                                               \
    y1_ = sin((p) + (f));                                       \
    *(x) = y0_;                                                 \
    *(v) = y1_ - ((c) * y0_) - y0_;                             \
    /* amp. scale */                                            \
    *(x) *= (a); *(v) *= (a);                                   \
}

/* calculate buffer size in sample frames */

static int diskin2_calc_buffer_size(DISKIN2 *p, int n_monoSamps)
{
    int i, nFrames;

    /* default to 4096 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 4096;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 1;
    while (nFrames < i)
      nFrames <<= 1;
    /* limit to sane range */
    if (nFrames < 512)
      nFrames = 512;
    else if (nFrames < p->winSize)
      nFrames = 1024;
    else if (nFrames > 1048576)
      nFrames = 1048576;

    return nFrames;
}

int diskin2_init(ENVIRON *csound, DISKIN2 *p)
{
    double  pos;
    char    *s, name[1024];
    SF_INFO sfinfo;
    int     i, n;

    /* check number of channels */
    p->nChannels = (int) (p->OUTOCOUNT);
    if (p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN) {
      initerror(Str("diskin2: invalid number of channels"));
      return NOTOK;
    }
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (*(p->iSkipInit) != FL(0.0))
        return OK;
      fdclose(&(p->fdch));
    }
    /* open file */
    if (*(p->iFileCode) == SSTRCOD && p->STRARG != NULL) {
      s = p->STRARG;
      i = 0;
      /* unquote name */
      while (*s == '"')
        s++;
      while (*s != '"' && *s != '\0')
        name[i++] = *(s++);     /* FIXME: can overflow with very long string */
      name[i] = '\0';
    }
    else
      sprintf(name, "soundin.%d", (int) (*(p->iFileCode)
                                         + (*(p->iFileCode) >= FL(0.0) ?
                                            FL(0.5) : FL(-0.5))));
    s = csound->FindInputFile(csound, &(name[0]), "SFDIR;SSDIR");
    if (s == NULL) {
      csound->Message(csound, Str("diskin2: opening '%s':\n"), &(name[0]));
      initerror(Str("cannot find file in any of the search paths"));
      return NOTOK;
    }
    memset(&sfinfo, 0, sizeof(SF_INFO));
    p->sf = sf_open(s, SFM_READ, &sfinfo);
    if (p->sf == NULL) {
      /* file may be raw, set default format parameters */
      memset(&sfinfo, 0, sizeof(SF_INFO));
      sfinfo.samplerate = (int) (csound->esr_ + FL(0.5));
      sfinfo.channels = p->nChannels;
      sfinfo.format = SF_FORMAT_RAW;
      /* check for user specified sample format */
      n = (int) (*(p->iSampleFormat) + FL(0.5));
      if (n <= 0)
        n = 4;      /* <= 0 ? default to 16 bit signed integer */
      switch (n) {
        case 1: sfinfo.format |= SF_FORMAT_PCM_S8;  break;
        case 2: sfinfo.format |= SF_FORMAT_ALAW;    break;
        case 3: sfinfo.format |= SF_FORMAT_ULAW;    break;
        case 4: sfinfo.format |= SF_FORMAT_PCM_16;  break;
        case 5: sfinfo.format |= SF_FORMAT_PCM_32;  break;
        case 6: sfinfo.format |= SF_FORMAT_FLOAT;   break;
        case 7: sfinfo.format |= SF_FORMAT_PCM_U8;  break;
        case 8: sfinfo.format |= SF_FORMAT_PCM_24;  break;
        case 9: sfinfo.format |= SF_FORMAT_DOUBLE;  break;
        default:
          initerror(Str("diskin2: unknown sample format"));
          mfree(csound, s);
          return NOTOK;
      }
      /* re-open as raw file */
      p->sf = sf_open(s, SFM_READ, &sfinfo);
    }
    if (p->sf == NULL) {
      csound->Message(csound, Str("diskin2: opening '%s':\n"), s);
      initerror(Str("sf_open() failed"));
      mfree(csound, s);
      return NOTOK;
    }
    /* print file information */
    if (csound->GetMessageLevel(csound) != 0) {
      csound->Message(csound, Str("diskin2: opened '%s':\n"), s);
      csound->Message(csound, Str("         %d Hz, %d channel(s), "
                                  "%ld sample frames\n"), sfinfo.samplerate,
                                  sfinfo.channels, (long) sfinfo.frames);
    }
    mfree(csound, s);
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = (void*) p->sf;
    fdrecord(&(p->fdch));
    /* check number of channels in file (must equal the number of outargs) */
    if (sfinfo.channels != p->nChannels) {
      initerror(Str("diskin2: number of output args inconsistent with "
                    "number of file channels"));
      return NOTOK;
    }
    /* skip initialisation if requested */
    if (p->initDone && *(p->iSkipInit) != FL(0.0))
      return OK;
    /* interpolation window size: valid settings are 1 (no interpolation), */
    /* 2 (linear interpolation), 4 (cubic interpolation), and integer */
    /* multiples of 4 in the range 8 to 1024 (sinc interpolation) */
    p->winSize = (int) (*(p->iWinSize) + FL(0.5));
    if (p->winSize < 1)
      p->winSize = 4;               /* use cubic interpolation by default */
    else if (p->winSize > 2) {
      /* cubic/sinc: round to nearest integer multiple of 4 */
      p->winSize = (p->winSize + 2) & (~3);
      if (p->winSize > 1024)
        p->winSize = 1024;
    }
    /* set file parameters from header info */
    p->fileLength = (long) sfinfo.frames;
    p->warpScale = 1.0;
    if ((int) (csound->esr_ + FL(0.5)) != sfinfo.samplerate) {
      if (p->winSize != 1) {
        /* will automatically convert sample rate if interpolation is enabled */
        p->warpScale = (double) sfinfo.samplerate / (double) csound->esr_;
      }
      else {
        csound->Message(csound,
                        Str("diskin2: warning: file sample rate (%d) "
                            "!= orchestra sr (%d)\n"),
                        sfinfo.samplerate, (int) (csound->esr_ + FL(0.5)));
      }
    }
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (p->fileLength < 1L)
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double) *(p->iSkipTime) * (double) csound->esr_ * p->warpScale;
    pos *= (double) POS_FRAC_SCALE;
    p->pos_frac = (int64_t) (pos >= 0.0 ? (pos + 0.5) : (pos - 0.5));
    if (p->wrapMode) {
      p->pos_frac %= ((int64_t) p->fileLength << POS_FRAC_SHIFT);
      if (p->pos_frac < (int64_t) 0)
        p->pos_frac += ((int64_t) p->fileLength << POS_FRAC_SHIFT);
    }
    p->pos_frac_inc = (int64_t) 0;
    p->prv_kTranspose = FL(0.0);
    /* constant for window calculation */
    p->winFact = (MYFLT) ((1.0 - pow((double) p->winSize * 0.85172, -0.89624))
                          / ((double) (p->winSize * p->winSize) * 0.25));
    /* allocate and initialise buffers */
    p->bufSize = diskin2_calc_buffer_size(p, (int) (*(p->iBufSize) + FL(0.5)));
    n = 2 * p->bufSize * p->nChannels * (int) sizeof(MYFLT);
    if (n != (int) p->auxData.size)
      csound->AuxAlloc(csound, (long) n, &(p->auxData));
    p->bufStartPos = p->prvBufStartPos = -((long) p->bufSize);
    n = p->bufSize * p->nChannels;
    p->buf = (MYFLT*) (p->auxData.auxp);
    p->prvBuf = (MYFLT*) p->buf + (int) n;
    for (i = 0; i < n; i++)
      p->buf[i] = FL(0.0);
    /* done initialisation */
    p->initDone = 1;
    return OK;
}

#define DISKIN2_FILE_POS_INC                                        \
{                                                                   \
    p->pos_frac += p->pos_frac_inc;                                 \
    ndx = (long) (p->pos_frac >> POS_FRAC_SHIFT);                   \
    if (p->wrapMode) {                                              \
      if (ndx >= p->fileLength) {                                   \
        ndx -= p->fileLength;                                       \
        p->pos_frac -= ((int64_t) p->fileLength << POS_FRAC_SHIFT); \
      }                                                             \
      else if (ndx < 0L) {                                          \
        ndx += p->fileLength;                                       \
        p->pos_frac += ((int64_t) p->fileLength << POS_FRAC_SHIFT); \
      }                                                             \
    }                                                               \
}

int diskin2_perf(ENVIRON *csound, DISKIN2 *p)
{
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    long    ndx;
    int     i, nn, chn, wsized2, warp;

    if (p->initDone <= 0) {
      perferror(Str("diskin2: not initialised"));
      return NOTOK;
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      p->prv_kTranspose = *(p->kTranspose);
      p->pos_frac_inc = (int64_t) ((double) p->prv_kTranspose * p->warpScale
                                   * (double) POS_FRAC_SCALE + 0.5);
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps_; nn++)
        p->aOut[chn][nn] = FL(0.0);
    /* file read position (updated by DISKIN2_FILE_POS_INC macro) */
    ndx = (long) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
      case 1:                   /* ---- no interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          if (p->pos_frac & ((int64_t) POS_FRAC_SCALE >> 1))
            ndx++;                      /* round to nearest sample */
          DISKIN2_GET_SAMPLE(p, ndx, nn, FL(1.0));
          /* update file position */
          DISKIN2_FILE_POS_INC;
        }
        break;
      case 2:                   /* ---- linear interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          a1 = (MYFLT) ((int) (p->pos_frac & (int64_t) POS_FRAC_MASK))
               * (FL(1.0) / (MYFLT) POS_FRAC_SCALE);
          a0 = FL(1.0) - a1;
          DISKIN2_GET_SAMPLE(p, ndx, nn, a0);
          ndx++;
          DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
          /* update file position */
          DISKIN2_FILE_POS_INC;
        }
        break;
      case 4:                   /* ---- cubic interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          frac = (MYFLT) ((int) (p->pos_frac & (int64_t) POS_FRAC_MASK))
                 * (FL(1.0) / (MYFLT) POS_FRAC_SCALE);
          a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
          a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
          a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
          a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
          ndx--;                                /* sample -1 */
          DISKIN2_GET_SAMPLE(p, ndx, nn, a0);
          ndx++;                                /* sample 0 */
          DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
          ndx++;                                /* sample +1 */
          DISKIN2_GET_SAMPLE(p, ndx, nn, a2);
          ndx++;                                /* sample +2 */
          DISKIN2_GET_SAMPLE(p, ndx, nn, a3);
          /* update file position */
          DISKIN2_FILE_POS_INC;
        }
        break;
      default:                  /* ---- sinc interpolation ---- */
        wsized2 = p->winSize >> 1;
        nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
        if (p->pos_frac_inc > (int64_t) nn ||
            p->pos_frac_inc < (int64_t) (-nn)) {
          warp = 1;                     /* enable warp */
          onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                      ((MYFLT) nn / (MYFLT) p->pos_frac_inc)
                      : ((MYFLT) (-nn) / (MYFLT) p->pos_frac_inc));
          pidwarp_d = PI * (double) onedwarp;
          c = 2.0 * cos(pidwarp_d) - 2.0;
          /* correct window for kwarp */
          x = v = (double) wsized2; x *= x; x = 1.0 / x;
          v *= (double) onedwarp; v -= (double) ((int) v) + 0.5; v *= 4.0 * v;
          winFact = (MYFLT) (((double) p->winFact - x) * v + x);
        }
        else {
          warp = 0;
          onedwarp = FL(0.0);
          pidwarp_d = c = 0.0;
          winFact = p->winFact;
        }
        for (nn = 0; nn < csound->ksmps_; nn++) {
          frac_d = (double) ((int) (p->pos_frac & (int64_t) POS_FRAC_MASK))
                   * (1.0 / (double) POS_FRAC_SCALE);
          ndx += (long) (1 - wsized2);
          d = (double) (1 - wsized2) - frac_d;
          if (warp) {                           /* ---- warp enabled ---- */
            INIT_SINE_GEN((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
            /* samples -(window size / 2 - 1) to -1 */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
            /* sample 0 */
            /* avoid division by zero */
            if (frac_d < 0.00003) {
              a1 = onedwarp;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
            }
            DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* sample 1 */
            /* avoid division by zero */
            if (frac_d > 0.99997) {
              a1 = onedwarp;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
            }
            DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* samples 2 to (window size / 2) */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
          }
          else {                                /* ---- warp disabled ---- */
            /* avoid division by zero */
            if (frac_d < 0.00001 || frac_d > 0.99999) {
              ndx += (long) (wsized2 - (frac_d < 0.5 ? 1 : 0));
              DISKIN2_GET_SAMPLE(p, ndx, nn, FL(1.0));
            }
            else {
              a0 = (MYFLT) (sin(PI * frac_d) / PI);
              i = wsized2;
              do {
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = a0 * a1 * a1 / (MYFLT) d;
                DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
                d += 1.0;
                ndx++;
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = -(a0 * a1 * a1 / (MYFLT) d);
                DISKIN2_GET_SAMPLE(p, ndx, nn, a1);
                d += 1.0;
                ndx++;
              } while (--i);
            }
          }
          /* update file position */
          DISKIN2_FILE_POS_INC;
        }
    }
    /* apply 0dBFS scale */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps_; nn++)
        p->aOut[chn][nn] *= csound->e0dbfs_;
    return OK;
}

