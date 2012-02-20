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
#include "soundio.h"
#include "diskin2.h"
#include <math.h>

static CS_NOINLINE void diskin2_read_buffer(DISKIN2 *p, int bufReadPos)
{
    MYFLT *tmp;
    int32 nsmps;
    int   i;

    /* swap buffer pointers */
    tmp = p->buf;
    p->buf = p->prvBuf;
    p->prvBuf = tmp;
    /* check if requested data can be found in previously used buffer */
    i = (int)((int32) bufReadPos + (p->bufStartPos - p->prvBufStartPos));
    if ((unsigned int) i < (unsigned int) p->bufSize) {
      int32  tmp2;
      /* yes, only need to swap buffers and return */
      tmp2 = p->bufStartPos;
      p->bufStartPos = p->prvBufStartPos;
      p->prvBufStartPos = tmp2;
      return;
    }
    /* save buffer position */
    p->prvBufStartPos = p->bufStartPos;
    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int32) bufReadPos;
    p->bufStartPos &= (~((int32) (p->bufSize - 1)));
    i = 0;
    if (p->bufStartPos >= 0L) {
      /* number of sample frames to read */
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0L) {         /* if there is anything to read: */
        if (nsmps > (int32) p->bufSize)
          nsmps = (int32) p->bufSize;
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        nsmps *= (int32) p->nChannels;
        i = (int)sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    memset(&p->buf[i], 0, sizeof(MYFLT)*(p->bufSize * p->nChannels-i));
    /* while (i < (p->bufSize * p->nChannels)) */
    /*   p->buf[i++] = FL(0.0); */
}

/* Mix one sample frame from input file at location 'pos' to outputs    */
/* of opcode 'p', at sample index 'n' (0 <= n < ksmps), with amplitude  */
/* scale 'scl'.                                                         */

