/*
  pvlock.c:

  Copyright (C) 2009 V Lazzarini

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
#include "pstream.h"
#include "soundio.h"
#define MAXOUTS 2

#define FORMANT_MAX_ITERS 10
#define FORMANT_ITER_THRESH FL(0.23)
#define FORMANT_PITCH_LOW FL(0.97153194)
#define FORMANT_PITCH_HIGH FL(1.02930224)
#define FORMANT_FLOOR FL(1e-20)
#define FORMANT_MAX_GAIN FL(10.0)
#define FORMANT_MIN_ENV (FL(1.0)/FORMANT_MAX_GAIN)
#define PSHIFTER_PITCH_MIN FL(0.25)
#define PSHIFTER_PITCH_MAX FL(4.0)
#define PSHIFTER_RING_FACTOR 16

typedef struct {
  AUXCH fenv, ceps;
  void *fwdsetup, *invsetup;
} FORMANT_DATA;

typedef struct {
  int32_t N, d;
  void *setup;
} FFT_CACHE;

typedef struct {
  FFT_CACHE *entries;
  int32_t count, cap;
} FFT_CACHE_STORE;

typedef struct dats {
  OPDS h;
  cs_float *out[MAXOUTS], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *konset, *offset, *dbthresh, *ibeta, *kkeepform, *kcoefs;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  cs_double pos;
  cs_float accum, beta;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata, lockdata;
  cs_float *indata[2];
  cs_float *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  cs_float resamp;
  cs_double tstamp, incr;
  void *fwdsetup, *invsetup;
  FORMANT_DATA formant;
} DATASPACE;


typedef struct dats1 {
  OPDS h;
  cs_float *out[1], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *konset, *offset, *dbthresh, *ibeta, *kkeepform, *kcoefs;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  cs_double pos;
  cs_float accum, beta;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata, lockdata;
  cs_float *indata[2];
  cs_float *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  cs_float resamp;
  cs_double tstamp, incr;
  void *fwdsetup, *invsetup;
  FORMANT_DATA formant;
} DATASPACEM;


typedef struct datsmin {
  OPDS h;
  cs_float *out[MAXOUTS], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *ibeta, *kkeepform, *kcoefs;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  cs_double pos;
  cs_float accum, beta;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata, lockdata;
  cs_float *indata[2];
  cs_float *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  cs_float resamp;
  cs_double tstamp, incr;
  void *fwdsetup, *invsetup;
  FORMANT_DATA formant;
} DATASPACE_MIN;


typedef struct dats1min {
  OPDS h;
  cs_float *out[1], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *ibeta, *kkeepform, *kcoefs;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  cs_double pos;
  cs_float accum, beta;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata, lockdata;
  cs_float *indata[2];
  cs_float *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  cs_float resamp;
  cs_double tstamp, incr;
  void *fwdsetup, *invsetup;
  FORMANT_DATA formant;
} DATASPACEM_MIN;


typedef struct {
  OPDS h;
  cs_float *out, *del, *in, *kpitch, *kfscal, *klock, *kkeepform;
  cs_float *iN, *idecim, *ibeta, *kcoefs;
  int32_t cnt, hsize, curframe, N, decim, ringSize;
  uint64_t wp;
  double rp;
  cs_float beta;
  AUXCH ring, outframe, win, bwin, fwin, prev, framecount, lockdata, map;
  FFT_CACHE_STORE fftcache;
  void *fwdsetup, *invsetup;
  FORMANT_DATA formant;
} DATASPACE_PSHIFT;


typedef struct {
  cs_float *mag;
  int32_t *region, *cpeaks;
} PVLOCK_DATA;

static void pv_lock_arrays(AUXCH *lockdata, int32_t N, PVLOCK_DATA *ld)
{
  int32_t nbins = N/2 + 1;
  char *base = (char *) lockdata->auxp;
  ld->mag = (cs_float *) base;
  base += nbins*sizeof(cs_float);
  ld->region = (int32_t *) base;
  base += nbins*sizeof(int32_t);
  ld->cpeaks = (int32_t *) base;
}

static int32_t pv_alloc_lock(CSOUND *csound, AUXCH *lockdata, int32_t N)
{
  int32_t nbins = N/2 + 1;
  size_t size = (size_t) nbins*(sizeof(cs_float) + 2*sizeof(int32_t));
  if (lockdata->auxp == NULL || lockdata->size < size)
    csound->AuxAlloc(csound, size, lockdata);
  if (UNLIKELY(lockdata->auxp == NULL))
    return csound->InitError(csound, "%s\n",
                             Str("could not allocate phase-locking data"));
  return OK;
}

static int32_t pv_lock_mode(cs_float lock)
{
  if (lock == FL(2.0)) return 2;
  if (lock == FL(3.0)) return 3;
  return lock != FL(0.0);
}

static void *pv_fft_setup(CSOUND *csound, void *setup, int32_t N, int32_t d)
{
  if (setup != NULL && ((CSOUND_FFT_SETUP *)setup)->N == N) return setup;
  return csound->RealFFTSetup(csound, N, d);
}

static void pv_fft_cache_free(CSOUND *csound, FFT_CACHE_STORE *store)
{
  if (store->entries != NULL) {
    csound->Free(csound, store->entries);
    store->entries = NULL;
  }
  store->count = 0;
  store->cap = 0;
}

static void *pv_fft_cached_setup(CSOUND *csound, FFT_CACHE_STORE *store,
                                 int32_t N, int32_t d)
{
  int32_t i, cap;
  FFT_CACHE *entries;
  void *setup;

  for (i = 0; i < store->count; i++) {
    if (store->entries[i].N == N && store->entries[i].d == d)
      return store->entries[i].setup;
  }
  if (store->count == store->cap) {
    if (store->cap > INT32_MAX/2) return NULL;
    cap = store->cap == 0 ? 8 : store->cap*2;
    entries = csound->Malloc(csound, sizeof(FFT_CACHE)*(size_t)cap);
    if (entries == NULL) return NULL;
    if (store->count > 0)
      memcpy(entries, store->entries,
             sizeof(FFT_CACHE)*(size_t)store->count);
    if (store->entries != NULL) csound->Free(csound, store->entries);
    store->entries = entries;
    store->cap = cap;
  }
  setup = csound->RealFFTSetup(csound, N, d);
  if (setup == NULL) return NULL;
  store->entries[store->count].N = N;
  store->entries[store->count].d = d;
  store->entries[store->count].setup = setup;
  store->count++;
  return setup;
}

static int32_t pv_formant_init(CSOUND *csound, FORMANT_DATA *fd,
                               FFT_CACHE_STORE *cache, int32_t N)
{
  int32_t M = N/2;
  size_t size;

  if (M < 2) {
    fd->fwdsetup = fd->invsetup = NULL;
    return OK;
  }
  size = (size_t)(M + 2)*sizeof(cs_float);
  if (fd->fenv.auxp == NULL || fd->fenv.size < size)
    csound->AuxAlloc(csound, size, &fd->fenv);
  if (fd->ceps.auxp == NULL || fd->ceps.size < size)
    csound->AuxAlloc(csound, size, &fd->ceps);
  if (UNLIKELY(fd->fenv.auxp == NULL || fd->ceps.auxp == NULL)) {
    fd->fwdsetup = fd->invsetup = NULL;
    return csound->InitError(csound, "%s\n",
                             Str("could not allocate formant data"));
  }
  memset(fd->fenv.auxp, 0, size);
  memset(fd->ceps.auxp, 0, size);
  if (cache != NULL) {
    fd->fwdsetup = pv_fft_cached_setup(csound, cache, M, FFT_FWD);
    fd->invsetup = pv_fft_cached_setup(csound, cache, M, FFT_INV);
  }
  else {
    fd->fwdsetup = pv_fft_setup(csound, fd->fwdsetup, M, FFT_FWD);
    fd->invsetup = pv_fft_setup(csound, fd->invsetup, M, FFT_INV);
  }
  if (UNLIKELY(fd->fwdsetup == NULL || fd->invsetup == NULL))
    return csound->InitError(csound, "%s\n",
                             Str("could not create formant FFT setups"));
  return OK;
}

static int32_t pv_formant_active(FORMANT_DATA *fd, int32_t N,
                                 const cs_float *keepform, const cs_float *kcoefs,
                                 cs_float kpitch, int32_t *mode, int32_t *coefs)
{
  cs_float pitch, keep, coeff;
  int32_t m, n;

  if (fd->fwdsetup == NULL || fd->invsetup == NULL) return 0;
  keep = *keepform;
  if (!isfinite(keep)) m = 0;
  else if (keep >= FL(2.5)) m = 2;
  else if (keep < FL(0.5)) m = 0;
  else m = (int32_t)(keep + FL(0.5));
  if (m == 0) return 0;
  if (!isfinite(kpitch)) return 0;
  pitch = FABS(kpitch);
  if (pitch >= FORMANT_PITCH_LOW && pitch <= FORMANT_PITCH_HIGH) return 0;
  coeff = *kcoefs;
  if (!isfinite(coeff) || coeff < FL(0.5)) n = 80;
  else if (coeff >= (cs_float)(N/2)) n = N/2;
  else n = (int32_t)(coeff + FL(0.5));
  if (n < 1) n = 80;
  *mode = m;
  *coefs = n;
  return 1;
}

static void pv_formant_lifter(cs_float *ceps, int32_t M, int32_t coefs)
{
  int32_t k, limit;

  ceps[1] = FL(0.0);
  limit = 2*coefs;
  if (limit > M) limit = M;
  for (k = limit; k < M; k++) ceps[k] = FL(0.0);
}

static cs_float pv_formant_mag(const cs_float *win, int32_t N, int32_t k)
{
  if (k == 0) return FABS(win[0]);
  if (k == N/2) return FABS(win[N]);
  return HYPOT(win[2*k], win[2*k + 1]);
}

static void pv_formant_analyse(CSOUND *csound, FORMANT_DATA *fd, int32_t N,
                               int32_t mode, int32_t coefs,
                               cs_float *fwin, cs_float *bwin)
{
  int32_t M = N/2, iter, k, j, cond = 0;
  cs_float *fenv = (cs_float *) fd->fenv.auxp;
  cs_float *ceps = (cs_float *) fd->ceps.auxp;
  cs_float mag, diff, max = FL(0.0), lift = FL(0.0), nyq;

  for (k = 0; k < M; k++) {
    mag = pv_formant_mag(fwin, N, k);
    fenv[k] = LOG(mag > FORMANT_FLOOR ? mag : FORMANT_FLOOR);
  }
  for (iter = 0; iter < FORMANT_MAX_ITERS; iter++) {
    cond = 0;
    for (k = 0; k < M; k++) ceps[k] = fenv[k];
    csound->RealFFT(csound, fd->fwdsetup, ceps);
    pv_formant_lifter(ceps, M, coefs);
    csound->RealFFT(csound, fd->invsetup, ceps);
    for (k = 0; k < M; k++) {
      if (mode > 1) {
        if (fenv[k] < ceps[k]) fenv[k] = ceps[k];
        mag = pv_formant_mag(fwin, N, k);
        if (LOG(mag > FORMANT_FLOOR ? mag : FORMANT_FLOOR) - ceps[k]
            > FORMANT_ITER_THRESH)
          cond = 1;
      }
    }
    if (mode == 1 || !cond) break;
  }
  if (mode > 1 && cond) {
    for (k = 0; k < M; k++) {
      mag = pv_formant_mag(fwin, N, k);
      diff = LOG(mag > FORMANT_FLOOR ? mag : FORMANT_FLOOR) - ceps[k];
      if (diff > lift) lift = diff;
    }
    for (k = 0; k < M; k++) ceps[k] += lift;
  }
  for (k = 0; k < M; k++) {
    cs_float e = EXP(ceps[k]);
    if (e > max) max = e;
    fenv[k] = e;
  }
  if (max <= FL(0.0)) return;
  for (k = 0; k < M; k++) {
    fenv[k] /= max;
    if (fenv[k] < FORMANT_MIN_ENV) fenv[k] = FORMANT_MIN_ENV;
  }
  nyq = pv_formant_mag(fwin, N, M);
  if (nyq < FORMANT_FLOOR) nyq = FORMANT_FLOOR;
  fenv[M] = nyq/max;
  if (fenv[M] < FORMANT_MIN_ENV) fenv[M] = FORMANT_MIN_ENV;
  else if (fenv[M] > FL(1.0)) fenv[M] = FL(1.0);
  for (k = 1, j = 2; k < M; k++, j += 2) {
    cs_float inv = FL(1.0)/fenv[k];
    fwin[j] *= inv;
    fwin[j + 1] *= inv;
    bwin[j] *= inv;
    bwin[j + 1] *= inv;
  }
  fwin[N] /= fenv[M];
  bwin[N] /= fenv[M];
}

static void pv_formant_restore(FORMANT_DATA *fd, int32_t N, cs_float *fwin,
                               cs_float pitch)
{
  int32_t k, j, idx;
  int32_t M = N/2;
  cs_float *fenv = (cs_float *) fd->fenv.auxp;
  cs_float pos, frac, env;

  if (!isfinite(pitch)) return;
  pitch = FABS(pitch);
  for (k = 1, j = 2; k < M; k++, j += 2) {
    pos = pitch*(cs_float)k;
    if (pos < FL(1.0)) pos = FL(1.0);
    else if (pos > (cs_float)(M - 1)) pos = (cs_float)(M - 1);
    idx = (int32_t) pos;
    frac = pos - (cs_float) idx;
    if (idx >= M - 1) {
      idx = M - 1;
      frac = FL(0.0);
    }
    env = fenv[idx];
    if (frac > FL(0.0)) env += frac*(fenv[idx + 1] - env);
    fwin[j] *= env;
    fwin[j + 1] *= env;
  }
  pos = pitch*(cs_float)M;
  if (pos < FL(1.0)) pos = FL(1.0);
  else if (pos > (cs_float)M) pos = (cs_float)M;
  idx = (int32_t) pos;
  frac = pos - (cs_float) idx;
  if (idx >= M) {
    idx = M;
    frac = FL(0.0);
  }
  env = fenv[idx];
  if (frac > FL(0.0) && idx < M) env += frac*(fenv[idx + 1] - env);
  fwin[N] *= env;
  fwin[N + 1] *= env;
}

static cs_float pv_bin_mag2(const cs_float *buf, int32_t bin)
{
  return HYPOT(buf[2*bin], buf[2*bin + 1]);
}

static int32_t pv_find_peaks(const cs_float *buf, int32_t nbins,
                             cs_float *mag, int32_t *peaks)
{
  int32_t i, count = 0;
  for (i=0; i < nbins; i++) {
    mag[i] = pv_bin_mag2(buf, i);
    peaks[i] = 0;
  }
  for (i=2; i < nbins - 2; i++) {
    if (mag[i] > mag[i-2] && mag[i] > mag[i-1] &&
        mag[i] > mag[i+1] && mag[i] > mag[i+2]) {
      peaks[i] = 1;
      count++;
    }
  }
  return count;
}

static void pv_nearest_peaks(const int32_t *peaks, int32_t nbins,
                             int32_t *nearest)
{
  int32_t i, last = -1, next = -1;
  for (i=0; i < nbins; i++) {
    if (peaks[i]) last = i;
    nearest[i] = last;
  }
  for (i=nbins-1; i >= 0; i--) {
    if (peaks[i]) next = i;
    if (next >= 0 && (nearest[i] < 0 || next - i < i - nearest[i]))
      nearest[i] = next;
  }
}

static void pv_unit(cs_float re, cs_float im, cs_float *ur, cs_float *ui)
{
  cs_float mag = HYPOT(re, im);
  if (mag > 1e-20f) {
    *ur = re/mag;
    *ui = im/mag;
  }
  else {
    *ur = FL(1.0);
    *ui = FL(0.0);
  }
}

static cs_float pv_phase_delta(cs_float phase, cs_float last)
{
  cs_float delta = phase - last;
  if (delta > PI_F) delta -= FL(2.0)*PI_F;
  else if (delta < -PI_F) delta += FL(2.0)*PI_F;
  return delta;
}

static void pv_unwrap_region_phases(cs_float *phase, int32_t nbins,
                                    const int32_t *peaks,
                                    const int32_t *region)
{
  int32_t p, b;
  for (p=0; p < nbins; p++) {
    cs_float last, wrapped, unwrapped;
    if (!peaks[p]) continue;
    last = phase[p];
    wrapped = last;
    unwrapped = last;
    for (b=p+1; b < nbins && region[b] == p; b++) {
      cs_float current = phase[b];
      unwrapped += pv_phase_delta(current, wrapped);
      phase[b] = unwrapped;
      wrapped = current;
    }
    wrapped = last;
    unwrapped = last;
    for (b=p-1; b >= 0 && region[b] == p; b--) {
      cs_float current = phase[b];
      unwrapped += pv_phase_delta(current, wrapped);
      phase[b] = unwrapped;
      wrapped = current;
    }
  }
}

static void pv_lock_ld(AUXCH *lockdat, int32_t N, int32_t mode,
                       cs_float beta, cs_float *bwin, cs_float *fwin,
                       cs_float *prev)
{
  int32_t nbins = N/2 + 1, i, b, p;
  cs_float *mag, ph_real, ph_im, tmp_real, tmp_im, div;
  cs_float nr, ni, dr, di, phi_fk, phi_fp, theta, c, s;
  int32_t *region, *cpeaks;
  PVLOCK_DATA ld;

  pv_lock_arrays(lockdat, N, &ld);
  mag = ld.mag;
  region = ld.region;
  cpeaks = ld.cpeaks;

  pv_find_peaks(fwin, nbins, mag, cpeaks);
  pv_nearest_peaks(cpeaks, nbins, region);
  if (mode == 3 && beta != FL(1.0)) {
    for (b=0; b < nbins; b++)
      mag[b] = ATAN2(fwin[2*b + 1], fwin[2*b]);
    pv_unwrap_region_phases(mag, nbins, cpeaks, region);
  }

  for (b=0; b < nbins; b++) {
    i = 2*b;
    div = FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1e-20);
    ph_real = prev[i]*div;
    ph_im = prev[i+1]*div;
    tmp_real = bwin[i]*ph_real + bwin[i+1]*ph_im;
    tmp_im = bwin[i]*ph_im - bwin[i+1]*ph_real;
    bwin[i] = tmp_real;
    bwin[i+1] = tmp_im;
  }

  for (b=0; b < nbins; b++) {
    i = 2*b;
    p = region[b];
    nr = FL(1.0);
    ni = FL(0.0);
    if (p >= 0) {
      dr = bwin[2*p];
      di = bwin[2*p + 1];
      if (HYPOT(dr, di) > 1e-20f) {
        pv_unit(dr, di, &nr, &ni);
        if (mode == 3 && beta != FL(1.0)) {
          phi_fk = mag[b];
          phi_fp = mag[p];
          theta = (beta - FL(1.0))*(phi_fk - phi_fp);
          c = COS(theta);
          s = SIN(theta);
          tmp_real = nr*c - ni*s;
          ni = nr*s + ni*c;
          nr = tmp_real;
        }
      }
      else p = -1;
    }
    if (p < 0) pv_unit(bwin[i], bwin[i+1], &nr, &ni);

    tmp_real = fwin[i]*nr - fwin[i+1]*ni;
    tmp_im = fwin[i]*ni + fwin[i+1]*nr;
    prev[i] = fwin[i] = tmp_real;
    prev[i+1] = fwin[i+1] = tmp_im;
  }
}


static cs_float pshifter_ring_read(const AUXCH *ring, int32_t size,
                                   double pos)
{
  const cs_float *buf = (const cs_float *)ring->auxp;
  int64_t base;
  int32_t i0, i1;
  cs_float frac;

  if (buf == NULL || size < 1) return FL(0.0);
  base = (int64_t)floor(pos);
  i0 = (int32_t)(base % (int64_t)size);
  if (i0 < 0) i0 += size;
  i1 = i0 + 1;
  if (i1 >= size) i1 = 0;
  frac = (cs_float)(pos - (double)base);
  return buf[i0] + frac*(buf[i1] - buf[i0]);
}


static int32_t pshifter_valid_i32(double value)
{
  return value >= -2147483648.0 && value < 2147483648.0;
}


static cs_float pshifter_valid_pitch(cs_float pitch)
{
  if (!isfinite(pitch) || pitch <= FL(0.0)) return FL(1.0);
  if (pitch < PSHIFTER_PITCH_MIN) return PSHIFTER_PITCH_MIN;
  if (pitch > PSHIFTER_PITCH_MAX) return PSHIFTER_PITCH_MAX;
  return pitch;
}


static cs_float pshifter_valid_fscal(cs_float fscal)
{
  if (!isfinite(fscal) || fscal == FL(0.0)) return FL(1.0);
  fscal = FABS(fscal);
  if (fscal < PSHIFTER_PITCH_MIN) return PSHIFTER_PITCH_MIN;
  if (fscal > PSHIFTER_PITCH_MAX) return PSHIFTER_PITCH_MAX;
  return fscal;
}


static int32_t pshifter_deinit(CSOUND *csound, DATASPACE_PSHIFT *p)
{
  pv_fft_cache_free(csound, &p->fftcache);
  return OK;
}


static int32_t sinit(CSOUND *csound, DATASPACE *p)
{
    int32_t N =  *p->iN, ui;
    uint32_t nchans, i;
    uint32_t size;
    int32_t decim = *p->idecim;

    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t)intpow1(2, i-1);  /* faster than pow fn */
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->pos = 0;

    nchans = p->nchans;

    if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      return csound->InitError(csound, "%s\n", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(cs_float);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      memset(p->prev[i].auxp, 0, size);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(cs_float)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      cs_float x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((cs_float *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((cs_float)ui*x);
    }

    p->N = N;
    p->decim = decim;
    p->beta = *p->ibeta;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    {
      int32_t rc = pv_alloc_lock(csound, &p->lockdata, N);
      if (rc != OK) return rc;
      return pv_formant_init(csound, &p->formant, NULL, N);
    }
}

static int32_t sinitm(CSOUND *csound, DATASPACEM *p)
{
    int32_t N =  *p->iN, ui;
    uint32_t nchans, i;
    uint32_t size;
    int32_t decim = *p->idecim;

    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t)intpow1(2, i-1);  /* faster than pow fn */
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->pos = 0;

    nchans = p->nchans;

    if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      return csound->InitError(csound, "%s\n", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(cs_float);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      memset(p->prev[i].auxp, 0, size);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(cs_float)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      cs_float x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((cs_float *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((cs_float)ui*x);
    }

    p->N = N;
    p->decim = decim;
    p->beta = *p->ibeta;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    {
      int32_t rc = pv_alloc_lock(csound, &p->lockdata, N);
      if (rc != OK) return rc;
      return pv_formant_init(csound, &p->formant, NULL, N);
    }
}

static int32_t sinit_min(CSOUND *csound, DATASPACE_MIN *p)
{
    int32_t N =  *p->iN, ui;
    uint32_t nchans, i;
    uint32_t size;
    int32_t decim = *p->idecim;

    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t)intpow1(2, i-1);  /* faster than pow fn */
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->pos = 0;

    nchans = p->nchans;

    if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      return csound->InitError(csound, "%s\n", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(cs_float);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      memset(p->prev[i].auxp, 0, size);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(cs_float)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      cs_float x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((cs_float *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((cs_float)ui*x);
    }

    p->N = N;
    p->decim = decim;
    p->beta = *p->ibeta;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    {
      int32_t rc = pv_alloc_lock(csound, &p->lockdata, N);
      if (rc != OK) return rc;
      return pv_formant_init(csound, &p->formant, NULL, N);
    }
}

static int32_t sinitm_min(CSOUND *csound, DATASPACEM_MIN *p)
{
    int32_t N =  *p->iN, ui;
    uint32_t nchans, i;
    uint32_t size;
    int32_t decim = *p->idecim;

    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int32_t)intpow1(2, i-1);  /* faster than pow fn */
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->pos = 0;

    nchans = p->nchans;

    if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      return csound->InitError(csound, "%s\n", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(cs_float);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      memset(p->prev[i].auxp, 0, size);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(cs_float)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      cs_float x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((cs_float *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((cs_float)ui*x);
    }

    p->N = N;
    p->decim = decim;
    p->beta = *p->ibeta;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    {
      int32_t rc = pv_alloc_lock(csound, &p->lockdata, N);
      if (rc != OK) return rc;
      return pv_formant_init(csound, &p->formant, NULL, N);
    }
}

static int32_t sinit1(CSOUND *csound, DATASPACE_MIN *p) {
    p->nchans = GetOutputArgCnt((OPDS *)p);
    return sinit_min(csound, p);
}

static int32_t sinit1m(CSOUND *csound, DATASPACEM_MIN *p) {
    p->nchans = 1;
    return sinitm_min(csound, p);
}

static int32_t sprocess1(CSOUND *csound, DATASPACE_MIN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    cs_float pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    cs_float formant_pitch = *p->kpitch;
    int32_t formant = 0, fmode = 0, fcoefs = 0;
    cs_float *tab, frac;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int32_t nsmps = CS_KSMPS, n;
    int32_t sizefrs, size, post, i, j;
    int64_t spos;  // = p->pos;
    cs_double pos;
    cs_float *fwin, *bwin, in,
      *prev, *win = (cs_float *) p->win.auxp;
    cs_float *outframe;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt;
    int32_t curframe = p->curframe, decim = p->decim;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(cs_float));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        /* audio samples are stored in a function table */
        cs_double tim;
        cs_double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                    formant_pitch, &fmode, &fcoefs);
        pitch *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));



        /* spos is the reading position in samples, hsize is hopsize,
           time[n] is current read position in secs
           esr is sampling rate
        */
        tim = time[n]*resamp;
        spos  = hsize*(int64_t)(tim*CS_ESR/hsize);
        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;

        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (cs_float *) p->bwin[j].auxp;
          fwin = (cs_float *) p->fwin[j].auxp;
          prev = (cs_float *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (cs_float *) p->outframe[j].auxp;
          /* this loop fills two frames/windows with samples from table,
             reading is linearly-interpolated,
             frames are separated by 1 hopsize
          */
          for (i=0; i < N; i++) {
            /* front window, fwin */
            post = (int32_t) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i]; /* window it */
            /* back windo, bwin */
            post = (int32_t) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            bwin[i] = in * win[i];  /* window it */
            /* increment read pos according to pitch transposition */
            pos += pitch;
          }

          /* take the FFT of both frames
             re-order Nyquist bin from pos 1 to N
          */
          csound->RealFFT(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = 0.0;
          csound->RealFFT(csound,  p->fwdsetup, fwin);
          fwin[N] = fwin[1];
          fwin[N+1] = 0.0;

          if (formant)
            pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                               fwin, bwin);

          /* phase vocoder processing */

          if (mode > 1) {
            pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
          }
          else {
            for (i=0; i < N + 2; i+=2) {
              /* phases of previous output frame in exponential format,
                 obtained by dividing by magnitude */
              div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1e-20);
              ph_real  =    prev[i]*div;
              ph_im =       prev[i+1]*div;

              /* back window magnitudes, phase differences between
                 prev and back windows */
              tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
              tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
              bwin[i] = tmp_real;
              bwin[i+1] = tmp_im;
            }

            for (i=0; i < N + 2; i+=2) {
              if (mode) {  /* phase-locking */
                if (i > 0) {
                  if (i < N) {
                    tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                    tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                  }
                  else { /* Nyquist */
                    tmp_real = bwin[i] + bwin[i-2];
                    tmp_im = FL(0.0);
                  }
                }
                else { /* 0 Hz */
                  tmp_real = bwin[i] + bwin[i+2];
                  tmp_im = FL(0.0);
                }
              } else { /* no locking */
                tmp_real = bwin[i];
                tmp_im = bwin[i+1];
              }

              tmp_real = (cs_float)(tmp_real + 1e-15);
              div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

              /* phases of tmp frame */
              ph_real = tmp_real*div;
              ph_im = tmp_im*div;

              /* front window mags, phase sum of
                 tmp and front windows */
              tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
              tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

              /* phase vocoder output */
              prev[i] = fwin[i] = tmp_real;
              prev[i+1] = fwin[i+1] = tmp_im;
            }
          }
          if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
          /* re-order bins and take inverse FFT */
          fwin[1] = fwin[N];
          csound->RealFFT(csound, p->invsetup, fwin);
          /* frame counter */
          framecnt[curframe] = curframe*N;
          /* write to overlapped output frames */
          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

        }

        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }

      for (j=0; j < nchans; j++) {
        framecnt  = (int32_t *) p->framecount[j].auxp;
        outframe  = (cs_float *) p->outframe[j].auxp;
        out = p->out[j];
        out[n] = (cs_float)0;
        /* write output */
        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;
        }
        /* scale output */
        out[n] *= amp*scaling;
      }
      cnt++;
    }

    p->cnt = cnt;
    p->curframe = curframe;
    return OK;
}


