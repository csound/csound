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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
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
    if (i >= 0 && i < DISKIN2_BUFSIZE) {
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
    p->bufStartPos &= (~((long) (DISKIN2_BUFSIZE - 1)));
    if (p->bufStartPos < 0L) {
      /* before beginning of file ? just fill buffer with zero samples */
      for (i = 0; i < DISKIN2_BUFSIZE; i++)
        p->buf[i] = FL(0.0);
    }
    else {
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0L) {         /* if there is anything to read: */
        if (nsmps > (long) DISKIN2_BUFSIZE)
          nsmps = (long) DISKIN2_BUFSIZE;
        sf_seek(p->sf,
               (sf_count_t) (p->bufStartPos / (long) p->fileChannels),
               SEEK_SET);
        i = (int) sf_read_MYFLT(p->sf, p->buf, (sf_count_t) nsmps);
        if (i < 0)
          i = 0;
      }
      else
        i = 0;
      /* at end of file: fill rest of buffer with zero samples */
      while (i < DISKIN2_BUFSIZE)
        p->buf[i++] = FL(0.0);
    }
}

#define diskin2_get_sample(p,pos,bufp)                          \
{                                                               \
    long filePos;                                               \
    int  i, bufPos, j;                                          \
                                                                \
    filePos = (long) pos * (long) p->fileChannels;              \
    i = 0;                                                      \
    bufPos = (int) (filePos - p->bufStartPos);                  \
    do {                                                        \
      j = bufPos + p->channelMap[i];                            \
      if (j < 0 || j >= DISKIN2_BUFSIZE) {                      \
        /* not in current buffer frame, need to read file */    \
        diskin2_read_buffer(p, j);                              \
        /* recalculate buffer position */                       \
        bufPos = (int) (filePos - p->bufStartPos);              \
        j = bufPos + p->channelMap[i];                          \
      }                                                         \
      (bufp)[i] = p->buf[j];                                    \
    } while (++i < p->nChannels);                               \
}

/* ------------- set up fast sine generator ------------- */
/* Input args:                                            */
/*   a: amplitude                                         */
/*   f: frequency (-PI - PI)                              */
/*   p: initial phase (0 - PI/2)                          */
/* Output args:                                           */
/*  *x: first output sample                               */
/*  *c, *v: coefficients for calculating next sample as   */
/*          shown below:                                  */
/*            v = v + c * x                               */
/*            x = x + v                                   */
/*          These values are calculated by:               */
/*            x = y[0]                                    */
/*            c = 2.0 * cos(f) - 2.0                      */
/*            v = y[1] - (c + 1.0) * y[0]                 */
/*          where y[0], and y[1] are the first, and       */
/*          second sample of the sine wave to be          */
/*          generated, respectively.                      */
/* -------- written by Istvan Varga, Jan 28 2002 -------- */

static void init_sine_gen(double a, double f, double p,
                           double *x, double *c, double *v)
{
    double  y0, y1;                 /* these should be doubles */

    y0 = sin(p);
    y1 = sin(p + f);
    *x = y0;
    *c = 2.0 * cos(f) - 2.0;
    *v = y1 - *c * y0 - y0;
    /* amp. scale */
    *x *= a; *v *= a;
}

