/*
  arrays.c: various array operations

  Copyright (C) 2013-2025 Victor Lazzarini
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

#include "interlocks.h"
#include "arrays.h"
#include "array_ops.h"  // for ARRAYINIT and array_init

/*
  transform operations
*/

typedef struct _autocorr {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in;
  AUXCH mem;
  int32_t N;
  int32_t FN;
} AUTOCORR;

int32_t init_autocorr(CSOUND *csound, AUTOCORR *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t N = p->in->sizes[0], fn;
  for(fn=2; fn < N*2-1; fn*=2);
  if (p->mem.auxp == 0 || p->mem.size < fn*sizeof(MYFLT))
    csound->AuxAlloc(csound, fn*sizeof(MYFLT), &p->mem);
  p->N = N;
  p->FN = fn;
  tabinit(csound, p->out,N, p->h.insdshead);
  return OK;
}

int32_t perf_autocorr(CSOUND *csound, AUTOCORR *p) {
  MYFLT *r = p->out->data;
  MYFLT *buf = (MYFLT *) p->mem.auxp;
  MYFLT *s = p->in->data;
  csound->AutoCorrelation(csound,r,s,p->N,buf,p->FN);
  return OK;
}

typedef struct _fft {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in, *in2;
  MYFLT *f;
  MYFLT b;
  int32_t n;
  void *setup;
  AUXCH mem;
} FFT;

static int32_t init_fft_complex(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("fft: only one-dimensional arrays allowed"));
  /* Sanity checks: prevent pathological sizes and integer overflows */
  if (UNLIKELY(N <= 0))
    return csound->InitError(csound, "%s", Str("fft: input array size must be > 0"));
  /* Cap to a conservative upper bound to avoid overflow in allocations */
  const int32_t MAX_REASONABLE_N = (1 << 24); /* ~16M elements */
  if (UNLIKELY(N > MAX_REASONABLE_N)) {
    /* Try to repair from backing allocation metadata if sizes[] is corrupted */
    if (p->in->allocated > 0 && p->in->arrayMemberSize > 0) {
      int64_t n2 = (int64_t)(p->in->allocated / (size_t)p->in->arrayMemberSize);
      if (n2 > 0 && n2 <= MAX_REASONABLE_N) {
        N = (int32_t)n2;
        if (p->in->sizes && p->in->dimensions == 1)
          p->in->sizes[0] = N;
      } else {
        return csound->InitError(csound, "fft: input array size (%d) is unreasonable", N);
      }
    } else {
      return csound->InitError(csound, "fft: input array size (%d) is unreasonable", N);
    }
  }

  tabinit(csound, p->out, N, p->h.insdshead);
  size_t bytes = (size_t)N * 2u * sizeof(MYFLT);
  csound->AuxAlloc(csound, bytes, &p->mem);
  if(p->out->arrayType == csound->GetType(csound, "Complex")
     && p->in->arrayType == csound->GetType(csound, "Complex"))
    p->b = *((MYFLT *)p->in2);
  else if(p->out->arrayType == csound->GetType(csound, "Complex"))
    p->b = 0;
  else p->b = 1;
  return OK;
}

static int32_t perf_fft_complex(CSOUND *csound, FFT *p) {
  int32_t N = p->in->sizes[0];
  MYFLT *tmp = (MYFLT *)p->mem.auxp;
  COMPLEXDAT *c = (COMPLEXDAT *) p->in->data;
  if(p->in->arrayType == csound->GetType(csound, "Complex")) {
    for(int32_t i = 0, j = 0; j < N; i+=2, j++) {
      tmp[i] = c[j].real;
      tmp[i+1] = c[j].imag;
    }
  } else {
    MYFLT *re = p->in->data;
    for(int32_t i = 0, j = 0; j < N; i+=2, j++) {
      tmp[i] = re[j];
      tmp[i+1] = 0;
    }
  }
  if(!p->b)
    csound->ComplexFFT(csound,tmp,N);
  else csound->InverseComplexFFT(csound,tmp,N);
  if(p->out->arrayType == csound->GetType(csound, "Complex")) {
    c = (COMPLEXDAT *) p->out->data;
    for(int32_t i = 0, j = 0; j < N; i+=2, j++) {
      c[j].real = tmp[i];
      c[j].imag = tmp[i+1];
    }
  } else {
    MYFLT *re = p->out->data;
    for(int32_t i = 0, j = 0; j < N; i+=2, j++) {
      re[j] = tmp[i];
    }
  }
  return OK;
}

static int32_t init_rfft_r2c(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rfft: only one-dimensional arrays allowed"));
  tabinit(csound, p->out,N+1, p->h.insdshead);
  p->setup = csound->RealFFTSetup(csound, N, FFT_FWD);
  csound->AuxAlloc(csound, sizeof(MYFLT)*N, &p->mem);
  return OK;
}
// NB: outputs are NOT packed (N+1 size)
static int32_t perf_rfft_r2c(CSOUND *csound, FFT *p) {
  int32_t N = p->out->sizes[0]-1;
  MYFLT *tmp = (MYFLT *)p->mem.auxp;
  COMPLEXDAT *c = (COMPLEXDAT *) p->out->data;
  memcpy(tmp,p->in->data,N*sizeof(MYFLT));
  csound->RealFFT(csound,p->setup,tmp);
  for(int32_t i = 0, j = 0; i < N; i+=2, j++) {
    c[j].real = tmp[i];
    if(j) c[j].imag = tmp[i+1];
    else c[j].imag = 0.;
  }
  c[N].real = tmp[1];
  c[N].imag =  0.;
  return OK;
}