static inline void diskin2_get_sample(DISKIN2 *p, int32 fPos, int n, MYFLT scl)
{
    int  bufPos, i;

    if (p->wrapMode) {
      if (UNLIKELY(fPos >= p->fileLength))
        fPos -= p->fileLength;
      else if (UNLIKELY(fPos < 0L))
        fPos += p->fileLength;
    }
    bufPos = (int)(fPos - p->bufStartPos);
    if (UNLIKELY((unsigned int) bufPos >= (unsigned int) p->bufSize)) {
      /* not in current buffer frame, need to read file */
      diskin2_read_buffer(p, bufPos);
      /* recalculate buffer position */
      bufPos = (int)(fPos - p->bufStartPos);
    }
    /* copy all channels from buffer */
    if (p->nChannels == 1) {
      p->aOut[0][n] += scl * p->buf[bufPos];
    }
    else if (p->nChannels == 2) {
      bufPos += bufPos;
      p->aOut[0][n] += scl * p->buf[bufPos];
      p->aOut[1][n] += scl * p->buf[bufPos + 1];
    }
    else {
      bufPos *= p->nChannels;
      i = 0;
      /* p->aOut[i++][n] += scl * p->buf[bufPos++]; */
      /* p->aOut[i++][n] += scl * p->buf[bufPos++]; */
      do {
        p->aOut[i++][n] += scl * p->buf[bufPos++];
      } while (i < p->nChannels);
    }
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

/* calculate buffer size in sample frames */

static int diskin2_calc_buffer_size(DISKIN2 *p, int n_monoSamps)
{
    int i, nFrames;

    /* default to 4096 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 4096;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* limit to sane range */
    if (i < p->winSize)
      i = p->winSize;
    else if (i > 1048576)
      i = 1048576;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 64;       /* will be at least 128 sample frames */
    do {
      nFrames <<= 1;
    } while (nFrames < i);

    return nFrames;
}

static const int diskin2_format_table[11] = {
    0,
    SF_FORMAT_RAW | SF_FORMAT_PCM_16,
    SF_FORMAT_RAW | SF_FORMAT_PCM_S8,
    SF_FORMAT_RAW | SF_FORMAT_ALAW,
    SF_FORMAT_RAW | SF_FORMAT_ULAW,
    SF_FORMAT_RAW | SF_FORMAT_PCM_16,
    SF_FORMAT_RAW | SF_FORMAT_PCM_32,
    SF_FORMAT_RAW | SF_FORMAT_FLOAT,
    SF_FORMAT_RAW | SF_FORMAT_PCM_U8,
    SF_FORMAT_RAW | SF_FORMAT_PCM_24,
    SF_FORMAT_RAW | SF_FORMAT_DOUBLE
};

int diskin2_init(CSOUND *csound, DISKIN2 *p)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int     n;

    /* check number of channels */
    p->nChannels = (int)(p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN)) {
      return csound->InitError(csound, Str("diskin2: invalid number of channels"));
    }
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (*(p->iSkipInit) != FL(0.0))
        return OK;
      fdclose(csound, &(p->fdch));
    }
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = (int)(csound->esr + FL(0.5));
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = (int)(*(p->iSampleFormat) + FL(2.5)) - 1;
    if (UNLIKELY(n < 0 || n > 10))
      return csound->InitError(csound, Str("diskin2: unknown sample format"));
    sfinfo.format = diskin2_format_table[n];
    /* open file */
    /* FIXME: name can overflow with very long string */
    csound->strarg2name(csound, name, p->iFileCode, "soundin.", p->XSTRCODE);
    fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound,
                               Str("diskin2: %s: failed to open file"), name);
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
    /* print file information */
    if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
      csound->Message(csound, Str("diskin2: opened '%s':\n"
                                  "         %d Hz, %d channel(s), "
                                  "%ld sample frames\n"),
                      csound->GetFileName(fd),
                      (int)sfinfo.samplerate, (int)sfinfo.channels,
                      (int32) sfinfo.frames);
    }
    /* check number of channels in file (must equal the number of outargs) */
    if (UNLIKELY(sfinfo.channels != p->nChannels)) {
      return csound->InitError(csound,
                               Str("diskin2: number of output args "
                                   "inconsistent with number of file channels"));
    }
    /* skip initialisation if requested */
    if (p->initDone && *(p->iSkipInit) != FL(0.0))
      return OK;
    /* interpolation window size: valid settings are 1 (no interpolation), */
    /* 2 (linear interpolation), 4 (cubic interpolation), and integer */
    /* multiples of 4 in the range 8 to 1024 (sinc interpolation) */
    p->winSize = (int)(*(p->iWinSize) + FL(0.5));
    if (p->winSize < 1)
      p->winSize = 4;               /* use cubic interpolation by default */
    else if (p->winSize > 2) {
      /* cubic/sinc: round to nearest integer multiple of 4 */
      p->winSize = (p->winSize + 2) & (~3L);
      if ((uint32) p->winSize > 1024UL)
        p->winSize = 1024;
      /* constant for window calculation */
      p->winFact = (FL(1.0) - POWER(p->winSize * FL(0.85172), -FL(0.89624)))
                            / ((MYFLT)((p->winSize * p->winSize) >> 2));
    }
    /* set file parameters from header info */
    p->fileLength = (int32) sfinfo.frames;
    p->warpScale = 1.0;
    if ((int)(csound->esr + FL(0.5)) != sfinfo.samplerate) {
      if (LIKELY(p->winSize != 1)) {
        /* will automatically convert sample rate if interpolation is enabled */
        p->warpScale = (double)sfinfo.samplerate / (double)csound->esr;
      }
      else {
        csound->Warning(csound, Str("diskin2: warning: file sample rate (%d) "
                                    "!= orchestra sr (%d)\n"),
                        sfinfo.samplerate, (int)(csound->esr + FL(0.5)));
      }
    }
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (UNLIKELY(p->fileLength < 1L))
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)csound->esr * p->warpScale;
    pos *= (double)POS_FRAC_SCALE;
    p->pos_frac = (int64_t)(pos >= 0.0 ? (pos + 0.5) : (pos - 0.5));
    if (p->wrapMode) {
      p->pos_frac %= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      if (UNLIKELY(p->pos_frac < (int64_t)0))
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
    }
    p->pos_frac_inc = (int64_t)0;
    p->prv_kTranspose = FL(0.0);
    /* allocate and initialise buffers */
    p->bufSize = diskin2_calc_buffer_size(p, (int)(*(p->iBufSize) + FL(0.5)));
    n = 2 * p->bufSize * p->nChannels * (int)sizeof(MYFLT);
    if (n != (int)p->auxData.size)
      csound->AuxAlloc(csound, (int32) n, &(p->auxData));
    p->bufStartPos = p->prvBufStartPos = -((int32)p->bufSize);
    n = p->bufSize * p->nChannels;
    p->buf = (MYFLT*) (p->auxData.auxp);
    p->prvBuf = (MYFLT*) p->buf + (int)n;