int diskin2_init(ENVIRON *csound, DISKIN2 *p)
{
    double  pos;
    char    name[256];
    SF_INFO sfinfo;
    int     i, n, fd;

    /* skip initialisation if requested */
    if (*(p->iSkipInit) != FL(0.0))
      return OK;
    p->nChannels = (int) (p->OUTOCOUNT);
    if (p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN) {
      initerror(Str("diskin2: invalid number of channels"));
      return NOTOK;
    }
    p->fileChannels = p->nChannels;     /* default for raw file */
    n = DISKIN2_MAXCHN;
    while (n && (*(p->iChannelMap[n - 1]) == FL(0.0)))
      n--;
    if (n <= 0) {
      /* no channel map specification */
      for (i = 0; i < p->nChannels; i++)
        p->channelMap[i] = i;
    }
    else {
      if (n != p->nChannels) {
        initerror(Str("diskin2: channel map inconsistent with number of "
                      "output args"));
        return NOTOK;
      }
      for (i = 0; i < p->nChannels; i++)        /* will check later */
        p->channelMap[i] = (int) (*(p->iChannelMap[i]) - FL(0.5));
    }
    /* interpolation window size: valid settings are 1 (no interpolation), */
    /* 2 (linear interpolation), 4 (cubic interpolation), and integer */
    /* multiples of 4 in the range 8 to 1024 (sinc interpolation) */
    p->winSize = (int) (*(p->iWinSize) + FL(0.5));
    if (p->winSize < 1)
      p->winSize = 1;
    else if (p->winSize > 2) {
      p->winSize = (p->winSize + 2) & (~3);
      if (p->winSize > 1024)
        p->winSize = 1024;
    }
    /* open file */
    if (*(p->iFileCode) == SSTRCOD && p->STRARG != NULL)
      fd = openin(unquote(p->STRARG));
    else {
      sprintf(name, "soundin.%d", (int) (*(p->iFileCode) + FL(0.5)));
      fd = openin(name);
    }
    if (fd < 0)
      return NOTOK;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    p->sf = sf_open_fd(fd, SFM_READ, &sfinfo, SF_TRUE);
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
        default:
          initerror(Str("diskin2: unknown sample format"));
          return NOTOK;
      }
      /* re-open as raw file */
      p->sf = sf_open_fd(fd, SFM_READ, &sfinfo, SF_TRUE);
    }
    if (p->sf == NULL) {
      initerror(Str("diskin2: error opening sound file"));
      close(fd);
      return NOTOK;
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = (void*) p->sf;
    fdrecord(&(p->fdch));
    /* check number of channels in file (must be power of two) */
    if (sfinfo.channels <= 0 ||
        (sfinfo.channels & (sfinfo.channels - 1)) != 0) {
      initerror(Str("diskin2: invalid number of channels in file"));
      return NOTOK;
    }
    /* set file parameters from header info */
    p->fileChannels = sfinfo.channels;
    p->fileLength = (long) sfinfo.frames * (long) sfinfo.channels;
    if ((int) (csound->esr_ + FL(0.5)) != sfinfo.samplerate) {
      if (p->winSize == 1) {
        csoundMessage(csound, Str("diskin2: warning: file sample rate (%d) "
                                  "!= orchestra sr (%d)\n"), sfinfo.samplerate,
                              (int) (csound->esr_ + FL(0.5)));
        p->warpScale = FL(1.0);
      }
      else    /* will automatically convert sample rate if interpolation is  */
        p->warpScale = (MYFLT) sfinfo.samplerate / csound->esr_;  /* enabled */
    }
    else
      p->warpScale = FL(1.0);
    /* check for a valid channel map */
    for (i = 0; i < p->nChannels; i++) {
      if (p->channelMap[i] < 0 || p->channelMap[i] >= p->fileChannels) {
        initerror(Str("diskin2: invalid channel map"));
        return NOTOK;
      }
    }
    /* initialise read position */
    pos = (double) *(p->iSkipTime) * (double) csound->esr_;
    pos *= (double) p->warpScale;
    p->pos_int = (long) (pos >= 0.0 ? pos : pos - 1.0);
    p->pos_frac = (int) (((pos - (double) p->pos_int) * (double) POS_FRAC_SCALE)
                         + 0.5);
    p->pos_int += ((long) p->pos_frac >> POS_FRAC_SHIFT);   /* safety */
    p->pos_frac &= POS_FRAC_MASK;
    p->pos_frac_inc = 0;
    p->prv_kTranspose = FL(0.0);
    /* constant for window calculation */
    p->winFact = (MYFLT) ((1.0 - pow((double) p->winSize * 0.85172, -0.89624))
                          / ((double) (p->winSize * p->winSize) * 0.25));
    /* initialise buffers */
    p->bufStartPos = p->prvBufStartPos = -((long) DISKIN2_BUFSIZE);
    p->buf = &(p->buf_1[0]);
    p->prvBuf = &(p->buf_2[0]);
    for (i = 0; i < DISKIN2_BUFSIZE; i++) {
      p->buf_1[i] = FL(0.0);
      p->buf_2[i] = FL(0.0);
    }
    /* done initialisation */
    p->initDone = 1;
    return OK;
}