static int32_t sprocess1m(CSOUND *csound, DATASPACEM_MIN *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    cs_float pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    cs_float formant_pitch = *p->kpitch;
    int32_t formant = 0, fmode = 0, fcoefs = 0;
    cs_float *tab, frac;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int32_t nsmps = CS_KSMPS, n;
    int32_t sizefrs, size, post, i, j;
    int64_t spos; //= p->pos;
    cs_double pos;
    cs_float *fwin, *bwin, in,
      *prev, *win = (cs_float *) p->win.auxp;
    cs_float *outframe;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt;
    int32_t curframe = p->curframe, decim = p->decim;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(cs_float));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        /* audio samples are stored in a function table */
        cs_double tim;
        cs_double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                    formant_pitch, &fmode, &fcoefs);
        pitch *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));



        /* spos is the reading position in samples, hsize is hopsize,
           time[n] is current read position in secs
           esr is sampling rate
        */
        tim = time[n]*resamp;
        spos  = hsize*(int64_t)(tim*CS_ESR/hsize);
        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;

        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (cs_float *) p->bwin[j].auxp;
          fwin = (cs_float *) p->fwin[j].auxp;
          prev = (cs_float *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (cs_float *) p->outframe[j].auxp;
          /* this loop fills two frames/windows with samples from table,
             reading is linearly-interpolated,
             frames are separated by 1 hopsize
          */
          for (i=0; i < N; i++) {
            /* front window, fwin */
            post = (int32_t) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i]; /* window it */
            /* back windo, bwin */
            post = (int32_t) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            bwin[i] = in * win[i];  /* window it */
            /* increment read pos according to pitch transposition */
            pos += pitch;
          }

          /* take the FFT of both frames
             re-order Nyquist bin from pos 1 to N
          */
          csound->RealFFT(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = 0.0;
          csound->RealFFT(csound,  p->fwdsetup, fwin);
          fwin[N] = fwin[1];
          fwin[N+1] = 0.0;

          if (formant)
            pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                               fwin, bwin);

          /* phase vocoder processing */

          if (mode > 1) {
            pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
          }
          else {
            for (i=0; i < N + 2; i+=2) {
              /* phases of previous output frame in exponential format,
                 obtained by dividing by magnitude */
              div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1e-20);
              ph_real  =    prev[i]*div;
              ph_im =       prev[i+1]*div;

              /* back window magnitudes, phase differences between
                 prev and back windows */
              tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
              tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
              bwin[i] = tmp_real;
              bwin[i+1] = tmp_im;
            }

            for (i=0; i < N + 2; i+=2) {
              if (mode) {  /* phase-locking */
                if (i > 0) {
                  if (i < N) {
                    tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                    tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                  }
                  else { /* Nyquist */
                    tmp_real = bwin[i] + bwin[i-2];
                    tmp_im = FL(0.0);
                  }
                }
                else { /* 0 Hz */
                  tmp_real = bwin[i] + bwin[i+2];
                  tmp_im = FL(0.0);
                }
              } else { /* no locking */
                tmp_real = bwin[i];
                tmp_im = bwin[i+1];
              }

              tmp_real = (cs_float)(tmp_real + 1e-15);
              div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

              /* phases of tmp frame */
              ph_real = tmp_real*div;
              ph_im = tmp_im*div;

              /* front window mags, phase sum of
                 tmp and front windows */
              tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
              tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

              /* phase vocoder output */
              prev[i] = fwin[i] = tmp_real;
              prev[i+1] = fwin[i+1] = tmp_im;
            }
          }
          if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
          /* re-order bins and take inverse FFT */
          fwin[1] = fwin[N];
          csound->RealFFT(csound, p->invsetup, fwin);
          /* frame counter */
          framecnt[curframe] = curframe*N;
          /* write to overlapped output frames */
          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

        }

        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }

      for (j=0; j < nchans; j++) {
        framecnt  = (int32_t *) p->framecount[j].auxp;
        outframe  = (cs_float *) p->outframe[j].auxp;
        out = p->out[j];
        out[n] = (cs_float)0;
        /* write output */
        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;
        }
        /* scale output */
        out[n] *= amp*scaling;
      }
      cnt++;
    }

    p->cnt = cnt;
    p->curframe = curframe;
    return OK;
}