static int32_t init_rfft_c2r(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0] - 1;
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rfft: only one-dimensional arrays allowed"));
  tabinit(csound, p->out,N, p->h.insdshead);
  p->setup = csound->RealFFTSetup(csound, N, FFT_INV);
  csound->AuxAlloc(csound, sizeof(MYFLT)*N, &p->mem);
  return OK;
}

// NB: these expect NOT packed (N+1 size) inputs
static int32_t perf_rfft_c2r(CSOUND *csound, FFT *p) {
  int32_t N = p->out->sizes[0];
  MYFLT *tmp = (MYFLT *)p->mem.auxp;
  COMPLEXDAT *c = (COMPLEXDAT *) p->in->data;
  for(int32_t i = 0, j = 0; i < N; i+=2, j++) {
    tmp[i] = c[j].real;
    if(j) tmp[i+1] = c[j].imag;
  }
  tmp[1] = c[N].real;
  csound->RealFFT(csound,p->setup,tmp);
  memcpy(p->out->data,tmp,N*sizeof(MYFLT));
  return OK;
}

static int32_t init_rfft(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rfft: only one-dimensional arrays allowed"));
  tabinit(csound, p->out,N, p->h.insdshead);
  p->setup = csound->RealFFTSetup(csound, N, FFT_FWD);
  return OK;
}

static  int32_t perf_rfft(CSOUND *csound, FFT *p) {
  int32_t N = p->out->sizes[0];
  memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
  csound->RealFFT(csound,p->setup,p->out->data);
  return OK;
}

static  int32_t rfft_i(CSOUND *csound, FFT *p) {
  if (init_rfft(csound,p) == OK)
    return perf_rfft(csound, p);
  else return NOTOK;
}

static int32_t init_rifft(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rifft: only one-dimensional arrays allowed"));
  p->setup = csound->RealFFTSetup(csound, N, FFT_INV);
  tabinit(csound, p->out, N, p->h.insdshead);
  return OK;
}

static int32_t perf_rifft(CSOUND *csound, FFT *p) {
  int32_t N = p->in->sizes[0];
  memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
  csound->RealFFT(csound,p->setup,p->out->data);
  return OK;
}

static int32_t rifft_i(CSOUND *csound, FFT *p) {
  if (init_rifft(csound,p) == OK)
    return perf_rifft(csound, p);
  else return NOTOK;
}

static int32_t init_rfftmult(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if(p->in2->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(N != p->in2->sizes[0]))
    return csound->InitError(csound, "%s", Str("array sizes do not match\n"));
  tabinit(csound, p->out, N, p->h.insdshead);
  return OK;
}

static int32_t perf_rfftmult(CSOUND *csound, FFT *p) {

  int32_t N = p->out->sizes[0];
  csound->RealFFTMult(csound,p->out->data,p->in->data,p->in2->data,N,1);
  return OK;
}

static int32_t initialise_fft(CSOUND *csound, FFT *p) {
  int32_t   N2 = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("fft: only one-dimensional arrays allowed"));
  tabinit(csound,p->out,N2, p->h.insdshead);
  return OK;
}

static int32_t perf_fft(CSOUND *csound, FFT *p) {
  int32_t N2 = p->in->sizes[0];
  memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
  csound->ComplexFFT(csound,p->out->data,N2/2);
  return OK;
}

static int32_t fft_i(CSOUND *csound, FFT *p) {
  if (initialise_fft(csound,p) == OK)
    return perf_fft(csound, p);
  else return NOTOK;
}


static int32_t init_ifft(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N2 = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("fftinv: only one-dimensional arrays allowed"));
  tabinit(csound, p->out, N2, p->h.insdshead);
  return OK;
}

static int32_t perf_ifft(CSOUND *csound, FFT *p) {
  int32_t N2 = p->out->sizes[0];
  memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
  csound->InverseComplexFFT(csound,p->out->data,N2/2);
  return OK;
}

static int32_t ifft_i(CSOUND *csound, FFT *p) {
  if (LIKELY(init_ifft(csound,p) == OK))
    return perf_ifft(csound, p);
  else return NOTOK;
}

static int32_t init_recttopol(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  tabinit(csound, p->out, N, p->h.insdshead);
  return OK;
}

static int32_t perf_recttopol(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i, end = p->out->sizes[0];
  MYFLT *in, *out, mag, ph;
  in = p->in->data;
  out = p->out->data;
  for (i=2;i<end;i+=2 ) {
    mag = HYPOT(in[i], in[i+1]);
    ph = ATAN2(in[i+1],in[i]);
    out[i] = mag; out[i+1] = ph;
  }
  return OK;
}

static int32_t perf_poltorect(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i, end = p->out->sizes[0];
  MYFLT *in, *out, re, im;
  in = p->in->data;
  out = p->out->data;
  for (i=2;i<end;i+=2) {
    re = in[i]*COS(in[i+1]);
    im = in[i]*SIN(in[i+1]);
    out[i] = re; out[i+1] = im;
  }
  return OK;
}