int diskin2_perf(ENVIRON *csound, DISKIN2 *p)
{
    MYFLT   tmpBuf[DISKIN2_MAXCHN];
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
      p->pos_frac_inc = (int) ((double) p->prv_kTranspose
                               * (double) p->warpScale
                               * (double) POS_FRAC_SCALE + 0.5);
    }
    switch (p->winSize) {
      case 1:                   /* ---- no interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          ndx = p->pos_int;
          if (p->pos_frac >= (POS_FRAC_SCALE >> 1))
            ndx++;      /* round to nearest sample */
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] = tmpBuf[chn];
          p->pos_frac += p->pos_frac_inc;
          p->pos_int += ((long) p->pos_frac >> POS_FRAC_SHIFT);
          p->pos_frac &= POS_FRAC_MASK;
        }
        break;
      case 2:                   /* ---- linear interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          frac = (MYFLT) p->pos_frac * (FL(1.0) / (MYFLT) POS_FRAC_SCALE);
          a0 = FL(1.0) - frac;
          a1 = frac;
          ndx = p->pos_int;
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          ndx++;
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] = tmpBuf[chn] * a0;
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] += tmpBuf[chn] * a1;
          p->pos_frac += p->pos_frac_inc;
          p->pos_int += ((long) p->pos_frac >> POS_FRAC_SHIFT);
          p->pos_frac &= POS_FRAC_MASK;
        }
        break;
      case 4:                   /* ---- cubic interpolation ---- */
        for (nn = 0; nn < csound->ksmps_; nn++) {
          frac = (MYFLT) p->pos_frac * (FL(1.0) / (MYFLT) POS_FRAC_SCALE);
          a3 = frac * frac; a3 -= FL(1.0); a3 *= (FL(1.0) / FL(6.0));
          a2 = frac; a2 += FL(1.0); a0 = (a2 *= FL(0.5)); a0 -= FL(1.0);
          a1 = FL(3.0) * a3; a2 -= a1; a0 -= a3; a1 -= frac;
          a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += FL(1.0);
          ndx = p->pos_int - 1L;
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          ndx++;
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] = tmpBuf[chn] * a0;        /* sample -1 */
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          ndx++;
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] += tmpBuf[chn] * a1;       /* sample 0 */
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          ndx++;
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] += tmpBuf[chn] * a2;       /* sample +1 */
          diskin2_get_sample(p, ndx, &(tmpBuf[0]));
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] += tmpBuf[chn] * a3;       /* sample +2 */
          p->pos_frac += p->pos_frac_inc;
          p->pos_int += ((long) p->pos_frac >> POS_FRAC_SHIFT);
          p->pos_frac &= POS_FRAC_MASK;
        }
        break;
      default:                  /* ---- sinc interpolation ---- */
        wsized2 = p->winSize >> 1;
        if (abs(p->pos_frac_inc) > (POS_FRAC_SCALE + (POS_FRAC_SCALE >> 10))) {
          warp = 1;                     /* enable warp */
          onedwarp = (MYFLT) POS_FRAC_SCALE / (MYFLT) (abs(p->pos_frac_inc));
          pidwarp_d = PI * (double) onedwarp;
          /* correct window for kwarp */
          x = v = (double) wsized2; x *= x; x = 1.0 / x;
          v *= (double) onedwarp; v -= (double)((int) v) + 0.5; v *= 4.0 * v;
          winFact = (MYFLT) (((double) p->winFact - x) * v + x);
        }
        else {
          warp = 0;
          onedwarp = FL(1.0);
          pidwarp_d = PI;
          winFact = p->winFact;
        }
        for (nn = 0; nn < csound->ksmps_; nn++) {
          frac = (MYFLT) p->pos_frac * (FL(1.0) / (MYFLT) POS_FRAC_SCALE);
          /* clear output */
          for (chn = 0; chn < p->nChannels; chn++)
            p->aOut[chn][nn] = FL(0.0);
          frac_d = (double) frac;
          ndx = p->pos_int + (long) (1 - wsized2);
          d = (double) (1 - wsized2) - frac_d;
          if (warp) {                           /* ---- warp enabled ---- */
            init_sine_gen((1.0 / PI), pidwarp_d, pidwarp_d * d, &x, &c, &v);
            /* samples -(window size / 2 - 1) to -1 */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
            /* sample 0 */
            /* avoid division by zero */
            if (frac_d < 0.00003) {
              a1 = onedwarp;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
            }
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* sample 1 */
            /* avoid division by zero */
            if (frac_d > 0.99997) {
              a1 = onedwarp;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
            }
            else {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
            }
            ndx++;
            d += 1.0; v += c * x; x += v;
            /* samples 2 to (window size / 2) */
            i = wsized2 - 1;
            do {
              a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
              a1 = (MYFLT) x * a1 * a1 / (MYFLT) d;
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] += tmpBuf[chn] * a1;
              ndx++;
              d += 1.0; v += c * x; x += v;
            } while (--i);
          }
          else {                                /* ---- warp disabled ---- */
            /* avoid division by zero */
            if (frac_d < 0.00001 || frac_d > 0.99999) {
              ndx += (long) (wsized2 - (frac_d < 0.5 ? 1 : 0));
              diskin2_get_sample(p, ndx, &(tmpBuf[0]));
              for (chn = 0; chn < p->nChannels; chn++)
                p->aOut[chn][nn] = tmpBuf[chn];
            }
            else {
              a0 = (MYFLT) (sin(PI * frac_d) / PI);
              i = wsized2;
              do {
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = a0 * a1 * a1 / (MYFLT) d;
                diskin2_get_sample(p, ndx, &(tmpBuf[0]));
                for (chn = 0; chn < p->nChannels; chn++)
                  p->aOut[chn][nn] += tmpBuf[chn] * a1;
                d += 1.0;
                ndx++;
                a1 = (MYFLT) d; a1 = FL(1.0) - a1 * a1 * winFact;
                a1 = a0 * a1 * a1 / (MYFLT) d;
                diskin2_get_sample(p, ndx, &(tmpBuf[0]));
                for (chn = 0; chn < p->nChannels; chn++)
                  p->aOut[chn][nn] -= tmpBuf[chn] * a1;
                d += 1.0;
                ndx++;
              } while (--i);
            }
          }
          p->pos_frac += p->pos_frac_inc;
          p->pos_int += ((long) p->pos_frac >> POS_FRAC_SHIFT);
          p->pos_frac &= POS_FRAC_MASK;
        }
    }
    /* apply 0dBFS scale */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps_; nn++)
        p->aOut[chn][nn] *= csound->e0dbfs_;
    return OK;
}