static int32_t pshifterset(CSOUND *csound, DATASPACE_PSHIFT *p)
{
    double rawN = (double)*p->iN, rawDecim = (double)*p->idecim;
    int32_t N, ui;
    uint32_t i;
    size_t size, ringSamples, frameSamples, ringBytes, frameBytes;
    int32_t decim;
    cs_float pitch = pshifter_valid_pitch(*p->kpitch);

    if (UNLIKELY(!pshifter_valid_i32(rawN) || !pshifter_valid_i32(rawDecim)))
      return csound->InitError(csound, "%s\n",
                               Str("pshifter: invalid ifftsize or idecim"));
    N = (int32_t)rawN;
    decim = (int32_t)rawDecim;
    if (N > 0) {
      for (i=0; N; i++) N >>= 1;
      N = (int32_t)intpow1(2, i-1);
    } else N = 2048;
    if (N < 8) N = 8;
    if (decim == 0) decim = 8;
    if (UNLIKELY(decim < 4 || decim > N || N % decim != 0 ||
                  N > INT32_MAX/PSHIFTER_RING_FACTOR ||
                  N > INT32_MAX/decim))
      return csound->InitError(csound, "%s\n",
                               Str("pshifter: invalid ifftsize or idecim"));
    ringSamples = (size_t)N*PSHIFTER_RING_FACTOR;
    if (UNLIKELY(ringSamples > SIZE_MAX/sizeof(cs_float)))
      return csound->InitError(csound, "%s\n",
                               Str("pshifter: frame buffers too large"));
    ringBytes = ringSamples*sizeof(cs_float);
    if (UNLIKELY((size_t)decim > SIZE_MAX/(size_t)N))
      return csound->InitError(csound, "%s\n",
                               Str("pshifter: frame buffers too large"));
    frameSamples = (size_t)decim*(size_t)N;
    if (UNLIKELY(frameSamples > SIZE_MAX/sizeof(cs_float)))
      return csound->InitError(csound, "%s\n",
                               Str("pshifter: frame buffers too large"));
    frameBytes = frameSamples*sizeof(cs_float);

    p->N = N;
    p->decim = decim;
    p->hsize = N/decim;
    p->ringSize = N*PSHIFTER_RING_FACTOR;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->wp = 0;
    p->rp = -((double)N + (double)p->hsize*(double)pitch);
    p->beta = *p->ibeta;
    *p->del = (cs_float)N/CS_ESR;

    size = ringBytes;
    if (p->ring.auxp == NULL || p->ring.size < size)
      csound->AuxAlloc(csound, size, &p->ring);
    if (UNLIKELY(p->ring.auxp == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not allocate pshifter ring buffer"));
    memset(p->ring.auxp, 0, size);

    size = (size_t)(N + 2)*sizeof(cs_float);
    if (p->fwin.auxp == NULL || p->fwin.size < size)
      csound->AuxAlloc(csound, size, &p->fwin);
    if (p->bwin.auxp == NULL || p->bwin.size < size)
      csound->AuxAlloc(csound, size, &p->bwin);
    if (p->prev.auxp == NULL || p->prev.size < size)
      csound->AuxAlloc(csound, size, &p->prev);
    if (p->map.auxp == NULL || p->map.size < size)
      csound->AuxAlloc(csound, size, &p->map);
    if (UNLIKELY(p->fwin.auxp == NULL || p->bwin.auxp == NULL ||
                 p->prev.auxp == NULL || p->map.auxp == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not allocate pshifter frames"));
    memset(p->prev.auxp, 0, size);

    size = (size_t)decim*sizeof(int32_t);
    if (p->framecount.auxp == NULL || p->framecount.size < size)
      csound->AuxAlloc(csound, size, &p->framecount);
    if (UNLIKELY(p->framecount.auxp == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not allocate pshifter counters"));
    for (i=0; i < (uint32_t)decim; i++)
      ((int32_t *)(p->framecount.auxp))[i] = (int32_t)i*N;

    size = frameBytes;
    if (p->outframe.auxp == NULL || p->outframe.size < size)
      csound->AuxAlloc(csound, size, &p->outframe);
    if (UNLIKELY(p->outframe.auxp == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not allocate pshifter output frames"));
    memset(p->outframe.auxp, 0, size);

    size = (size_t)N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);
    if (UNLIKELY(p->win.auxp == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not allocate pshifter window"));
    {
      cs_float x = FL(2.0)*PI_F/(cs_float)N;
      for (ui=0; ui < N; ui++)
        ((cs_float *)p->win.auxp)[ui] =
          FL(0.5) - FL(0.5)*COS((cs_float)ui*x);
    }

    p->fwdsetup = pv_fft_cached_setup(csound, &p->fftcache, N, FFT_FWD);
    p->invsetup = pv_fft_cached_setup(csound, &p->fftcache, N, FFT_INV);
    if (UNLIKELY(p->fwdsetup == NULL || p->invsetup == NULL))
      return csound->InitError(csound, "%s\n",
                               Str("could not create pshifter FFT setups"));
    {
      int32_t rc = pv_alloc_lock(csound, &p->lockdata, N);
      if (rc != OK) return rc;
      return pv_formant_init(csound, &p->formant, &p->fftcache, N);
    }
}


static void pshifter_map_add(cs_float *dst, cs_float *pow,
                             const cs_float *src, int32_t dest,
                             cs_float weight)
{
  cs_float amp;
  if (weight <= FL(0.0)) return;
  amp = SQRT(weight);
  pow[dest] += weight*(src[0]*src[0] + src[1]*src[1]);
  dst[2*dest] += amp*src[0];
  dst[2*dest + 1] += amp*src[1];
}


static int32_t pshifterproc(CSOUND *csound, DATASPACE_PSHIFT *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;
    cs_float pitch = pshifter_valid_pitch(*p->kpitch);
    cs_float fscal = pshifter_valid_fscal(*p->kfscal);
    cs_float lock = *p->klock;
    cs_float formant_pitch = pitch/fscal;
    int32_t formant = 0, fmode = 0, fcoefs = 0;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt;
    int32_t nsmps = CS_KSMPS, n, i, j;
    int32_t decim = p->decim, curframe = p->curframe;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double scaling = (8./decim)/3.;
    cs_float *ring = (cs_float *)p->ring.auxp;
    cs_float *win = (cs_float *)p->win.auxp;
    cs_float *fwin = (cs_float *)p->fwin.auxp;
    cs_float *bwin = (cs_float *)p->bwin.auxp;
    cs_float *dstpow = (cs_float *)p->map.auxp;
    cs_float *prev = (cs_float *)p->prev.auxp;
    cs_float *outframe = (cs_float *)p->outframe.auxp;
    int32_t *framecnt = (int32_t *)p->framecount.auxp;
    cs_float *out = p->out, *in = p->in;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    double front;

    if (!isfinite(formant_pitch)) formant_pitch = FL(1.0);
    formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                formant_pitch, &fmode, &fcoefs);

    if (UNLIKELY(early)) {
      nsmps -= (int32_t)early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
    }
    if (UNLIKELY(offset)) {
      memset(out, '\0', offset*sizeof(cs_float));
    }

    for (n=(int32_t)offset; n < nsmps; n++) {
      ring[(int32_t)(p->wp % (uint64_t)p->ringSize)] = in[n];
      p->wp++;
      if (cnt == hsize) {
        front = (double)p->wp - (double)N - 1.0;
        p->rp = front - (double)hsize*(double)pitch;
        for (i=0; i < N; i++) {
          fwin[i] = pshifter_ring_read(&p->ring, p->ringSize,
                                       front + (double)i)*win[i];
          bwin[i] = pshifter_ring_read(&p->ring, p->ringSize,
                                       p->rp + (double)i)*win[i];
        }
        csound->RealFFT(csound, p->fwdsetup, bwin);
        bwin[N] = bwin[1];
        bwin[N+1] = FL(0.0);
        csound->RealFFT(csound, p->fwdsetup, fwin);
        fwin[N] = fwin[1];
        fwin[N+1] = FL(0.0);

        if (formant)
          pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                             fwin, bwin);

        if (mode > 1) {
          pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
        }
        else {
          for (i=0; i < N + 2; i+=2) {
            div = FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1e-20);
            ph_real = prev[i]*div;
            ph_im = prev[i+1]*div;
            tmp_real = bwin[i]*ph_real + bwin[i+1]*ph_im;
            tmp_im = bwin[i]*ph_im - bwin[i+1]*ph_real;
            bwin[i] = tmp_real;
            bwin[i+1] = tmp_im;
          }
          for (i=0; i < N + 2; i+=2) {
            if (mode) {
              if (i > 0) {
                if (i < N) {
                  tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                  tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                }
                else {
                  tmp_real = bwin[i] + bwin[i-2];
                  tmp_im = FL(0.0);
                }
              }
              else {
                tmp_real = bwin[i] + bwin[i+2];
                tmp_im = FL(0.0);
              }
            }
            else {
              tmp_real = bwin[i];
              tmp_im = bwin[i+1];
            }
            tmp_real = (cs_float)(tmp_real + 1e-15);
            div = FL(1.0)/(HYPOT(tmp_real, tmp_im));
            ph_real = tmp_real*div;
            ph_im = tmp_im*div;
            tmp_real = fwin[i]*ph_real - fwin[i+1]*ph_im;
            tmp_im = fwin[i]*ph_im + fwin[i+1]*ph_real;
            prev[i] = fwin[i] = tmp_real;
            prev[i+1] = fwin[i+1] = tmp_im;
          }
        }
        if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
        if (pitch != FL(1.0)) {
          int32_t k, q, M = N/2, dest;
          double pos;
          cs_float mag, ph;
          const cs_float *src;
          memset(bwin, 0, (size_t)(N + 2)*sizeof(cs_float));
          memset(dstpow, 0, (size_t)(N + 2)*sizeof(cs_float));
          bwin[0] = fwin[0];
          for (k = 1; k <= M; k++) {
            src = k == M ? fwin + N : fwin + 2*k;
            pos = (double)k*(double)pitch;
            dest = (int32_t)floor(pos + 0.5);
            if (dest >= 1 && dest < M)
              pshifter_map_add(bwin, dstpow, src, dest, FL(1.0));
          }
          for (q = 1; q < M; q++) {
            mag = dstpow[q];
            if (mag <= FL(0.0)) {
              fwin[2*q] = fwin[2*q + 1] = FL(0.0);
              continue;
            }
            mag = SQRT(mag);
            ph = HYPOT(bwin[2*q], bwin[2*q + 1]);
            if (ph > 1e-20f) {
              div = FL(1.0)/ph;
              fwin[2*q] = mag*bwin[2*q]*div;
              fwin[2*q + 1] = mag*bwin[2*q + 1]*div;
            }
            else fwin[2*q] = mag, fwin[2*q + 1] = FL(0.0);
          }
          fwin[N] = fwin[N + 1] = FL(0.0);
        }
        fwin[1] = fwin[N];
        csound->RealFFT(csound, p->invsetup, fwin);
        framecnt[curframe] = curframe*N;
        for (i=0; i < N; i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];
        cnt = 0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }
      out[n] = FL(0.0);
      for (j=0; j < decim; j++) {
        out[n] += outframe[framecnt[j]];
        framecnt[j]++;
      }
      out[n] *= (cs_float)scaling;
      cnt++;
    }
    p->cnt = cnt;
    p->curframe = curframe;
    return OK;
}


static int32_t sinit2m(CSOUND *csound, DATASPACEM *p)
{
    uint32_t size,i;
    p->nchans = GetOutputArgCnt((OPDS *)p);
    if (UNLIKELY(sinitm(csound, p) != OK)) return CSOUND_ERROR;
    size = p->N*sizeof(cs_float);
    for (i=0; i < p->nchans; i++)
      if (p->nwin[i].auxp == NULL || p->nwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->nwin[i]);
    p->pos = *p->offset*CS_ESR + p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    return OK;
}

static int32_t sinit2(CSOUND *csound, DATASPACE *p)
{
    uint32_t size,i;
    p->nchans = GetOutputArgCnt((OPDS *)p);
    if (UNLIKELY(sinit(csound, p) != OK)) return CSOUND_ERROR;
    size = p->N*sizeof(cs_float);
    for (i=0; i < p->nchans; i++)
      if (p->nwin[i].auxp == NULL || p->nwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->nwin[i]);
    p->pos = *p->offset*CS_ESR + p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    return OK;
}

static int32_t sprocess2(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    cs_float pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    cs_float *out, amp =*p->kamp;
    cs_float formant_pitch = *p->kpitch;
    int32_t formant = 0, fmode = 0, fcoefs = 0;
    cs_float *tab,frac,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    cs_double pos, spos = p->pos;
    cs_float *fwin, *bwin;
    cs_float in, *nwin, *prev;
    cs_float *win = (cs_float *) p->win.auxp, *outframe;
    cs_float powrat;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(cs_float));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        cs_double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                    formant_pitch, &fmode, &fcoefs);
        pitch *= resamp;
        time  *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (time < 0 || time >= 1 || !*p->konset) {
          spos += hsize*time;
          //csound->Message(csound, "position: %f \n", spos);
        }
        else if (p->tscale) {
          spos += hsize*(time/(1+p->accum));
          p->accum=0.0;
        }
        else  {
          spos += hsize;
          p->accum++;
          p->tscale = 1;
        }
        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h),
                                   "%s\n", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));

        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;


        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (cs_float *) p->bwin[j].auxp;
          fwin = (cs_float *) p->fwin[j].auxp;
          nwin = (cs_float *) p->nwin[j].auxp;
          prev = (cs_float *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (cs_float *) p->outframe[j].auxp;

          for (i=0; i < N; i++) {
            post = (int32_t) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;

            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i];

            post = (int32_t) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            //if (post >= 0 && post < size)
            if (post+nchans <  size)
              in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            //else in =  (cs_float) 0;

            bwin[i] = in * win[i];
            post = (int32_t) pos + hsize;
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            nwin[i] = in * win[i];
            pos += pitch;
          }

          csound->RealFFT(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          csound->RealFFT(csound, p->fwdsetup,  fwin);
          csound->RealFFT(csound,  p->fwdsetup, nwin);

          tmp_real = tmp_im = (cs_float) 1e-20;
          for (i=2; i < N; i++) {
            tmp_real += nwin[i]*nwin[i];
            if (i+1 < N) tmp_real += nwin[i+1]*nwin[i+1];
            tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
          }
          powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
          if (powrat > dbtresh) p->tscale=0;
          /*else tscale=1;*/

          fwin[N] = fwin[1];
          fwin[N+1] = FL(0.0);

          if (formant)
            pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                               fwin, bwin);

          if (mode > 1) {
            pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
          }
          else {
            for (i=0; i < N + 2; i+=2) {

              div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1.0e-20);
              ph_real  =    prev[i]*div;
              ph_im =       prev[i+1]*div;

              tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
              tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
              bwin[i] = tmp_real;
              bwin[i+1] = tmp_im;
            }

            for (i=0; i < N + 2; i+=2) {
              if (mode) {
                if (i > 0) {
                  if (i < N) {
                    tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                    tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                  }
                  else {
                    tmp_real = bwin[i] + bwin[i-2];
                    tmp_im = FL(0.0);
                  }
                }
                else {
                  tmp_real = bwin[i] + bwin[i+2];
                  tmp_im = FL(0.0);
                }
              }
              else {
                tmp_real = bwin[i];
                tmp_im = bwin[i+1];
              }

              tmp_real = (cs_float)(tmp_real + 1e-15);
              div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

              ph_real = tmp_real*div;
              ph_im = tmp_im*div;

              tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
              tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

              prev[i] = fwin[i] = tmp_real;
              prev[i+1] = fwin[i+1] = tmp_im;
            }
          }

          if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
          fwin[1] = fwin[N];
          csound->RealFFT(csound, p->invsetup, fwin);

          framecnt[curframe] = curframe*N;

          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

        }
        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }

      for (j=0; j < nchans; j++) {
        out = p->out[j];
        framecnt  = (int32_t *) p->framecount[j].auxp;
        outframe  = (cs_float *) p->outframe[j].auxp;

        out[n] = (cs_float) 0;

        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;
        }
        out[n] *= amp*scaling;
      }
      cnt++;
    }
    p->cnt = cnt;
    p->curframe = curframe;
    p->pos = spos;
    return OK;

}