static int32_t init_poltorect2(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if(p->in2->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if (LIKELY(p->in2->sizes[0] == p->in->sizes[0])) {
    int32_t   N = p->in2->sizes[0];
    tabinit(csound, p->out, N*2 - 2, p->h.insdshead);
    return OK;
  } else return csound->InitError(csound,
                                  Str("in array sizes do not match: %d and %d\n"),
                                  p->in2->sizes[0],p->in->sizes[0]);
}


static int32_t perf_poltorect2(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->in->sizes[0]-1;
  MYFLT *mags, *phs, *out, re, im;
  mags = p->in->data;
  phs = p->in2->data;
  out = p->out->data;
  for (i=2,j=1;j<end;i+=2, j++) {
    re = mags[j]*COS(phs[j]);
    im = mags[j]*SIN(phs[j]);
    out[i] = re; out[i+1] = im;
  }
  out[0] = mags[0]*COS(phs[0]);
  out[1] = mags[end]*COS(phs[end]);
  return OK;
}

static int32_t init_mags(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  tabinit(csound, p->out, N/2+1, p->h.insdshead);
  return OK;
}

static int32_t perf_mags(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->out->sizes[0];
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  for (i=2,j=1;j<end-1;i+=2,j++)
    out[j] = HYPOT(in[i],in[i+1]);
  out[0] = fabs(in[0]);
  out[end-1] = fabs(in[1]);
  return OK;
}

static int32_t perf_phs(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->out->sizes[0];
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  for (i=2,j=1;j<end-1;i+=2,j++)
    out[j] = ATAN2(in[i+1],in[i]);
  return OK;
}

static int32_t init_logarray(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  tabinit(csound, p->out, p->in->sizes[0], p->h.insdshead);
  if (LIKELY(*((MYFLT *)p->in2)))
    p->b = 1/log(*((MYFLT *)p->in2));
  else
    p->b = FL(1.0);
  return OK;
}

static int32_t perf_logarray(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i, end = p->out->sizes[0];
  MYFLT bas = p->b;
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  if (LIKELY(bas))
    for (i=0;i<end;i++)
      out[i] = log(in[i])*bas;
  else
    for (i=0;i<end;i++)
      out[i] = log(in[i]);
  return OK;
}

static int32_t init_rtoc(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  tabinit(csound, p->out, N*2, p->h.insdshead);
  return OK;
}

static int32_t perf_rtoc(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->out->sizes[0];
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  for (i=0,j=0;i<end;i+=2,j++) {
    out[i] = in[j];
    out[i+1] = FL(0.0);
  }
  return OK;
}

static int32_t rtoc_i(CSOUND *csound, FFT *p) {
  if (LIKELY(init_rtoc(csound,p) == OK))
    return perf_rtoc(csound, p);
  else return NOTOK;
}

static int32_t init_ctor(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  tabinit(csound, p->out, N/2, p->h.insdshead);
  return OK;
}


static int32_t perf_ctor(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->out->sizes[0];
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  for (i=0,j=0;j<end;i+=2,j++)
    out[j] = in[i];
  return OK;
}

static int32_t ctor_i(CSOUND *csound, FFT *p) {
  if (LIKELY(init_ctor(csound,p) == OK))
    return perf_ctor(csound, p);
  else return NOTOK;
}


static int32_t init_window(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  int32_t   i,type = (int32_t) *p->f;
  MYFLT *w;
  tabinit(csound, p->out, N, p->h.insdshead);
  if (p->mem.auxp == 0 || p->mem.size < N*sizeof(MYFLT))
    csound->AuxAlloc(csound, N*sizeof(MYFLT), &p->mem);
  w = (MYFLT *) p->mem.auxp;
  switch(type) {
  case 0:
    for (i=0; i<N; i++) w[i] = 0.54 - 0.46*cos(i*TWOPI/N);
    break;
  case 1:
  default:
    for (i=0; i<N; i++) w[i] = 0.5 - 0.5*cos(i*TWOPI/N);
  }
  return OK;
}

static int32_t perf_window(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,end = p->out->sizes[0], off = *((MYFLT *)p->in2);
  MYFLT *in, *out, *w;
  in = p->in->data;
  out = p->out->data;
  w = (MYFLT *) p->mem.auxp;
  /*while (off < 0) off += end;
    for (i=0;i<end;i++)
    out[(i+off)%end] = in[i]*w[i];*/
  if(off) off = end - off;
  for(i=0;i<end;i++)
    out[i] = in[i]*w[(i+off)%end];
  return OK;

}

#include "pstream.h"

typedef struct _pvsceps {
  OPDS    h;
  ARRAYDAT  *out;
  PVSDAT  *fin;
  MYFLT   *coefs;
  void *setup;
  uint32_t  lastframe;
} PVSCEPS;

static int32_t pvsceps_init(CSOUND *csound, PVSCEPS *p) {
  int32_t N = p->fin->N;
  p->setup = csound->RealFFTSetup(csound, N/2, FFT_FWD);
  tabinit(csound, p->out, N/2+1, p->h.insdshead);
  p->lastframe = 0;
  return OK;
}

static int32_t pvsceps_perf(CSOUND *csound, PVSCEPS *p) {
  if (p->lastframe < p->fin->framecount) {
    int32_t N = p->fin->N;
    int32_t i, j;
    MYFLT *ceps = p->out->data;
    MYFLT coefs = *p->coefs;
    float *fin = (float *) p->fin->frame.auxp;
    for (i=j=0; i < N; i+=2, j++) {
      ceps[j] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
    }
    ceps[N/2] = fin[N/2];
    csound->RealFFT(csound, p->setup, ceps);
    if (coefs) {
      // lifter coefs
      for (i=coefs*2; i < N/2; i++) ceps[i] = 0.0;
      ceps[N/2] = 0.0;
    }
    p->lastframe = p->fin->framecount;
  }
  return OK;
}


static int32_t init_ceps(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t N = p->in->sizes[0]-1;
  if (UNLIKELY(N < 64))
    return csound->InitError(csound, "%s",
                             Str("FFT size too small (min 64 samples)\n"));
  p->setup = csound->RealFFTSetup(csound, N, FFT_FWD);
  tabinit(csound, p->out, N+1, p->h.insdshead);
  return OK;
}


static int32_t perf_ceps(CSOUND *csound, FFT *p) {
  int32_t siz = p->out->sizes[0]-1, i;
  MYFLT *ceps = p->out->data;
  MYFLT coefs = *((MYFLT *)p->in2);
  MYFLT *mags = (MYFLT *) p->in->data;
  for (i=0; i < siz; i++) {
    ceps[i] = log(mags[i] > 0.0 ? mags[i] : 1e-20);
  }
  ceps[siz] = mags[siz];
  csound->RealFFT(csound, p->setup, ceps);
  if (coefs) {
    // lifter coefs
    for (i=coefs*2; i < siz; i++) ceps[i] = 0.0;
    ceps[siz] = 0.0;
  }
  return OK;
}

static int32_t init_iceps(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t N = p->in->sizes[0]-1;
  p->setup = csound->RealFFTSetup(csound, N, FFT_INV);
  tabinit(csound, p->out, N+1, p->h.insdshead);
  N++;
  if (p->mem.auxp == NULL || p->mem.size < N*sizeof(MYFLT))
    csound->AuxAlloc(csound, N*sizeof(MYFLT), &p->mem);
  return OK;
}

static int32_t perf_iceps(CSOUND *csound, FFT *p) {
  int32_t siz = p->in->sizes[0]-1, i;
  MYFLT *spec = (MYFLT *)p->mem.auxp;
  MYFLT *out = p->out->data;
  memcpy(spec, p->in->data, siz*sizeof(MYFLT));
  csound->RealFFT(csound,p->setup,spec);
  for (i=0; i < siz; i++) {
    out[i] = exp(spec[i]);
  }
  out[siz] = spec[siz];       /* Writes outside data allocated */
  return OK;
}

static int32_t rows_init(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if (p->in->dimensions == 2) {
    int32_t siz = p->in->sizes[1];
    tabinit(csound, p->out, siz, p->h.insdshead);
    return OK;
  }
  else
    return csound->InitError(csound, "%s",
                             Str("in array not 2-dimensional\n"));
}

static int32_t rows_perf(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);
  if (LIKELY(start < p->in->sizes[0])) {
    int32_t bytes =  p->in->sizes[1]*sizeof(MYFLT);
    start *= p->in->sizes[1];
    memcpy(p->out->data,p->in->data+start,bytes);
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested row is out of range\n"));
}

/* Getrow for string arrays */
static int32_t rows_perf_S(CSOUND *csound, FFT *p)
{
  ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
  STRINGDAT* mem = (STRINGDAT*)dat->data;
  STRINGDAT* dest = (STRINGDAT*)p->out->data;
  int32_t i;
  int32_t index = (int32_t)(*((MYFLT *)p->in2));
  if (LIKELY(index < p->in->sizes[0])) {
    index = (index * dat->sizes[1]);
    //printf("%d : %d\n", index, dat->sizes[1]);
    mem += index;
    //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    //printf("*** mem = %p dst = %p\n", mem, dest);
    for (i = 0; i<p->in->sizes[1]; i++) {
      dat->arrayType->copyValue(csound, dat->arrayType, (void*)dest, (void*)mem, p->h.insdshead);
      //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
      dest +=1;
      mem += 1;
    }
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested row is out of range\n"));
}

static int32_t set_rows_perf_S(CSOUND *csound, FFT *p)
{
  ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
  STRINGDAT* mem = (STRINGDAT*)dat->data;
  STRINGDAT* dest = (STRINGDAT*)p->out->data;
  int32_t i;
  int32_t index = (int32_t)(*((MYFLT *)p->in2));
  if (LIKELY(index < p->in->sizes[0])) {
    index = (index * dat->sizes[1]);
    //printf("%d : %d\n", index, dat->sizes[1]);
    mem += index;
    //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    //printf("*** mem = %p dst = %p\n", mem, dest);
    for (i = 0; i<p->in->sizes[1]; i++) {
      dat->arrayType->copyValue(csound, dat->arrayType, (void*)mem, (void*)dest,  p->h.insdshead);
      //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
      dest+= 1;
      mem += 1;
    }
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested row is out of range\n"));
}


static int32_t rows_i(CSOUND *csound, FFT *p) {
  if (rows_init(csound,p) == OK) {
    int32_t start = *((MYFLT *)p->in2);
    if (LIKELY(start < p->in->sizes[0])) {
      int32_t bytes =  p->in->sizes[1]*sizeof(MYFLT);
      start *= p->in->sizes[1];
      memcpy(p->out->data,p->in->data+start,bytes);
      return OK;
    }
    else return csound->InitError(csound, "%s",
                                  Str("requested row is out of range\n"));

  }
  else return NOTOK;
}

static inline void tabensure2D(CSOUND *csound, ARRAYDAT *p,
                               int32_t rows, int32_t columns,
                               INSDS *ctx)
{
  if (p->data==NULL || p->dimensions == 0 ||
      (p->dimensions==2 && (p->sizes[0] < rows || p->sizes[1] < columns))) {
    size_t ss;
    if (p->data == NULL) {
      CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL, ctx);
      p->arrayMemberSize = var->memBlockSize;
    }
    ss = p->arrayMemberSize*rows*columns;
    if (p->data==NULL) {
      p->data = (MYFLT*)csound->Calloc(csound, ss);
      p->dimensions = 2;
      p->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t)*2);
    }
    else p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
    p->sizes[0] = rows;  p->sizes[1] = columns;
  }
}


