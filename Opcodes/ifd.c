/*  ifd.c pvsifd

    Copyright (c) Victor Lazzarini, 2005

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

/* PVSIFD

   Instantaneous Frequency Distribution analysis, plus magnitude
   and phase output.

   ffrs, fphs pvsifd ain, ifftsize, ihopsize, iwintype[,iscal]

   ffrs - AMP_FREQ signal
   fphs - AMP_PHASE signal (phase in radians, wrapped to -pi through pi)

   ain - input
   ifftsize - fftsize (must be integer multiple of hopsize)
   ihopsize - hopsize
   iwintype - O:hamming; 1: hanning,
   iscal - magnitude scaling (defaults to 1)

*/

#include "pvs_ops.h"
#include "pstream.h"

typedef struct _ifd {
  OPDS    h;
  /* outputs */
  PVSDAT *fout1, *fout2;
  /* inputs */
  MYFLT  *in, *p2, *p3, *p4, *p5, *p6, *p7;
  /* data */
  AUXCH   sigframe, diffsig, win, diffwin;
  AUXCH   counter;
  int32_t     fftsize, hopsize, wintype, frames;
  uint64_t cnt;
  double  fund, factor;
  MYFLT   norm, g;
  void  *setup;
} IFD;

/* Both opcodes use the same windows, output layout, and FFT plan. */
static int32_t ifd_setup(CSOUND *csound, IFD *p, double requested_fft,
                        double requested_hop, MYFLT window, int32_t streaming)
{
  int32_t fftsize, hopsize, frames, i;
  size_t samples, bytes;
  MYFLT *winf, *dwinf;
  double alpha, fac;
  PVSDAT *outputs[2] = {p->fout1, p->fout2};

  if (UNLIKELY(!(requested_fft >= 2 && requested_fft <= INT32_MAX-2 &&
                 requested_hop >= 1 && requested_hop <= requested_fft)))
    return csound->InitError(csound, "%s", Str("IFD: invalid FFT or hop size"));
  fftsize = (int32_t) requested_fft;
  hopsize = (int32_t) requested_hop;
  if (UNLIKELY(fftsize != requested_fft || hopsize != requested_hop ||
               (fftsize & (fftsize-1)) || fftsize % hopsize))
    return csound->InitError(csound, "%s",
                            Str("IFD: FFT size must be a power of two and "
                                "an integer multiple of the hop size"));
  if (UNLIKELY(window != PVS_WIN_HAMMING && window != PVS_WIN_HANN))
    return csound->InitError(csound, "%s", Str("IFD: unsupported window type"));
  if (UNLIKELY(window == PVS_WIN_HANN && fftsize < 4))
    return csound->InitError(csound, "%s",
                            Str("IFD: Hann window needs at least 4 samples"));

  frames = streaming ? fftsize / hopsize : 1;
  if (UNLIKELY(frames > INT32_MAX / fftsize ||
               (size_t)fftsize > SIZE_MAX / sizeof(MYFLT) / frames ||
               (size_t)fftsize+2 > SIZE_MAX / sizeof(float)))
    return csound->InitError(csound, "%s", Str("IFD: frame buffers too large"));
  p->fftsize = fftsize;
  p->hopsize = hopsize;
  p->wintype = (int32_t)window;
  p->frames = frames;
  p->cnt = hopsize;

  samples = (size_t)frames * fftsize;
  bytes = samples * sizeof(MYFLT);
  if (p->sigframe.auxp == NULL || p->sigframe.size < bytes)
    csound->AuxAlloc(csound, bytes, &p->sigframe);
  memset(p->sigframe.auxp, 0, bytes);

  bytes = (size_t)fftsize * sizeof(MYFLT);
  if (p->diffsig.auxp == NULL || p->diffsig.size < bytes)
    csound->AuxAlloc(csound, bytes, &p->diffsig);
  memset(p->diffsig.auxp, 0, bytes);
  if (p->diffwin.auxp == NULL || p->diffwin.size < bytes)
    csound->AuxAlloc(csound, bytes, &p->diffwin);
  if (p->win.auxp == NULL || p->win.size < bytes)
    csound->AuxAlloc(csound, bytes, &p->win);

  bytes = ((size_t)fftsize+2) * sizeof(float);
  for (i = 0; i < 2; i++) {
    PVSDAT *out = outputs[i];
    if (out->frame.auxp == NULL || out->frame.size < bytes)
      csound->AuxAlloc(csound, bytes, &out->frame);
    memset(out->frame.auxp, 0, bytes);
    out->N = fftsize;
    out->NB = fftsize/2+1;
    out->sliding = 0;
    out->overlap = hopsize;
    out->winsize = fftsize;
    out->wintype = p->wintype;
    out->framecount = 1;
    out->format = i == 0 ? PVS_AMP_FREQ : PVS_AMP_PHASE;
  }

  if (streaming) {
    int32_t *counter;
    bytes = (size_t)frames * sizeof(int32_t);
    if (p->counter.auxp == NULL || p->counter.size < bytes)
      csound->AuxAlloc(csound, bytes, &p->counter);
    counter = (int32_t *)p->counter.auxp;
    for (i = 0; i < frames; i++) counter[i] = i * hopsize;
  }

  winf = (MYFLT *)p->win.auxp;
  dwinf = (MYFLT *)p->diffwin.auxp;
  alpha = window == PVS_WIN_HAMMING ? 0.54 : 0.5;
  fac = TWOPI / (fftsize-1.0);
  for (i = 0; i < fftsize; i++)
    winf[i] = (MYFLT)(alpha - (1.0-alpha) * cos(fac*i));
  p->norm = 0;
  for (i = 0; i < fftsize; i++) {
    dwinf[i] = winf[i] - (i+1 < fftsize ? winf[i+1] : FL(0.0));
    p->norm += winf[i];
  }

  p->factor = CS_ESR / TWOPI_F;
  p->fund = CS_ESR / fftsize;
  p->setup = csound->RealFFTSetup(csound, fftsize, FFT_FWD);
  return OK;
}