static int32_t sprocess2m(CSOUND *csound, DATASPACEM *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    cs_float pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    cs_float *out, amp =*p->kamp;
    cs_float formant_pitch = *p->kpitch;
    int32_t formant = 0, fmode = 0, fcoefs = 0;
    cs_float *tab,frac,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    cs_double pos, spos = p->pos;
    cs_float *fwin, *bwin;
    cs_float in, *nwin, *prev;
    cs_float *win = (cs_float *) p->win.auxp, *outframe;
    cs_float powrat;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(cs_float));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(cs_float));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        cs_double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s\n", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                    formant_pitch, &fmode, &fcoefs);
        pitch *= resamp;
        time  *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (time < 0 || time >= 1 || !*p->konset) {
          spos += hsize*time;
          //csound->Message(csound, "position: %f \n", spos);
        }
        else if (p->tscale) {
          spos += hsize*(time/(1+p->accum));
          p->accum=0.0;
        }
        else  {
          spos += hsize;
          p->accum++;
          p->tscale = 1;
        }
        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h),
                                   "%s\n", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));

        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;


        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (cs_float *) p->bwin[j].auxp;
          fwin = (cs_float *) p->fwin[j].auxp;
          nwin = (cs_float *) p->nwin[j].auxp;
          prev = (cs_float *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (cs_float *) p->outframe[j].auxp;

          for (i=0; i < N; i++) {
            post = (int32_t) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;

            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i];

            post = (int32_t) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            //if (post >= 0 && post < size)
            if (post+nchans <  size)
              in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            //else in =  (cs_float) 0;

            bwin[i] = in * win[i];
            post = (int32_t) pos + hsize;
            post *= nchans;
            post += j;
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if (post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            nwin[i] = in * win[i];
            pos += pitch;
          }

          csound->RealFFT(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          csound->RealFFT(csound, p->fwdsetup,  fwin);
          csound->RealFFT(csound,  p->fwdsetup, nwin);

          tmp_real = tmp_im = (cs_float) 1e-20;
          for (i=2; i < N; i++) {
            tmp_real += nwin[i]*nwin[i];
            if (i+1 < N) tmp_real += nwin[i+1]*nwin[i+1];
            tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
          }
          powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
          if (powrat > dbtresh) p->tscale=0;
          /*else tscale=1;*/

          fwin[N] = fwin[1];
          fwin[N+1] = FL(0.0);

          if (formant)
            pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                               fwin, bwin);

          if (mode > 1) {
            pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
          }
          else {
            for (i=0; i < N + 2; i+=2) {

              div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1.0e-20);
              ph_real  =    prev[i]*div;
              ph_im =       prev[i+1]*div;

              tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
              tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
              bwin[i] = tmp_real;
              bwin[i+1] = tmp_im;
            }

            for (i=0; i < N + 2; i+=2) {
              if (mode) {
                if (i > 0) {
                  if (i < N) {
                    tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                    tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                  }
                  else {
                    tmp_real = bwin[i] + bwin[i-2];
                    tmp_im = FL(0.0);
                  }
                }
                else {
                  tmp_real = bwin[i] + bwin[i+2];
                  tmp_im = FL(0.0);
                }
              }
              else {
                tmp_real = bwin[i];
                tmp_im = bwin[i+1];
              }

              tmp_real = (cs_float)(tmp_real + 1e-15);
              div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

              ph_real = tmp_real*div;
              ph_im = tmp_im*div;

              tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
              tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

              prev[i] = fwin[i] = tmp_real;
              prev[i+1] = fwin[i+1] = tmp_im;
            }
          }

          if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
          fwin[1] = fwin[N];
          csound->RealFFT(csound, p->invsetup, fwin);

          framecnt[curframe] = curframe*N;

          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

        }
        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }

      for (j=0; j < nchans; j++) {
        out = p->out[j];
        framecnt  = (int32_t *) p->framecount[j].auxp;
        outframe  = (cs_float *) p->outframe[j].auxp;

        out[n] = (cs_float) 0;

        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;
        }
        out[n] *= amp*scaling;
      }
      cnt++;
    }
    p->cnt = cnt;
    p->curframe = curframe;
    p->pos = spos;
    return OK;

}

