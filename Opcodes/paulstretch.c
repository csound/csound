/*
    paulstretch.c:

    This is an implementation of the paulstretch algorithm by
    Paul Nasca Octavian, based off of the Numpy/Scipy python
    implementation written by the same author.

    Copyright (C) 2016 by Paul Batchelor

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
#include <stdlib.h>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

typedef struct {
    OPDS h;
    MYFLT *out, *stretch, *winsize, *ifn;
    double start_pos, displace_pos;
    MYFLT *window;
    MYFLT *old_windowed_buf;
    MYFLT *hinv_buf;
    MYFLT *output;
    FUNC *ft;
    uint32_t windowsize;
    uint32_t half_windowsize;
    MYFLT *tmp;
    uint32_t counter;
    AUXCH m_window;
    AUXCH m_old_windowed_buf;
    AUXCH m_hinv_buf;
    AUXCH m_output;
    AUXCH m_tmp;
    void *setup, *isetup;
} PAULSTRETCH;

static void compute_block(CSOUND *csound, PAULSTRETCH *p)
{
    uint32_t istart_pos = (uint32_t)p->start_pos;
    uint32_t pos;
    uint32_t i;
    uint32_t windowsize = p->windowsize;
    uint32_t half_windowsize = p->half_windowsize;
    MYFLT *hinv_buf = p->hinv_buf;
    MYFLT *old_windowed_buf= p->old_windowed_buf;
    MYFLT *tbl = p->ft->ftable;
    MYFLT *window = p->window;
    MYFLT *output= p->output;
    MYFLT *tmp = p->tmp;
    for (i = 0; i < windowsize; i++) {
      pos = istart_pos + i;
      if (LIKELY(pos < p->ft->flen)) {
        tmp[i] = tbl[pos] * window[i];
      } else {
        tmp[i] = FL(0.0);
      }
    }
    csound->RealFFT(csound, p->setup, tmp);
    /* Unpack Nyquist after the FFT; DC has no imaginary component. */
    tmp[p->windowsize] = tmp[1];
    tmp[1] = FL(0.0);
    tmp[p->windowsize + 1] = FL(0.0);
    /* randomize phase */
    for (i = 0; i < windowsize + 2; i += 2) {
      MYFLT mag = HYPOT(tmp[i], tmp[i + 1]);
      MYFLT  x = (((MYFLT)rand() / RAND_MAX) * 2 * PI);
      tmp[i] = mag * COS(x);
      tmp[i + 1] = mag * SIN(x);
    }

    /* re-order bins and take inverse FFT */
    tmp[1] = tmp[p->windowsize];
    csound->RealFFT(csound, p->isetup, tmp);
    /* apply window and overlap */
    for (i = 0; i < windowsize; i++) {
      tmp[i] *= window[i];
      if (i < half_windowsize) {
        output[i] = (MYFLT)(tmp[i] + old_windowed_buf[half_windowsize + i]);
        output[i] *= hinv_buf[i];
      }
      old_windowed_buf[i] = tmp[i];
    }
    /* Keep emitting the overlap tail, then silence, after the source ends. */
    if (p->displace_pos >= (double)p->ft->flen - p->start_pos)
      p->start_pos = p->ft->flen;
    else
      p->start_pos += p->displace_pos;
}

static int32_t ps_init(CSOUND* csound, PAULSTRETCH *p)
{
    FUNC *ftp = csound->FTFind(csound, p->ifn);
    uint32_t i = 0;
    size_t size;
    double samples = (double)CS_ESR * (double)*p->winsize;
    double stretch = (double)*p->stretch;

    if (ftp == NULL)
      return csound->InitError(csound, "%s", Str("paulstretch: table not found"));

    if (UNLIKELY(!(stretch > 0.0) || !isfinite(stretch)))
      return csound->InitError(csound, "%s",
                               Str("paulstretch: stretch must be finite and positive"));
    /* RealFFT uses signed byte counts internally and needs two spare samples. */
    if (UNLIKELY(!(samples >= 0.0 &&
                   samples < (double)(INT32_MAX / sizeof(MYFLT) - 2))))
      return csound->InitError(csound, "%s",
                               Str("paulstretch: invalid window size"));
    p->ft = ftp;
    p->windowsize = (uint32_t)samples;
    if (p->windowsize < 16) {
      p->windowsize = 16;
    }
    /* Real FFT bins and the two equal overlap halves require an even size. */
    p->windowsize &= ~1u;
    p->half_windowsize = p->windowsize / 2;
    p->displace_pos = (double)p->half_windowsize / stretch;

    size = sizeof(MYFLT) * p->windowsize;
    csound->AuxAlloc(csound, size, &(p->m_window));
    p->window = p->m_window.auxp;

    csound->AuxAlloc(csound, size, &p->m_old_windowed_buf);
    p->old_windowed_buf = p->m_old_windowed_buf.auxp;

    csound->AuxAlloc(csound,
                     (size_t)(sizeof(MYFLT) * p->half_windowsize), &p->m_hinv_buf);
    p->hinv_buf = p->m_hinv_buf.auxp;

    csound->AuxAlloc(csound,
                     (size_t)(sizeof(MYFLT) * p->half_windowsize), &p->m_output);
    p->output = p->m_output.auxp;

    csound->AuxAlloc(csound, size + 2 * sizeof(MYFLT), &p->m_tmp);
    p->tmp = p->m_tmp.auxp;

    /* Create Hann window */
    for (i = 0; i < p->windowsize; i++) {
      p->window[i] = FL(0.5) - COS(i * TWOPI_F / (p->windowsize - 1)) * FL(0.5);
    }
    /* create inverse Hann window */
    {
      MYFLT hinv_sqrt2 = (1 + SQRT(FL(0.5))) * FL(0.5);
      for (i = 0; i < p->half_windowsize; i++) {
        p->hinv_buf[i] =
          hinv_sqrt2 - (FL(1.0) - hinv_sqrt2) *
          COS(i * TWOPI_F / p->half_windowsize);
      }
    }
    p->start_pos = FL(0.0);
    p->counter = 0;
    p->setup = csound->RealFFTSetup(csound, p->windowsize, FFT_FWD);
    p->isetup = csound->RealFFTSetup(csound, p->windowsize, FFT_INV);
    return OK;
}

static int32_t paulstretch_perf(CSOUND* csound, PAULSTRETCH *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n = offset; n < nsmps; n++) {
      if (p->counter == 0) {
        compute_block(csound, p);
      }
      out[n] = p->output[p->counter];
      p->counter = (p->counter + 1) % p->half_windowsize;
    }
    return OK;
}

static OENTRY paulstretch_localops[] = {
  { "paulstretch", (int32_t) sizeof(PAULSTRETCH), TR,  "a", "iii",
    (int32_t (*)(CSOUND *, void *)) ps_init,
    (int32_t (*)(CSOUND *, void *)) paulstretch_perf}
};

LINKAGE_BUILTIN(paulstretch_localops)