/*     for (i = 0; i < n; i++) */
/*       p->buf[i] = FL(0.0); */
    memset(p->buf, 0, n*sizeof(MYFLT));
    /* done initialisation */
    p->initDone = 1;
    return OK;
}

static inline void diskin2_file_pos_inc(DISKIN2 *p, int32 *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32) (p->pos_frac >> POS_FRAC_SHIFT);
    if (p->wrapMode) {
      if (*ndx >= p->fileLength) {
        *ndx -= p->fileLength;
        p->pos_frac -= ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
      else if (*ndx < 0L) {
        *ndx += p->fileLength;
        p->pos_frac += ((int64_t)p->fileLength << POS_FRAC_SHIFT);
      }
    }
}

int diskin2_perf(CSOUND *csound, DISKIN2 *p)
{
    double  d, frac_d, x, c, v, pidwarp_d;
    MYFLT   frac, a0, a1, a2, a3, onedwarp, winFact;
    int32   ndx;
    int     i, nn, chn, wsized2, warp;

    if (UNLIKELY(p->fdch.fd == NULL) ) goto file_error;
    if(!p->initDone && !p->iSkipInit){
      return csound->PerfError(csound, Str("diskin2: not initialised"));
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * p->warpScale * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t)llrint(f);
#else
      p->pos_frac_inc = (int64_t)(f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps; nn++)
        p->aOut[chn][nn] = FL(0.0);
    /* file read position */
    ndx = (int32) (p->pos_frac >> POS_FRAC_SHIFT);
    switch (p->winSize) {
      case 1:                   /* ---- no interpolation ---- */
        for (nn = 0; nn < csound->ksmps; nn++) {
          if (p->pos_frac & ((int64_t)POS_FRAC_SCALE >> 1))
            ndx++;                      /* round to nearest sample */
          diskin2_get_sample(p, ndx, nn, FL(1.0));
          /* update file position */
          diskin2_file_pos_inc(p, &ndx);
        }
        break;
      case 2:                   /* ---- linear interpolation ---- */
        for (nn = 0; nn < csound->ksmps; nn++) {
          a1 = (MYFLT)((int)(p->pos_frac & (int64_t)POS_FRAC_MASK))
               * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
          a0 = FL(1.0) - a1;
          diskin2_get_sample(p, ndx, nn, a0);
          ndx++;
          diskin2_get_sample(p, ndx, nn, a1);
          /* update file position */
          diskin2_file_pos_inc(p, &ndx);
        }
        break;
      case 4:                   /* ---- cubic interpolation ---- */
        for (nn = 0; nn < csound->ksmps; nn++) {
          frac = (MYFLT)((int)(p->pos_frac & (int64_t)POS_FRAC_MASK))
                 * (FL(1.0) / (MYFLT)POS_FRAC_SCALE);
          a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
          a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
          a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
          a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
          ndx--;                                /* sample -1 */
          diskin2_get_sample(p, ndx, nn, a0);
          ndx++;                                /* sample 0 */
          diskin2_get_sample(p, ndx, nn, a1);
          ndx++;                                /* sample +1 */
          diskin2_get_sample(p, ndx, nn, a2);
          ndx++;                                /* sample +2 */
          diskin2_get_sample(p, ndx, nn, a3);
          /* update file position */
          diskin2_file_pos_inc(p, &ndx);
        }
        break;
      default:                  /* ---- sinc interpolation ---- */
        wsized2 = p->winSize >> 1;
        nn = POS_FRAC_SCALE + (POS_FRAC_SCALE >> 12);
        if (p->pos_frac_inc > (int64_t) nn ||
            p->pos_frac_inc < (int64_t) (-nn)) {
          warp = 1;                     /* enable warp */
          onedwarp = (p->pos_frac_inc >= (int64_t) 0 ?
                      ((MYFLT)nn / (MYFLT)p->pos_frac_inc)
                      : ((MYFLT)(-nn) / (MYFLT)p->pos_frac_inc));
          pidwarp_d = PI * (double)onedwarp;
          c = 2.0 * cos(pidwarp_d) - 2.0;
          /* correct window for kwarp */
          x = v = (double)wsized2; x *= x; x = 1.0 / x;
          v *= (double)onedwarp; v -= (double)((int)v) + 0.5; v *= 4.0 * v;
          winFact = (MYFLT)(((double)p->winFact - x) * v + x);
        }
        else {
          warp = 0;
          onedwarp = FL(0.0);
          pidwarp_d = c = 0.0;
          winFact = p->winFact;
        }
        for (nn = 0; nn < csound->ksmps; nn++) {
          frac_d = (double)((int)(p->pos_frac & (int64_t)POS_FRAC_MASK))
                   * (1.0 / (double)POS_FRAC_SCALE);
          ndx += (int32)(1 - wsized2);
          d = (double)(1 - wsized2) - frac_d;
          if (warp) {                           /* ---- warp enabled ---- */
            init_sine_gen((1.0 / PI), pidwarp_d, (pidwarp_d * d), c, &x, &v);
            /* samples -(window size / 2 - 1) to -1 */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
              diskin2_get_sample(p, ndx, nn, a1);
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
            /* sample 0 */
            /* avoid division by zero */
            if (UNLIKELY(frac_d < 0.00003)) {
              a1 = onedwarp;
            }
            else {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            }
            diskin2_get_sample(p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* sample 1 */
            /* avoid division by zero */
            if (UNLIKELY(frac_d > 0.99997)) {
              a1 = onedwarp;
            }
            else {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
            }
            diskin2_get_sample(p, ndx, nn, a1);
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* samples 2 to (window size / 2) */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT)x * a1 * a1 / (MYFLT)d;
              diskin2_get_sample(p, ndx, nn, a1);
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
          }
          else {                                /* ---- warp disabled ---- */
            /* avoid division by zero */
            if (frac_d < 0.00001 || frac_d > 0.99999) {
              ndx += (int32) (wsized2 - (frac_d < 0.5 ? 1 : 0));
              diskin2_get_sample(p, ndx, nn, FL(1.0));
            }
            else {
              a0 = (MYFLT)(sin(PI * frac_d) / PI);
              i = wsized2;
              do {
                a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = a0 * a1 * a1 / (MYFLT)d;
                diskin2_get_sample(p, ndx, nn, a1);
                d += 1.0;
                ndx++;
                a1 = (MYFLT)d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = -(a0 * a1 * a1 / (MYFLT)d);
                diskin2_get_sample(p, ndx, nn, a1);
                d += 1.0;
                ndx++;
              } while (--i);
            }
          }
          /* update file position */
          diskin2_file_pos_inc(p, &ndx);
        }
    }
    /* apply 0dBFS scale */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps; nn++)
        p->aOut[chn][nn] *= csound->e0dbfs;
    return OK;
 file_error:
    csound->ErrorMsg(csound, "diskin2: file descriptor closed or invalid\n");
   return NOTOK;
}