#define BUFS 20
static void fillbuf(CSOUND *csound, DATASPACE *p, int32_t nsmps);
/* file-reading version of temposcal */
static int32_t sinit3(CSOUND *csound, DATASPACE *p)
{
    uint32_t size,i;
    char *name;
    SFLIB_INFO sfinfo;
    // open file
    void *fd;
    name = ((STRINGDAT *)p->knum)->data;
    fd  = csound->FileOpen(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                            "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (p->sf == NULL)
      return csound->InitError(csound,
                               Str("filescal: failed to open file %s\n"), name);
    if (sfinfo.samplerate != CS_ESR)
      p->resamp = sfinfo.samplerate/CS_ESR;
    else
      p->resamp = 1;
    p->nchans = sfinfo.channels;

    if (p->OUTOCOUNT != p->nchans)
      return csound->InitError(csound,
                               Str("filescal: mismatched channel numbers. "
                                   "%d outputs, %d inputs\n"),
                               p->OUTOCOUNT, p->nchans);

    if (UNLIKELY(sinit(csound, p) != OK)) return CSOUND_ERROR;
    size = p->N*sizeof(cs_float);
    for (i=0; i < p->nchans; i++)
      if (p->nwin[i].auxp == NULL || p->nwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->nwin[i]);

    size = p->N*sizeof(cs_float)*BUFS;
    if (p->fdata.auxp == NULL || p->fdata.size < size)
      csound->AuxAlloc(csound, size, &p->fdata);
    p->indata[0] = p->fdata.auxp;
    p->indata[1] = (cs_float *) (((char*)p->fdata.auxp) + size/2);

    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    csound->FDRecord(csound, &(p->fdch));

    // fill buffers
    p->curbuf = 0;
    fillbuf(csound, p, p->N*BUFS/2);
    p->pos = *p->offset*CS_ESR + p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    p->tab = (cs_float *) p->indata[0];
    p->tstamp = 0.0;
    return OK;
}

/*
  this will read a buffer full of samples
  from disk position offset samps from the last
  call to fillbuf
*/

void fillbuf(CSOUND *csound, DATASPACE *p, int32_t nsmps) {

    IGN(csound);
    sf_count_t sampsread;
    // fill p->curbuf
    sampsread = csound->SndfileReadSamples(csound, p->sf, p->indata[p->curbuf],
                              nsmps);
    if (sampsread < nsmps)
      memset(p->indata[p->curbuf]+sampsread, 0,
             sizeof(cs_float)*(nsmps-sampsread));
    // point to the other
    p->curbuf = p->curbuf ? 0 : 1;
}

static int32_t sprocess3(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    cs_float pitch = *p->kpitch*p->resamp, time = *p->time, lock = *p->klock;
    cs_float formant_pitch = *p->kpitch;
    cs_float *out, amp =*p->kamp;
    int32_t formant, fmode = 0, fcoefs = 0;
    cs_float *tab,frac,  dbtresh = *p->dbthresh;
    //FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    cs_double pos, spos = p->pos;
    cs_float *fwin, *bwin;
    cs_float in, *nwin, *prev;
    cs_float *win = (cs_float *) p->win.auxp, *outframe;
    cs_float powrat;
    cs_float ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    int32_t mode = pv_lock_mode(lock);
    cs_float beta = p->beta;
    cs_double tstamp = p->tstamp, incrt = p->incr;

    formant = pv_formant_active(&p->formant, N, p->kkeepform, p->kcoefs,
                                *p->kpitch, &fmode, &fcoefs);
    if (time < 0) /* negative tempo is not possible */
      time = 0.0;
    time *= p->resamp;

    {
      int32_t outnum = GetOutputArgCnt((OPDS *)p);
      cs_double _0dbfs = csound->Get0dBFS(csound);

      if (UNLIKELY(early)) {
        nsmps -= early;
        for (j=0; j < nchans; j++) {
          out = p->out[j];
          memset(&out[nsmps], '\0', early*sizeof(cs_float));
        }
      }
      if (UNLIKELY(offset)) {
        for (j=0; j < nchans; j++) {
          out = p->out[j];
          memset(out, '\0', offset*sizeof(cs_float));
        }
      }

      for (n=offset; n < nsmps; n++) {

        if (cnt == hsize) {
          tab = p->tab;
          size = (int32_t) (p->fdata.size/sizeof(cs_float));

          if (time < 0 || time >= 1 || !*p->konset) {
            spos += hsize*time;
            incrt =  time*nsmps;
          }
          else if (p->tscale) {
            spos += hsize*(time/(1+p->accum));
            incrt =  (time/(1+p->accum))*nsmps;
            p->accum=0.0;
          }
          else {
            spos += hsize;
            incrt =  nsmps;
            p->accum++;
            p->tscale = 1;
          }
          sizefrs = size/nchans;

          while (spos > sizefrs) {
            spos -= sizefrs;
          }
          while (spos <= 0) {
            spos += sizefrs;
          }

          if (spos > (cs_float)(sizefrs/2) && p->curbuf == 0) {
            fillbuf(csound, p, size/2);
          } else if (spos < (cs_float)(sizefrs/2) && p->curbuf == 1) {
            fillbuf(csound, p, size/2);
          }

          for (j = 0; j < nchans; j++) {
            pos = spos;
            bwin = (cs_float *) p->bwin[j].auxp;
            fwin = (cs_float *) p->fwin[j].auxp;
            nwin = (cs_float *) p->nwin[j].auxp;
            prev = (cs_float *)p->prev[j].auxp;
            framecnt  = (int32_t *)p->framecount[j].auxp;
            outframe= (cs_float *) p->outframe[j].auxp;

            for (i=0; i < N; i++) {
              post = (int32_t) pos;
              frac = pos  - post;
              post *= nchans;
              post += j;

              while (post < 0) post += size;
              while (post >= size) post -= size;
              if (post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else {
                in = tab[post];
              }

              fwin[i] = in * win[i];

              post = (int32_t) (pos - hsize*pitch);
              post *= nchans;
              post += j;
              while (post < 0) post += size;
              while (post >= size) post -= size;
              if (post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else in = tab[post];
              bwin[i] = in * win[i];
              post = (int32_t) pos + hsize;
              post *= nchans;
              post += j;
              while (post < 0) post += size;
              while (post >= size) post -= size;
              if (post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else in = tab[post];
              nwin[i] = in * win[i];
              pos += pitch;
            }

            csound->RealFFT(csound,  p->fwdsetup,  bwin);
            bwin[N] = bwin[1];
            bwin[N+1] = FL(0.0);
            csound->RealFFT(csound, p->fwdsetup,  fwin);
            csound->RealFFT(csound,  p->fwdsetup, nwin);

            tmp_real = tmp_im = (cs_float) 1e-20;
            for (i=2; i < N; i++) {
              tmp_real += nwin[i]*nwin[i];
              if (i+1 < N) tmp_real += nwin[i+1]*nwin[i+1];
              tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
            }
            powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
            if (powrat > dbtresh) p->tscale=0;
            /*else tscale=1;*/

            fwin[N] = fwin[1];
            fwin[N+1] = FL(0.0);

            if (formant)
              pv_formant_analyse(csound, &p->formant, N, fmode, fcoefs,
                                 fwin, bwin);

            if (mode > 1) {
              pv_lock_ld(&p->lockdata, N, mode, beta, bwin, fwin, prev);
            }
            else {
              for (i=0; i < N + 2; i+=2) {

                div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1.0e-20);
                ph_real  =    prev[i]*div;
                ph_im =       prev[i+1]*div;

                tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
                tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
                bwin[i] = tmp_real;
                bwin[i+1] = tmp_im;
              }

              for (i=0; i < N + 2; i+=2) {
                if (mode) {
                  if (i > 0) {
                    if (i < N) {
                      tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                      tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
                    }
                    else {
                      tmp_real = bwin[i] + bwin[i-2];
                      tmp_im = FL(0.0);
                    }
                  }
                  else {
                    tmp_real = bwin[i] + bwin[i+2];
                    tmp_im = FL(0.0);
                  }
                }
                else {
                  tmp_real = bwin[i];
                  tmp_im = bwin[i+1];
                }

                tmp_real = (cs_float)(tmp_real + 1e-15);
                div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

                ph_real = tmp_real*div;
                ph_im = tmp_im*div;

                tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
                tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

                prev[i] = fwin[i] = tmp_real;
                prev[i+1] = fwin[i+1] = tmp_im;
              }
            }

            if (formant) pv_formant_restore(&p->formant, N, fwin, formant_pitch);
            fwin[1] = fwin[N];
            csound->RealFFT(csound,  p->invsetup,  fwin);

            framecnt[curframe] = curframe*N;

            for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];

          }
          cnt=0;
          curframe++;
          if (curframe == decim) curframe = 0;
        }

        /* we only output as many channels as we have outs for */
        for (j=0; j < outnum; j++) {
          out = p->out[j];
          framecnt  = (int32_t *) p->framecount[j].auxp;
          outframe  = (cs_float *) p->outframe[j].auxp;
          out[n] = (cs_float) 0;

          for (i = 0; i < decim; i++) {
            out[n] += outframe[framecnt[i]];
            framecnt[i]++;
          }
          out[n] *= _0dbfs*amp*(2./3.);
        }
        cnt++;
      }
    }
    p->cnt = cnt;
    p->curframe = curframe;
    p->pos = spos;
    //printf("%f s  \n", tstamp/CS_ESR);
    p->tstamp = tstamp + incrt;
    p->incr = incrt;
    return OK;

}




typedef struct {
  OPDS h;
  PVSDAT *fout;
  PVSDAT *fin;
  cs_float *klock;
  cs_float  *file;
  uint32 lastframe;
}PVSLOCK;

static int32_t pvslockset(CSOUND *csound, PVSLOCK *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, "%s\n", Str("Unsafe to have same fsig as in and out"));
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ))
      return csound->InitError(csound, "%s\n", Str("pvsfreeze: signal format "
                                           "must be amp-freq."));

    return OK;
}