static int32_t ifd_init(CSOUND *csound, IFD *p)
{
  p->g = *p->p5;
  return ifd_setup(csound, p, *p->p2, *p->p3, *p->p4, 1);
}

static void IFAnalysis(CSOUND * csound, IFD * p, MYFLT * signal)
{

  double  powerspec, da, db, a, b, ph, factor = p->factor, fund = p->fund;
  MYFLT   scl = p->g / p->norm;
  int32_t     i2, i, fftsize = p->fftsize, hsize = p->fftsize / 2;
  MYFLT   tmp1, tmp2;
  MYFLT *diffwin = (MYFLT *) p->diffwin.auxp;
  MYFLT  *win = (MYFLT *) p->win.auxp;
  MYFLT  *diffsig = (MYFLT *) p->diffsig.auxp;
  float  *output = (float *) p->fout1->frame.auxp;
  float  *outphases = (float *) p->fout2->frame.auxp;

  for (i = 0; i < fftsize; i++) {
    diffsig[i] = signal[i] * diffwin[i];
    signal[i] = signal[i] * win[i];
  }

  for (i = 0; i < hsize; i++) {
    tmp1 = diffsig[i + hsize];
    tmp2 = diffsig[i];
    diffsig[i] = tmp1;
    diffsig[i + hsize] = tmp2;

    tmp1 = signal[i + hsize];
    tmp2 = signal[i];
    signal[i] = tmp1;
    signal[i + hsize] = tmp2;
    }

  csound->RealFFT(csound, p->setup, signal);
  csound->RealFFT(csound, p->setup,diffsig);

  for (i = 2; i < fftsize; i += 2) {

    i2 = i / 2;
    a = signal[i] * scl;
    b = signal[i + 1] * scl;
    da = diffsig[i] * scl;
    db = diffsig[i + 1] * scl;
    powerspec = a * a + b * b;

    if ((outphases[i] = output[i] = (float) sqrt(powerspec)) != 0.0f) {
      output[i + 1] = ((a * db - b * da) / powerspec) * factor + i2 * fund;
      ph = (float) ATAN2(b, a);
      /*double d = ph - outphases[i + 1];
        while (d > PI)
        d -= TWOPI;
        while (d < -PI)
        d += TWOPI; */
      outphases[i + 1] = (float)ph;
    }
    else {
      output[i + 1] = i2 * fund;
      outphases[i + 1] = 0.0f;
    }
  }
  /* DC and Nyquist are real coefficients: their sign belongs in the phase. */
  tmp1 = signal[0] * scl;
  tmp2 = signal[1] * scl;
  output[0] = outphases[0] = (float)FABS(tmp1);
  output[fftsize] = outphases[fftsize] = (float)FABS(tmp2);
  outphases[1] = tmp1 < FL(0.0) ? (float)PI : 0.0f;
  outphases[fftsize + 1] = tmp2 < FL(0.0) ? (float)PI : 0.0f;
  output[1] = 0.0f;
  output[fftsize + 1] = CS_ESR * FL(0.5);
  p->fout1->framecount++;
  p->fout2->framecount++;
}