/* -------- soundin opcode: simplified version of diskin2 -------- */

static void soundin_read_buffer(SOUNDIN_ *p, int bufReadPos)
{
    int i = 0;

    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int_least64_t) bufReadPos;
    p->bufStartPos &= (~((int_least64_t) (p->bufSize - 1)));
    if (p->bufStartPos >= (int_least64_t) 0) {
      int_least64_t lsmps;
      int           nsmps;
      /* number of sample frames to read */
      lsmps = p->fileLength - p->bufStartPos;
      if (lsmps > (int_least64_t) 0) {  /* if there is anything to read: */
        nsmps = (lsmps < (int_least64_t) p->bufSize ? (int) lsmps : p->bufSize);
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        /* convert sample count to mono samples and read file */
        nsmps *= (int) p->nChannels;
        i = (int) sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    for ( ; i < (p->bufSize * p->nChannels); i++)
      p->buf[i] = FL(0.0);
}

/* calculate buffer size in sample frames */

static int soundin_calc_buffer_size(SOUNDIN_ *p, int n_monoSamps)
{
    int i, nFrames;

    /* default to 2048 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 2048;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* limit to sane range */
    if (i > 1048576)
      i = 1048576;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 32;       /* will be at least 64 sample frames */
    do {
      nFrames <<= 1;
    } while (nFrames < i);

    return nFrames;
}

int sndinset(CSOUND *csound, SOUNDIN_ *p)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int     n, fmt, typ;

    /* check number of channels */
    p->nChannels = (int) (p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN)) {
      return csound->InitError(csound, Str("soundin: invalid number of channels"));
    }
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (*(p->iSkipInit) != FL(0.0))
        return OK;
      fdclose(csound, &(p->fdch));
    }
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = (int) (*(p->iSampleFormat) + FL(2.5)) - 1;
    if (n == 1) {
      sfinfo.format = SF_FORMAT_RAW
                      | (int) FORMAT2SF(csound->oparms_.outformat);
    }
    else {
      if (UNLIKELY(n < 0 || n > 10))
        return csound->InitError(csound, Str("soundin: unknown sample format"));
      sfinfo.format = diskin2_format_table[n];
    }
    /* open file */
    /* FIXME: name can overflow with very long string */
    csound->strarg2name(csound, name, p->iFileCode, "soundin.", p->XSTRCODE);
    fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                                  "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound,
                               Str("soundin: %s: failed to open file"), name);
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
    /* print file information */
    if (UNLIKELY((csound->oparms_.msglevel & 7) == 7)) {
      csound->Message(csound, Str("soundin: opened '%s':\n"
                                  "         %d Hz, %d channel(s), "
                                  "%ld sample frames\n"),
                      csound->GetFileName(fd),
                      (int) sfinfo.samplerate, (int) sfinfo.channels,
                      (int32) sfinfo.frames);
    }
    /* check number of channels in file (must equal the number of outargs) */
    if (UNLIKELY(sfinfo.channels != p->nChannels)) {
      return csound->InitError(csound,
                               Str("soundin: number of output args "
                                   "inconsistent with number of file channels"));
    }
    /* skip initialisation if requested */
    if (p->auxData.auxp != NULL && *(p->iSkipInit) != FL(0.0))
      return OK;
    /* set file parameters from header info */
    p->fileLength = (int_least64_t) sfinfo.frames;
    if ((int) (csound->esr + FL(0.5)) != sfinfo.samplerate)
      csound->Warning(csound, Str("soundin: file sample rate (%d) "
                                  "!= orchestra sr (%d)\n"),
                              sfinfo.samplerate, (int) (csound->esr + FL(0.5)));
    fmt = sfinfo.format & SF_FORMAT_SUBMASK;
    typ = sfinfo.format & SF_FORMAT_TYPEMASK;
    if ((fmt != SF_FORMAT_FLOAT && fmt != SF_FORMAT_DOUBLE) ||
        (typ == SF_FORMAT_WAV || typ == SF_FORMAT_W64 || typ == SF_FORMAT_AIFF))
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);    /* do not scale "raw" float files */
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)sfinfo.samplerate;
    p->read_pos = (int_least64_t)(pos + (pos >= 0.0 ? 0.5 : -0.5));
    /* allocate and initialise buffer */
    p->bufSize = soundin_calc_buffer_size(p, (int) (*(p->iBufSize) + FL(0.5)));
    n = p->bufSize * p->nChannels;
    if (n != (int) p->auxData.size)
      csound->AuxAlloc(csound, (int32) (n * (int) sizeof(MYFLT)), &(p->auxData));
    p->buf = (MYFLT*) (p->auxData.auxp);
    /* make sure that read position is not in buffer, to force read */
    if (p->read_pos < (int_least64_t) 0)
      p->bufStartPos = (int_least64_t) p->bufSize;
    else
      p->bufStartPos = -((int_least64_t) p->bufSize);
    /* done initialisation */
    return OK;
}