static int32_t pvslockproc(CSOUND *csound, PVSLOCK *p)
{
    IGN(csound);
    int32_t i;
    float *fout = (float *) p->fout->frame.auxp,
      *fin = (float *) p->fin->frame.auxp;
    int32_t N = p->fin->N;

    if (p->lastframe < p->fin->framecount) {
      memcpy(fout,fin, sizeof(float)*(N+2));
      //int32_t l=0;
      if (*p->klock) {
        for (i=2; i < N-4; i+=2) {
          float p2 = fin[i];
          float p3 = fin[i+2];
          float p1 = fin[i-2];
          float p4 = fin[i+4];
          float p5 = fin[i+6];
          if (p3 > p1 && p3 > p2 && p3 > p4 && p3 > p5) {
            float freq = fin[i+3], d;
            d = 0.01*freq;
            if (FABS(fout[i-1] - freq) < d)fout[i-1]  = freq;
            if (FABS(fout[i+1] - freq) < d)fout[i+1]  = freq;
            if (FABS(fout[i+3] - freq) < d)fout[i+3]  = freq;
            if (FABS(fout[i+5] - freq) < d)fout[i+5]  = freq;
            if (FABS(fout[i+7] - freq) < d)fout[i+7]  = freq;
            //l+=1;
          }
        }
        //printf("%d peak locks\n", l);
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

typedef struct hilb {
  OPDS h;
  cs_float *out[2], *in, *ifftsize, *ihopsize;
  AUXCH fftdata, inframe, outframe;
  AUXCH win, iframecnt, oframecnt;
  int32_t off, cnt, decim;
  int32_t N, hop;
} HILB;

/* Shared initialization checks for both output forms. Keep all frame indices
   representable as int32_t and all allocation sizes representable as size_t. */
static int32_t hilbert_sizes(CSOUND *csound, cs_double fftsize, cs_double hopsize,
                             int32_t *N, int32_t *h)
{
    int32_t requested, decim;
    if (UNLIKELY(!(fftsize >= 2.0 && fftsize <= (INT32_MAX + 0.0) &&
                   hopsize >= 1.0 && hopsize <= (INT32_MAX + 0.0))))
      return csound->InitError(csound, "%s\n",
                               Str("hilbert2: invalid FFT or hop size"));

    /* Retain the existing rounding down to powers of two and hop clamp. */
    requested = (int32_t)fftsize;
    *N = 1;
    while (requested > 1) {
      requested >>= 1;
      *N <<= 1;
    }
    requested = (int32_t)hopsize;
    *h = 1;
    while (requested > 1) {
      requested >>= 1;
      *h <<= 1;
    }
    if (*h > *N) *h = *N;
    decim = *N / *h;
    if (UNLIKELY(*N > INT32_MAX / 2 / decim ||
                 (size_t)*N > SIZE_MAX / (2 * sizeof(cs_float)) / decim))
      return csound->InitError(csound, "%s\n",
                               Str("hilbert2: frame buffers too large"));
    return OK;
}

/* Positive-frequency bins have conjugate partners in a real input. DC and
   Nyquist have no partner, so halve them when discarding the negative bins. */
#define HILBERT_ANALYTIC(frame, N) do {                                  \
    (frame)[0] *= FL(0.5);                                               \
    (frame)[1] *= FL(0.5);                                               \
    (frame)[N] *= FL(0.5);                                               \
    (frame)[(N)+1] *= FL(0.5);                                           \
    memset((frame)+(N)+2, 0, ((N)-2)*sizeof(cs_float));                      \
  } while (0)

static int32_t hilbert_init(CSOUND *csound, HILB *p) {
    int32_t N, h;
    size_t size;
    int32_t *p1, *p2, i, decim;

    if (hilbert_sizes(csound, *p->ifftsize, *p->ihopsize, &N, &h) != OK)
      return NOTOK;
    decim = N/h;

    size = (size_t)N*decim*sizeof(cs_float);
    if (p->inframe.auxp == NULL || p->inframe.size < size)
      csound->AuxAlloc(csound, size, &p->inframe);
    memset(p->inframe.auxp, 0, size);
    size *= 2;
    if (p->outframe.auxp == NULL || p->outframe.size < size)
      csound->AuxAlloc(csound, size, &p->outframe);
    memset(p->outframe.auxp, 0, size);
    size /= decim;
    if (p->fftdata.auxp == NULL || p->fftdata.size < size)
      csound->AuxAlloc(csound, size, &p->fftdata);
    memset(p->fftdata.auxp, 0, size);
    size = (N/h)*sizeof(int32_t);
    if (p->iframecnt.auxp == NULL || p->iframecnt.size < size)
      csound->AuxAlloc(csound, size, &p->iframecnt);
    if (p->oframecnt.auxp == NULL || p->oframecnt.size < size)
      csound->AuxAlloc(csound, size, &p->oframecnt);
    p1 = (int32_t *) p->iframecnt.auxp;
    p2 = (int32_t *) p->oframecnt.auxp;
    for(i = 0; i < N/h; i++) {
      p1[i] = (decim - 1 - i)*h;
      p2[i] = 2*(decim - 1 - i)*h;
    }

    size = (size_t)2*N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);
    {
      cs_float *win = (cs_float *)p->win.auxp;
      cs_float x = FL(2.0)*PI_F/N;
      cs_float scale = decim < 4 ? FL(1.0) : FL(16.0)/(3*decim);
      /* Keep Hann windows and the original scale, including at large hops.
         Apply the scale here so synthesis needs only one multiplication. */
      for (i=0; i < N; i++) {
        win[i] = FL(0.5) - FL(0.5)*COS((cs_float)i*x);
        win[N+i] = win[i]*scale;
      }
    }

    p->cnt = 0;
    p->off = 0;
    p->N = N;
    p->hop = h;
    return OK;
}

static int32_t hilbert_proc(CSOUND *csound, HILB *p) {

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t n, nsmps = CS_KSMPS, off = p->off, decim = p->N/p->hop;
    int32_t hopsize = p->hop, fftsize = p->N;
    int32_t i,k,j, cnt = p->cnt;
    int32_t *iframecnt = (int32_t *) p->iframecnt.auxp;
    int32_t *oframecnt = (int32_t *) p->oframecnt.auxp;
    cs_float *fftdata = (cs_float *) p->fftdata.auxp;
    cs_float *inframe = (cs_float *) p->inframe.auxp;
    cs_float *outframe = (cs_float *) p->outframe.auxp;
    cs_float *win = (cs_float *) p->win.auxp;
    cs_float *swin = win + fftsize;
    cs_float **out = p->out;
    cs_float *in = p->in;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < 2; j++) {
        memset(&out[j][nsmps], '\0', early*sizeof(cs_float));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < 2; j++) {
        memset(out[j], '\0', offset*sizeof(cs_float));
      }
    }

    for (n=offset; n < nsmps; n++, cnt++) {
      if (cnt == hopsize) {
        cnt = 0;
        for(i = j = 0; i < fftsize; i++, j+=2) {
          fftdata[j] = inframe[i+off]*win[i];
          fftdata[j+1] = FL(0.0);
        }
        csound->ComplexFFT(csound, fftdata, fftsize);
        HILBERT_ANALYTIC(fftdata, fftsize);
        csound->InverseComplexFFT(csound, fftdata, fftsize);
        for(i = j = 0; i < fftsize; i++, j+=2) {
          outframe[j+2*off] = fftdata[j]*swin[i];
          outframe[j+1+2*off] = fftdata[j+1]*swin[i];
        }
        off += fftsize;
        p->off = off = off%(fftsize*decim);
      }
      cs_float input = in[n];
      out[0][n] = out[1][n] = FL(0.0);
      for (i = 0; i < decim; i++) {
        inframe[iframecnt[i]+i*fftsize] = input;
        iframecnt[i] = iframecnt[i] == fftsize-1 ? 0 : iframecnt[i]+1;
        k = 2*i*fftsize;
        out[0][n] += outframe[oframecnt[i]+k];
        out[1][n] += outframe[oframecnt[i]+k+1];
        oframecnt[i] = oframecnt[i] == 2*fftsize-2 ? 0 : oframecnt[i]+2;
      }
    }
    p->cnt = cnt;
    return OK;
}