static int32_t set_rows_init(CSOUND *csound, FFT *p) {
  int32_t sizs = p->in->sizes[0];
  int32_t row = *((MYFLT *)p->in2);
  tabensure2D(csound, p->out, row+1, sizs, p->h.insdshead);
  return OK;
}


static int32_t set_rows_init_S(CSOUND *csound, FFT *p) {
  if (set_rows_init(csound, p)==0)
    return set_rows_perf_S(csound, p);
  return NOTOK;
}

static int32_t rows_init_S(CSOUND *csound, FFT *p) {
  if (rows_init(csound, p)==0)
    return rows_perf_S(csound, p);
  return NOTOK;
}

static int32_t cols_init(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if (LIKELY(p->in->dimensions == 2)) {
    int32_t siz = p->in->sizes[0];
    tabinit(csound, p->out, siz, p->h.insdshead);
    return OK;
  }
  else
    return csound->InitError(csound, "%s",
                             Str("in array not 2-dimensional\n"));
}

static int32_t cols_perf(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);

  if (LIKELY(start < p->in->sizes[1])) {
    int32_t j,i,collen =  p->in->sizes[1], len = p->in->sizes[0];
    for (j=0,i=start; j < len; i+=collen, j++) {
      p->out->data[j] = p->in->data[i];
    }
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested col is out of range\n"));
}