static int32_t ifd_process(CSOUND * csound, IFD * p)
{
  int32_t     i;
  MYFLT  *sigin = p->in;
  MYFLT  *sigframe = (MYFLT *) p->sigframe.auxp;
  int32_t     fftsize = p->fftsize;
  int32_t *counter = (int32_t *) p->counter.auxp;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32_t     frames = p->frames;
  //int32_t     cnt = p->cnt;

  if (UNLIKELY(early)) nsmps -= early;
  for (n = offset; n < nsmps; n++) {
    for (i = 0; i < frames; i++) {
      sigframe[i * fftsize + counter[i]] = sigin[n];
      counter[i]++;
      if (counter[i] == fftsize) {
        //if (cnt < frames)
        // cnt++;
        //else
        IFAnalysis(csound, p, &sigframe[i * fftsize]);
        counter[i] = 0;
      }
    }
  }
  //p->cnt = cnt;

  return OK;
}

static int32_t tifd_init(CSOUND *csound, IFD *p)
{
  return ifd_setup(csound, p, *p->p4, *p->p5, *p->p6, 0);
}


static int32_t tifd_process(CSOUND * csound, IFD * p)
{
  int32_t     hopsize = p->hopsize;
  uint32_t nsmps = CS_KSMPS;

  if(p->cnt >= hopsize){
    MYFLT  pos = *p->in*CS_ESR;
    MYFLT  *sigframe = (MYFLT *) p->sigframe.auxp;
    MYFLT  pit = *p->p3;
    int32_t     fftsize = p->fftsize;
    uint32_t post;
    MYFLT frac;
    FUNC *ft = csound->FTFind(csound,p->p7);
    if (UNLIKELY(ft == NULL)) {
      return csound->PerfError(csound, &(p->h),
                               "could not find table number %d\n", (int32_t) *p->p7);
    }
    MYFLT *tab = ft->ftable;
    int32_t i,size = ft->flen;
    for(i=0; i < fftsize; i++){
      MYFLT in;
      /* Wrap before splitting the index and fraction. Wrap negatives first
         because adding size to a tiny negative can round up to size. */
      while (pos < 0) pos += size;
      while (pos >= size) pos -= size;
      post = (uint32_t) pos;
      frac = pos - post;
      in = tab[post] + frac*(tab[post+1] - tab[post]);
      sigframe[i] = in;
      pos += pit;
    }
    p->g = *p->p2;
    IFAnalysis(csound, p, sigframe);
    p->cnt -= hopsize;
  }
  p->cnt += nsmps;
  return OK;
}


static OENTRY localops[] =
  {
   { "pvsifd", sizeof(IFD), 0,  "ff", "aiiip",
     (SUBR) ifd_init, (SUBR) ifd_process},
   { "tabifd", sizeof(IFD), 0,  "ff", "kkkiiii",
     (SUBR) tifd_init, (SUBR) tifd_process}
  };

int32_t ifd_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t
                                ) (sizeof(localops) / sizeof(OENTRY)));
}