#include "arrays.h"
typedef struct hilba {
  OPDS h;
  ARRAYDAT *out;
  cs_float *in, *ifftsize, *ihopsize;
  AUXCH fftdata, inframe, outframe;
  AUXCH win, iframecnt, oframecnt;
  int32_t off, cnt, decim;
  int32_t N, hop;
} HILBA;

static int32_t hilbert_array_init(CSOUND *csound, HILBA *p) {
    int32_t N, h;
    size_t size;
    int32_t *p1, *p2, i, decim;

    if (hilbert_sizes(csound, *p->ifftsize, *p->ihopsize, &N, &h) != OK)
      return NOTOK;
    decim = N/h;

    size = (size_t)N*decim*sizeof(cs_float);
    if (p->inframe.auxp == NULL || p->inframe.size < size)
      csound->AuxAlloc(csound, size, &p->inframe);
    memset(p->inframe.auxp, 0, size);
    size *= 2;
    if (p->outframe.auxp == NULL || p->outframe.size < size)
      csound->AuxAlloc(csound, size, &p->outframe);
    memset(p->outframe.auxp, 0, size);
    size /= decim;
    if (p->fftdata.auxp == NULL || p->fftdata.size < size)
      csound->AuxAlloc(csound, size, &p->fftdata);
    memset(p->fftdata.auxp, 0, size);
    size = (N/h)*sizeof(int32_t);
    if (p->iframecnt.auxp == NULL || p->iframecnt.size < size)
      csound->AuxAlloc(csound, size, &p->iframecnt);
    if (p->oframecnt.auxp == NULL || p->oframecnt.size < size)
      csound->AuxAlloc(csound, size, &p->oframecnt);
    p1 = (int32_t *) p->iframecnt.auxp;
    p2 = (int32_t *) p->oframecnt.auxp;
    for(i = 0; i < N/h; i++) {
      p1[i] = (decim - 1 - i)*h;
      p2[i] = 2*(decim - 1 - i)*h;
    }

    size = (size_t)2*N*sizeof(cs_float);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);
    {
      cs_float *win = (cs_float *)p->win.auxp;
      cs_float x = FL(2.0)*PI_F/N;
      cs_float scale = decim < 4 ? FL(1.0) : FL(16.0)/(3*decim);
      /* Match the analysis and synthesis windows in the audio-output form. */
      for (i=0; i < N; i++) {
        win[i] = FL(0.5) - FL(0.5)*COS((cs_float)i*x);
        win[N+i] = win[i]*scale;
      }
    }

    p->cnt = 0;
    p->off = 0;
    p->N = N;
    p->hop = h;
    if (UNLIKELY(tabinit(csound, p->out, CS_KSMPS,
                         p->h.insdshead) != OK))
      return csound_array_init_resize_error(csound);
    for(uint32_t k=0; k < CS_KSMPS; k++)
      ((COMPLEXDAT *)p->out->data)[k].isPolar = 0;  
    return OK;
}