static int32_t cols_i(CSOUND *csound, FFT *p) {
  if (cols_init(csound, p) == OK) {
    int32_t start = *((MYFLT *)p->in2);
    if (LIKELY(start < p->in->sizes[1])) {
      int32_t j,i,collen =  p->in->sizes[1], len = p->in->sizes[0];
      for (j=0,i=start; j < len; i+=collen, j++) {
        p->out->data[j] = p->in->data[i];
      }
      return OK;
    }
    else return csound->InitError(csound, "%s",
                                  Str("requested col is out of range\n"));
  }
  else return NOTOK;
}

static int32_t cols_perf_S(CSOUND *csound, FFT *p) {
  ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
  STRINGDAT* mem = (STRINGDAT*)dat->data;
  STRINGDAT* dest = (STRINGDAT*)p->out->data;
  int32_t i;
  int32_t index = (int32_t)(*((MYFLT *)p->in2));
  if (LIKELY(index < p->in->sizes[0])) {
    mem += index;
    for (i = 0; i<p->in->sizes[0]; i++) {
      dat->arrayType->copyValue(csound, dat->arrayType, (void*)dest, (void*)mem,
                                p->h.insdshead);
      dest+= 1;
      mem += p->in->sizes[1];
    }
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested col is out of range\n"));
}

static int32_t set_rows_perf(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);
  if (UNLIKELY(start < 0 || start >= p->out->sizes[0]))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("Error: index out of range\n"));
  int32_t bytes =  p->in->sizes[0]*sizeof(MYFLT);
  start *= p->out->sizes[1];
  memcpy(p->out->data+start,p->in->data,bytes);
  return OK;
}

static int32_t set_rows_i(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);
  set_rows_init(csound, p);
  if (UNLIKELY(start < 0 || start >= p->out->sizes[0]))
    return csound->InitError(csound, "%s",
                             Str("Error: index out of range\n"));
  int32_t bytes =  p->in->sizes[0]*sizeof(MYFLT);
  start *= p->out->sizes[1];
  memcpy(p->out->data+start,p->in->data,bytes);
  return OK;
}


static int32_t set_cols_init(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t siz = p->in->sizes[0];
  int32_t col = *((MYFLT *)p->in2);
  tabensure2D(csound, p->out, siz, col+1, p->h.insdshead);
  return OK;
}

static int32_t set_cols_perf(CSOUND *csound, FFT *p) {

  int32_t start = *((MYFLT *)p->in2);

  if (UNLIKELY(start < 0 || start >= p->out->sizes[1]))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("Error: index out of range\n"));
  if (UNLIKELY(p->in->dimensions != 1 || p->in->sizes[0]<p->out->sizes[0]))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("Error: New column too short\n"));


  int32_t j,i,row = p->out->sizes[1], col = p->out->sizes[0];
  for (j=0,i=start; j < col; i+=row, j++)
    p->out->data[i] = p->in->data[j];
  return OK;
}

static int32_t set_cols_i(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);
  set_cols_init(csound,p);
  if (UNLIKELY(start < 0 || start >= p->out->sizes[1]))
    return csound->InitError(csound, "%s",
                             Str("Error: index out of range\n"));
  if (UNLIKELY(p->in->dimensions != 1 || p->in->sizes[0]<p->out->sizes[0]))
    return csound->InitError(csound, "%s",
                             Str("Error: New column too short\n"));
  int32_t j,i,row = p->out->sizes[1], col = p->out->sizes[0];
  for (j=0,i=start; j < col; i+=row, j++)
    p->out->data[i] = p->in->data[j];
  return OK;
}

static int32_t set_cols_perf_S(CSOUND *csound, FFT *p) {
  ARRAYDAT* dat = p->in;      /* The data in 2_D array */
  STRINGDAT* mem = (STRINGDAT*)dat->data;
  STRINGDAT* dest = (STRINGDAT*)p->out->data;
  int32_t i;
  int32_t index = (int32_t)(*((MYFLT *)p->in2));
  if (LIKELY(index < p->in->sizes[0])) {
    index = (index * dat->sizes[1]);
    //printf("%d : %d\n", index, dat->sizes[1]);
    mem += index;
    //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    //printf("*** mem = %p dst = %p\n", mem, dest);
    for (i = 0; i<p->in->sizes[0]; i++) {
      dat->arrayType->copyValue(csound, dat->arrayType, (void*)mem, (void*)dest,
                                p->h.insdshead);
      //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
      dest += p->in->sizes[1];
      mem  += 1;
    }
    return OK;
  }
  else return csound->PerfError(csound,  &(p->h),
                                "%s", Str("requested col is out of range\n"));
}

static int32_t set_cols_init_S(CSOUND *csound, FFT *p) {
  if (set_cols_i(csound,p)==0)
    return set_cols_perf_S(csound, p);
  return NOTOK;
}

static int32_t cols_init_S(CSOUND *csound, FFT *p) {
  if (cols_init(csound,p)==0)
    return cols_perf_S(csound, p);
  return NOTOK;
}