int soundin(CSOUND *csound, SOUNDIN_ *p)
{
    int nn, nsmps=csound->ksmps, bufPos, i;

    if (UNLIKELY(p->fdch.fd == NULL)) {
      return csound->PerfError(csound, Str("soundin: not initialised"));
    }
    for (nn = 0; nn < nsmps; nn++) {
      bufPos = (int) (p->read_pos - p->bufStartPos);
      if ((unsigned int) bufPos >= (unsigned int) p->bufSize) {
        /* not in current buffer frame, need to read file */
        soundin_read_buffer(p, bufPos);
        /* recalculate buffer position */
        bufPos = (int) (p->read_pos - p->bufStartPos);
      }
      /* copy all channels from buffer */
      if (p->nChannels == 1) {
        p->aOut[0][nn] = p->scaleFac * p->buf[bufPos];
      }
      else if (p->nChannels == 2) {
        bufPos += bufPos;
        p->aOut[0][nn] = p->scaleFac * p->buf[bufPos];
        p->aOut[1][nn] = p->scaleFac * p->buf[bufPos + 1];
      }
      else {
        bufPos *= p->nChannels;
        i = 0;
        /* p->aOut[i++][nn] = p->scaleFac * p->buf[bufPos++]; */
        /* p->aOut[i++][nn] = p->scaleFac * p->buf[bufPos++]; */
        do {
          p->aOut[i++][nn] = p->scaleFac * p->buf[bufPos++];
        } while (i < p->nChannels);
      }
      p->read_pos++;
    }

    return OK;
}