static int32_t hilbert_array_proc(CSOUND *csound, HILBA *p) {

    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t n, nsmps = CS_KSMPS, off = p->off, decim = p->N/p->hop;
    int32_t hopsize = p->hop, fftsize = p->N;
    int32_t i,k,j, cnt = p->cnt;
    int32_t *iframecnt = (int32_t *) p->iframecnt.auxp;
    int32_t *oframecnt = (int32_t *) p->oframecnt.auxp;
    cs_float *fftdata = (cs_float *) p->fftdata.auxp;
    cs_float *inframe = (cs_float *) p->inframe.auxp;
    cs_float *outframe = (cs_float *) p->outframe.auxp;
    cs_float *win = (cs_float *) p->win.auxp;
    cs_float *swin = win + fftsize;
    COMPLEXDAT *out;
    cs_float *in = p->in;

    if (UNLIKELY(tabcheck(csound, p->out, CS_KSMPS, &p->h) != OK))
      return NOTOK;
    out = (COMPLEXDAT *) p->out->data;

    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(COMPLEXDAT));
    }
    if (UNLIKELY(offset)) {
        memset(out, '\0', offset*sizeof(COMPLEXDAT));
    }

    for (n=offset; n < nsmps; n++, cnt++) {
      if (cnt == hopsize) {
        cnt = 0;
        for(i = j = 0; i < fftsize; i++, j+=2) {
          fftdata[j] = inframe[i+off]*win[i];
          fftdata[j+1] = FL(0.0);
        }
        csound->ComplexFFT(csound, fftdata, fftsize);
        HILBERT_ANALYTIC(fftdata, fftsize);
        csound->InverseComplexFFT(csound, fftdata, fftsize);
        for(i = j = 0; i < fftsize; i++, j+=2) {
          outframe[j+2*off] = fftdata[j]*swin[i];
          outframe[j+1+2*off] = fftdata[j+1]*swin[i];
        }
        off += fftsize;
        p->off = off = off%(fftsize*decim);
      }
      out[n].real = out[n].imag = FL(0.0);
      out[n].isPolar = 0;
      for (i = 0; i < decim; i++) {
        inframe[iframecnt[i]+i*fftsize] = in[n];
        iframecnt[i] = iframecnt[i] == fftsize-1 ? 0 : iframecnt[i]+1;
        k = 2*i*fftsize;
        out[n].real += outframe[oframecnt[i]+k];
        out[n].imag += outframe[oframecnt[i]+k+1];
        oframecnt[i] = oframecnt[i] == 2*fftsize-2 ? 0 : oframecnt[i]+2;
      }
    }
    p->cnt = cnt;
    return OK;
}



#undef HILBERT_ANALYTIC

typedef struct amfm {
  OPDS h;
  cs_float *am, *fm;
  cs_float *re, *im;
  cs_double ph;
  cs_double scal;
} AMFM;


static int32_t am_fm_init(CSOUND *csound, AMFM *p) {
    p->ph = FL(0.0);
    p->scal = CS_ESR/(2*PI);
    return OK;
}

static int32_t am_fm(CSOUND *csound, AMFM *p) {
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t n, nsmps = CS_KSMPS;
    cs_double oph = p->ph, f, ph;
    cs_float *fm = p->fm;
    cs_float *am = p->am;
    cs_float *re = p->re;
    cs_float *im = p->im;
    cs_float scal = p->scal;

    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&am[nsmps], '\0', early*sizeof(cs_float));
      memset(&fm[nsmps], '\0', early*sizeof(cs_float));
    }
    if (UNLIKELY(offset)) {
      memset(am, '\0', offset*sizeof(cs_float));
      memset(fm, '\0', offset*sizeof(cs_float));
    }

    for (n=offset; n < nsmps; n++) {
      ph = ATAN2(im[n], re[n]);
      am[n] = HYPOT(re[n], im[n]);
      f = ph - oph;
      oph = ph;
      if (f >= PI) f -= 2*PI;
      else if (f < -PI) f += 2*PI;
      fm[n] = f*scal;
    }
    p->ph = oph;
    return OK;
}


static OENTRY pvlock_localops[] =
  {
   {"mincer", sizeof(DATASPACEM_MIN), 0, "a", "akkkkoopOO",
    (SUBR)sinit1m,(SUBR)sprocess1m },
   {"pshifter", sizeof(DATASPACE_PSHIFT), 0, "ai", "akkkkoopO",
    (SUBR)pshifterset,(SUBR)pshifterproc,(SUBR)pshifter_deinit },
   {"mincer", sizeof(DATASPACE_MIN), 0, "mm", "akkkkoopOO",
    (SUBR)sinit1,(SUBR)sprocess1 },
   {"temposcal", sizeof(DATASPACEM), 0, "a", "kkkkkooPOPpOO",
    (SUBR)sinit2m,(SUBR)sprocess2m },
   {"temposcal", sizeof(DATASPACE), 0, "mm", "kkkkkooPOPpOO",
    (SUBR)sinit2,(SUBR)sprocess2 },
   {"filescal", sizeof(DATASPACE), 0, "mm", "kkkSkooPOPpOO",
    (SUBR)sinit3,(SUBR)sprocess3 },
   {"hilbert2", sizeof(HILB), 0, "aa", "aii", (SUBR) hilbert_init,
    (SUBR) hilbert_proc},
   {"hilbert2", sizeof(HILB), 0, ":Complex;[]", "aii", (SUBR) hilbert_array_init,
    (SUBR) hilbert_array_proc},
   {"fmanal", sizeof(AMFM), 0, "aa", "aa", (SUBR) am_fm_init,
    (SUBR) am_fm},
   {"pvslock", sizeof(PVSLOCK), 0, "f", "fk", (SUBR) pvslockset,
    (SUBR) pvslockproc},
};

LINKAGE_BUILTIN(pvlock_localops)