static int32_t shiftin_init(CSOUND *csound, FFT *p) {

  int32_t sizs = CS_KSMPS;
  if(p->out->sizes[0] < sizs)
    tabinit(csound, p->out, sizs, p->h.insdshead);
  p->n = 0;
  return OK;
}

static int32_t shiftin_perf(CSOUND *csound, FFT *p) {
  IGN(csound);
  uint32_t  siz =  p->out->sizes[0], n = p->n;
  MYFLT *in = ((MYFLT *) p->in);
  if (n + CS_KSMPS < siz) {
    memcpy(p->out->data+n,in,CS_KSMPS*sizeof(MYFLT));
  }
  else {
    int32_t num = siz - n;
    memcpy(p->out->data+n,in,num*sizeof(MYFLT));
    memcpy(p->out->data,in+num,(CS_KSMPS-num)*sizeof(MYFLT));
  }
  p->n = (n + CS_KSMPS)%siz;
  return OK;
}


static int32_t shiftout_init(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t siz = p->in->sizes[0];
  p->n = ((int32_t)*((MYFLT *)p->in2) % siz);
  if (UNLIKELY((uint32_t) siz < CS_KSMPS))
    return csound->InitError(csound, "%s", Str("input array too small\n"));
  return OK;
}

static int32_t shiftout_perf(CSOUND *csound, FFT *p) {
  IGN(csound);
  uint32_t siz =  p->in->sizes[0], n = p->n;
  MYFLT *out = ((MYFLT *) p->out);

  if (n + CS_KSMPS < siz) {
    memcpy(out,p->in->data+n,CS_KSMPS*sizeof(MYFLT));
  }
  else {
    int32_t num = siz - n;
    memcpy(out,p->in->data+n,num*sizeof(MYFLT));
    memcpy(out+num,p->in->data,(CS_KSMPS-num)*sizeof(MYFLT));
  }
  p->n = (n + CS_KSMPS)%siz;
  return OK;
}


static int32_t unwrap(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");;
  int32_t i,siz = p->in->sizes[0];
  MYFLT *phs = p->out->data;
  for (i=0; i < siz; i++) {
    while (phs[i] >= PI) phs[i] -= TWOPI;
    while (phs[i] < -PI) phs[i] += TWOPI;
  }
  return OK;
}


static int32_t init_dct(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("dct: only one-dimensional arrays allowed"));
  tabinit(csound, p->out, N, p->h.insdshead);
  p->setup =  csound->DCTSetup(csound,N,FFT_FWD);
  return OK;
}

static int32_t kdct(CSOUND *csound, FFT *p) {
  // FIXME: IF N changes value do we need a check
  int32_t N = p->out->sizes[0];
  memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
  csound->DCT(csound,p->setup,p->out->data);
  return OK;
}

static int32_t dct(CSOUND *csound, FFT *p) {
  if (!init_dct(csound,p)) {
    kdct(csound,p);
    return OK;
  } else return NOTOK;
}

static int32_t init_dctinv(CSOUND *csound, FFT *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("dctinv: only one-dimensional arrays allowed"));
  tabinit(csound, p->out, N, p->h.insdshead);
  p->setup =  csound->DCTSetup(csound,N,FFT_INV);
  return OK;
}

static int32_t dctinv(CSOUND *csound, FFT *p) {
  if (LIKELY(!init_dctinv(csound,p))) {
    kdct(csound,p);
    return OK;
  } else return NOTOK;
}

static int32_t perf_pows(CSOUND *csound, FFT *p) {
  IGN(csound);
  int32_t i,j, end = p->out->sizes[0];
  MYFLT *in, *out;
  in = p->in->data;
  out = p->out->data;
  for (i=2,j=1;j<end-1;i+=2,j++)
    out[j] = in[i]*in[i]+in[i+1]*in[i+1];
  out[0] = in[0]*in[0];
  out[end-1] = in[1]*in[1];
  return OK;
}

typedef struct _MFB {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in;
  MYFLT *low;
  MYFLT *up;
  MYFLT *len;
  AUXCH  bins;
} MFB;

static inline MYFLT f2mel(MYFLT f) {
  return 1125.*log(1.+f/700.);
}

static inline int32_t mel2bin(MYFLT m, int32_t N, MYFLT sr) {
  MYFLT f = 700.*(exp(m/1125.) - 1.);
  return  (int32_t)(f/(sr/(2*N)));
}

static int32_t mfb_init(CSOUND *csound, MFB *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  int32_t   L = *p->len;
  int32_t N = p->in->sizes[0];
  if (LIKELY(L < N)) {
    tabinit(csound, p->out, L, p->h.insdshead);
  }
  else
    return csound->InitError(csound, "%s",
                             Str("mfb: filter bank size exceeds input array length"));
  if (p->bins.auxp == NULL || p->bins.size < (L+2)*sizeof(int32_t))
    csound->AuxAlloc(csound, (L+2)*sizeof(MYFLT), &p->bins);
  return OK;
}

static int32_t mfb(CSOUND *csound, MFB *p) {
  /* FIXME: Init cals tabinit but not checked in erf? */
  int32_t i,j;
  int32_t *bin = (int32_t *) p->bins.auxp;
  MYFLT start,max,end;
  MYFLT g = FL(0.0), incr, decr;
  int32_t L = p->out->sizes[0];
  int32_t N = p->in->sizes[0];
  MYFLT sum = FL(0.0);
  MYFLT *out = p->out->data;
  MYFLT *in = p->in->data;
  MYFLT sr = CS_ESR;

  start = f2mel(*p->low);
  end = f2mel(*p->up);
  incr = (end-start)/(L+1);



  for (i=0;i<L+2;i++) {
    bin[i] = (int32_t) mel2bin(start,N-1,sr);

    if (bin[i] > N) bin[i] = N;
    start += incr;
  }

  for (i=0; i < L; i++) {
    start = bin[i];
    max = bin[i+1];
    end = bin[i+2];
    incr =  1.0/(max - start);
    decr =  1.0/(end - max);
    for (j=start; j < max; j++) {
      sum += in[j]*g;
      g += incr;
    }
    g = FL(1.0);
    for (j=max; j < end; j++) {
      sum += in[j]*g;
      g -= decr;
    }
    out[i] = sum/(end - start);

    g = FL(0.0);
    sum = FL(0.0);
  }

  return OK;
}

static int32_t mfbi(CSOUND *csound, MFB *p) {
  if (LIKELY(mfb_init(csound,p) == OK))
    return mfb(csound,p);
  else return NOTOK;
}

typedef struct _centr{
  OPDS h;
  MYFLT *out;
  ARRAYDAT *in;
} CENTR;

static int32_t array_centroid(CSOUND *csound, CENTR *p) {
  if(p->in->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  MYFLT *in = p->in->data,a=FL(0.0),b=FL(0.0);
  int32_t NP1 = p->in->sizes[0];
  MYFLT f = CS_ESR/(2*(NP1 - 1)),cf;
  int32_t i;
  cf = f*FL(0.5);
  for (i=0; i < NP1-1; i++, cf+=f) {
    a += in[i];
    b += in[i]*cf;
  }
  *p->out = a > FL(0.0) ? b/a : FL(0.0);
  return OK;
}

typedef struct _inout {
  OPDS H;
  MYFLT *out, *in;
} INOUT;

static int32_t nxtpow2(CSOUND *csound, INOUT *p) {
  IGN(csound);
  int32_t inval = (int32_t)*p->in;
  int32_t powtwo = 2;
  while (powtwo < inval) powtwo *= 2;
  *p->out = powtwo;
  return OK;
}


typedef struct interl{
  OPDS h;
  ARRAYDAT *a;
  ARRAYDAT *b;
  ARRAYDAT *c;
} INTERL;


static int32_t interleave_i (CSOUND *csound, INTERL *p) {
  if(p->b->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if(p->c->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if(p->b->dimensions == 1 &&
     p->c->dimensions == 1 &&
     p->b->sizes[0] == p->c->sizes[0]) {
    int32_t len = p->b->sizes[0], i,j;
    tabinit(csound, p->a, len*2, p->h.insdshead);
    for(i = 0, j = 0; i < len; i++,j+=2) {
      p->a->data[j] =  p->b->data[i];
      p->a->data[j+1] = p->c->data[i];
    }
    return OK;
  }
  return csound->InitError(csound, "%s", Str("array inputs not in correct format\n"));
}

static int32_t interleave_perf (CSOUND *csound, INTERL *p) {
  int32_t len = p->b->sizes[0], i,j;
  tabcheck(csound, p->a, len*2, &(p->h));
  for(i = 0, j = 0; i < len; i++,j+=2) {
    p->a->data[j] =  p->b->data[i];
    p->a->data[j+1] = p->c->data[i];
  }
  return OK;
}

static int32_t deinterleave_i (CSOUND *csound, INTERL *p) {
  if(p->c->sizes == NULL)
    return csound->InitError(csound, "array not initialised\n");
  if(p->c->dimensions == 1) {
    int32_t len = p->c->sizes[0]/2, i,j;
    tabinit(csound, p->a, len, p->h.insdshead);
    tabinit(csound, p->b, len, p->h.insdshead);
    for(i = 0, j = 0; i < len; i++,j+=2) {
      p->a->data[i] =  p->c->data[j];
      p->b->data[i] = p->c->data[j+1];
    }
    return OK;
  }
  return csound->InitError(csound, "%s", Str("array inputs not in correct format\n"));
}

static int32_t deinterleave_perf (CSOUND *csound, INTERL *p) {
  int32_t len = p->c->sizes[0]/2, i,j;
  tabcheck(csound, p->a, len, &(p->h));
  tabcheck(csound, p->b, len, &(p->h));
  for(i = 0, j = 0; i < len; i++,j+=2) {
    p->a->data[i] =  p->c->data[j];
    p->b->data[i] = p->c->data[j+1];
  }
  return OK;
}

static OENTRY arrayvars_localops[] =
  {
    { "nxtpow2", sizeof(INOUT), 0, "i", "i", (SUBR)nxtpow2},
    { "fft", sizeof(FFT), 0, ":Complex;[]",":Complex;[]o",
      (SUBR) init_fft_complex, (SUBR) perf_fft_complex, NULL},
    { "fft", sizeof(FFT), 0, ":Complex;[]","k[]",
      (SUBR) init_fft_complex, (SUBR) perf_fft_complex, NULL},
    { "fft", sizeof(FFT), 0, "k[]",":Complex;[]",
      (SUBR) init_fft_complex, (SUBR) perf_fft_complex, NULL},
    { "rfft", sizeof(FFT), 0, ":Complex;[]","k[]",
      (SUBR) init_rfft_r2c, (SUBR) perf_rfft_r2c, NULL},
    { "rifft", sizeof(FFT), 0, "k[]", ":Complex;[]",
      (SUBR) init_rfft_c2r, (SUBR) perf_rfft_c2r, NULL},
    { "rfft", sizeof(FFT), 0, "k[]","k[]",
      (SUBR) init_rfft, (SUBR) perf_rfft, NULL},
    {"rfft", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) rfft_i, NULL, NULL},
    {"rifft", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_rifft, (SUBR) perf_rifft, NULL},
    {"rifft", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) rifft_i, NULL, NULL},
    {"cmplxprod", sizeof(FFT), 0, "k[]","k[]k[]",
     (SUBR) init_rfftmult, (SUBR) perf_rfftmult, NULL},
    {"fft", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) initialise_fft, (SUBR) perf_fft, NULL},
    {"fft", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) fft_i, NULL, NULL},
    {"fftinv", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_ifft, (SUBR) perf_ifft, NULL},
    {"fftinv", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) ifft_i, NULL, NULL},
    {"rect2pol", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_recttopol, NULL},
    {"pol2rect", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_poltorect, NULL},
    {"pol2rect.AA", sizeof(FFT), 0, "k[]","k[]k[]",
     (SUBR) init_poltorect2, (SUBR) perf_poltorect2, NULL},
    {"mags", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_mags, NULL},
    {"pows", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_pows, NULL},
    {"phs", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_phs, NULL},
    {"log", sizeof(FFT), 0, "k[]","k[]i",
     (SUBR) init_logarray, (SUBR) perf_logarray, NULL},
    {"r2c", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) rtoc_i, NULL, NULL},
    {"r2c", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_rtoc, (SUBR) perf_rtoc, NULL},
    {"c2r", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) ctor_i, NULL, NULL},
    {"c2r", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_ctor, (SUBR) perf_ctor, NULL},
    {"window", sizeof(FFT), 0, "k[]","k[]Op",
     (SUBR) init_window, (SUBR) perf_window, NULL},
    {"pvsceps", sizeof(PVSCEPS), 0, "k[]","fo",
     (SUBR) pvsceps_init, (SUBR) pvsceps_perf, NULL},
    {"cepsinv", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_iceps, (SUBR) perf_iceps, NULL},
    {"ceps", sizeof(FFT), 0, "k[]","k[]k",
     (SUBR) init_ceps, (SUBR) perf_ceps, NULL},
    {"getrow", sizeof(FFT), 0, "i[]","i[]i",
     (SUBR) rows_i, NULL, NULL},
    {"getrow.S", sizeof(FFT), 0, "S[]","S[]k",
     (SUBR) rows_init_S, (SUBR) rows_perf_S, NULL},
    {"getrow", sizeof(FFT), 0, "k[]","k[]k",
     (SUBR) rows_init, (SUBR) rows_perf, NULL},
    {"getcol", sizeof(FFT), 0, "i[]","i[]i",
     (SUBR) cols_i, NULL, NULL},
    {"getcol", sizeof(FFT), 0, "k[]","k[]k",
     (SUBR) cols_init, (SUBR) cols_perf, NULL},
    {"getcol", sizeof(FFT), 0, "S[]","S[]k",
     (SUBR) cols_init_S, (SUBR) cols_perf_S, NULL},
    {"setrow", sizeof(FFT), 0, "i[]","i[]i",
     (SUBR) set_rows_i, NULL, NULL},
    {"setrow", sizeof(FFT), 0, "k[]","k[]k",
     (SUBR) set_rows_init, (SUBR) set_rows_perf, NULL},
    {"setrow.S", sizeof(FFT), 0, "S[]","S[]k",
     (SUBR) set_rows_init_S, (SUBR) set_rows_perf_S, NULL},
    {"setcol", sizeof(FFT), 0, "i[]","i[]i",
     (SUBR) set_cols_i, NULL, NULL},
    {"setcol", sizeof(FFT), 0, "k[]","k[]k",
     (SUBR) set_cols_init, (SUBR) set_cols_perf, NULL},
    {"setcol", sizeof(FFT), 0, "S[]","S[]k",
     (SUBR) set_cols_init_S, (SUBR) set_cols_perf_S, NULL},
    {"shiftin", sizeof(FFT), 0, "k[]","a",
     (SUBR) shiftin_init, (SUBR) shiftin_perf},
    {"shiftout", sizeof(FFT), 0, "a","k[]o",
     (SUBR) shiftout_init, (SUBR) shiftout_perf},
    {"unwrap", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) unwrap},
    {"dct", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_dct, (SUBR) kdct, NULL},
    {"dct", sizeof(FFT), 0, "i[]","i[]",
     (SUBR) dct, NULL, NULL},
    {"dctinv", sizeof(FFT), 0, "k[]","k[]",
     (SUBR) init_dctinv, (SUBR) kdct, NULL},
    {"dctinv", sizeof(FFT), 0, "i[]","i[]",
     (SUBR)dctinv, NULL, NULL},
    {"mfb", sizeof(MFB), 0, "k[]","k[]kki",
     (SUBR) mfb_init, (SUBR) mfb, NULL},
    {"mfb", sizeof(MFB), 0, "i[]","i[]iii",
     (SUBR)mfbi, NULL, NULL},
    {"centroid", sizeof(CENTR), 0, "i","i[]",
     (SUBR) array_centroid, NULL, NULL},
    {"centroid", sizeof(CENTR), 0, "k","k[]", NULL,
     (SUBR)array_centroid, NULL},
    {"interleave", sizeof(INTERL), 0, "i[]","i[]i[]",
     (SUBR)interleave_i},
    {"interleave", sizeof(INTERL), 0, "k[]","k[]k[]",
     (SUBR)interleave_i, (SUBR) interleave_perf},
    {"deinterleave", sizeof(INTERL), 0, "i[]i[]","i[]",
     (SUBR)deinterleave_i},
    {"deinterleave", sizeof(INTERL), 0, "k[]k[]","k[]",
     (SUBR)deinterleave_i, (SUBR)deinterleave_perf},
    { "autocorr", sizeof(AUTOCORR), 0, "k[]", "k[]",
      (SUBR) init_autocorr, (SUBR) perf_autocorr },
  };

LINKAGE_BUILTIN(arrayvars_localops)
