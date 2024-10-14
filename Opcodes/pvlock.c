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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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

typedef struct dats {
  OPDS h;
  MYFLT *out[MAXOUTS], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *konset, *offset, *dbthresh;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  double pos;
  MYFLT accum;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata;
  MYFLT *indata[2];
  MYFLT *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  MYFLT resamp;
  double tstamp, incr;
  void *fwdsetup, *invsetup;
} DATASPACE;


typedef struct dats1 {
  OPDS h;
  MYFLT *out[1], *time, *kamp, *kpitch, *knum, *klock, *iN,
    *idecim, *konset, *offset, *dbthresh;
  int32_t cnt, hsize, curframe, N, decim,tscale;
  uint32_t nchans;
  double pos;
  MYFLT accum;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata;
  MYFLT *indata[2];
  MYFLT *tab;
  int32_t curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  MYFLT resamp;
  double tstamp, incr;
  void *fwdsetup, *invsetup;
} DATASPACEM;


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
      return csound->InitError(csound, "%s", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(MYFLT);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(MYFLT)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(MYFLT);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      MYFLT x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((MYFLT *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((MYFLT)ui*x);
    }

    p->N = N;
    p->decim = decim;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    return OK;
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
      return csound->InitError(csound, "%s", Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++) {

      size = (N+2)*sizeof(MYFLT);
      if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->fwin[i]);
      if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->bwin[i]);
      if (p->prev[i].auxp == NULL || p->prev[i].size < size)
        csound->AuxAlloc(csound, size, &p->prev[i]);
      size = decim*sizeof(int32_t);
      if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
        csound->AuxAlloc(csound, size, &p->framecount[i]);
      {
        int32_t k=0;
        for (k=0; k < decim; k++) {
          ((int32_t *)(p->framecount[i].auxp))[k] = k*N;
        }
      }
      size = decim*sizeof(MYFLT)*N;
      if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
        csound->AuxAlloc(csound, size, &p->outframe[i]);
      else
        memset(p->outframe[i].auxp,0,size);
    }
    size = N*sizeof(MYFLT);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    {
      MYFLT x = FL(2.0)*PI_F/N;
      for (ui=0; ui < N; ui++)
        ((MYFLT *)p->win.auxp)[ui] = FL(0.5) - FL(0.5)*COS((MYFLT)ui*x);
    }

    p->N = N;
    p->decim = decim;

    p->fwdsetup = csound->RealFFTSetup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFTSetup(csound, N, FFT_INV);

    return OK;
}

static int32_t sinit1(CSOUND *csound, DATASPACE *p) {
    p->nchans = GetOutputArgCnt((OPDS *)p);
    return sinit(csound, p);
}

static int32_t sinit1m(CSOUND *csound, DATASPACEM *p) {
    p->nchans = 1;
    return sinitm(csound, p);
}

static int32_t sprocess1(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    MYFLT *tab, frac;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int32_t nsmps = CS_KSMPS, n;
    int32_t sizefrs, size, post, i, j;
    int64_t spos;  // = p->pos;
    double pos;
    MYFLT *fwin, *bwin, in,
      *prev, *win = (MYFLT *) p->win.auxp;
    MYFLT *outframe;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt;
    int32_t curframe = p->curframe, decim = p->decim;
    double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(MYFLT));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        /* audio samples are stored in a function table */
        double tim;
        double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        pitch *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h), "%s", Str("number of output arguments "
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
          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;
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

          /* phase vocoder processing */

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
            if (lock) {  /* phase-locking */
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

            tmp_real += 1e-15;
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
        outframe  = (MYFLT *) p->outframe[j].auxp;
        out = p->out[j];
        out[n] = (MYFLT)0;
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


static int32_t sprocess1m(CSOUND *csound, DATASPACEM *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    MYFLT *tab, frac;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int32_t nsmps = CS_KSMPS, n;
    int32_t sizefrs, size, post, i, j;
    int64_t spos; //= p->pos;
    double pos;
    MYFLT *fwin, *bwin, in,
      *prev, *win = (MYFLT *) p->win.auxp;
    MYFLT *outframe;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt;
    int32_t curframe = p->curframe, decim = p->decim;
    double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(MYFLT));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        /* audio samples are stored in a function table */
        double tim;
        double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
        pitch *= resamp;
        tab = ft->ftable;
        size = ft->flen;

        if (UNLIKELY((int32_t) ft->nchanls != nchans))
          return csound->PerfError(csound, &(p->h), "%s", Str("number of output arguments "
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
          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;
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

          /* phase vocoder processing */

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
            if (lock) {  /* phase-locking */
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

            tmp_real += 1e-15;
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
        outframe  = (MYFLT *) p->outframe[j].auxp;
        out = p->out[j];
        out[n] = (MYFLT)0;
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



static int32_t sinit2m(CSOUND *csound, DATASPACEM *p)
{
    uint32_t size,i;
    p->nchans = GetOutputArgCnt((OPDS *)p);
    sinitm(csound, p);
    size = p->N*sizeof(MYFLT);
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
    sinit(csound, p);
    size = p->N*sizeof(MYFLT);
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
    MYFLT pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(MYFLT));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
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
                                   "%s", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));

        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;


        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          nwin = (MYFLT *) p->nwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;

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
            //else in =  (MYFLT) 0;

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

          tmp_real = tmp_im = (MYFLT) 1e-20;
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
            if (lock) {
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

            tmp_real += 1e-15;
            div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

            ph_real = tmp_real*div;
            ph_im = tmp_im*div;

            tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
            tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

            prev[i] = fwin[i] = tmp_real;
            prev[i+1] = fwin[i+1] = tmp_im;
          }

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
        outframe  = (MYFLT *) p->outframe[j].auxp;

        out[n] = (MYFLT) 0;

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
    MYFLT pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    double scaling = (8./decim)/3.;

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(&out[nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        memset(out, '\0', offset*sizeof(MYFLT));
      }
    }

    for (n=offset; n < nsmps; n++) {

      if (cnt == hsize) {
        double resamp;
        ft = csound->FTFind(csound,p->knum);
        if (UNLIKELY(ft==NULL))
          return csound->PerfError(csound, &(p->h), "%s", Str("function table not found"));
        resamp = ft->gen01args.sample_rate/CS_ESR;
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
                                   "%s", Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));

        sizefrs = size/nchans;
        while (spos > sizefrs) spos -= sizefrs;
        while (spos <= 0)  spos += sizefrs;


        for (j = 0; j < nchans; j++) {
          pos = spos;
          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          nwin = (MYFLT *) p->nwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int32_t *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;

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
            //else in =  (MYFLT) 0;

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

          tmp_real = tmp_im = (MYFLT) 1e-20;
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
            if (lock) {
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

            tmp_real += 1e-15;
            div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

            ph_real = tmp_real*div;
            ph_im = tmp_im*div;

            tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
            tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

            prev[i] = fwin[i] = tmp_real;
            prev[i+1] = fwin[i+1] = tmp_im;
          }

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
        outframe  = (MYFLT *) p->outframe[j].auxp;

        out[n] = (MYFLT) 0;

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

    sinit(csound, p);
    size = p->N*sizeof(MYFLT);
    for (i=0; i < p->nchans; i++)
      if (p->nwin[i].auxp == NULL || p->nwin[i].size < size)
        csound->AuxAlloc(csound, size, &p->nwin[i]);

    size = p->N*sizeof(MYFLT)*BUFS;
    if (p->fdata.auxp == NULL || p->fdata.size < size)
      csound->AuxAlloc(csound, size, &p->fdata);
    p->indata[0] = p->fdata.auxp;
    p->indata[1] = (MYFLT *) (((char*)p->fdata.auxp) + size/2);

    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    csound->FDRecord(csound, &(p->fdch));

    // fill buffers
    p->curbuf = 0;
    fillbuf(csound, p, p->N*BUFS/2);
    p->pos = *p->offset*CS_ESR + p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    p->tab = (MYFLT *) p->indata[0];
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
    sampsread = csound->SndfileRead(csound, p->sf, p->indata[p->curbuf],
                              nsmps/p->nchans);
    if (sampsread < nsmps)
      memset(p->indata[p->curbuf]+sampsread, 0,
             sizeof(MYFLT)*(nsmps-sampsread));
    // point to the other
    p->curbuf = p->curbuf ? 0 : 1;
}

static int32_t sprocess3(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch*p->resamp, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,  dbtresh = *p->dbthresh;
    //FUNC *ft;
    int32_t N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int32_t  nsmps = CS_KSMPS, n;
    int32_t size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int32_t *framecnt, curframe = p->curframe;
    int32_t decim = p->decim;
    double tstamp = p->tstamp, incrt = p->incr;

    if (time < 0) /* negative tempo is not possible */
      time = 0.0;
    time *= p->resamp;

    {
      int32_t outnum = GetOutputArgCnt((OPDS *)p);
      double _0dbfs = csound->Get0dBFS(csound);

      if (UNLIKELY(early)) {
        nsmps -= early;
        for (j=0; j < nchans; j++) {
          out = p->out[j];
          memset(&out[nsmps], '\0', early*sizeof(MYFLT));
        }
      }
      if (UNLIKELY(offset)) {
        for (j=0; j < nchans; j++) {
          out = p->out[j];
          memset(out, '\0', offset*sizeof(MYFLT));
        }
      }

      for (n=offset; n < nsmps; n++) {

        if (cnt == hsize) {
          tab = p->tab;
          size = (int32_t) (p->fdata.size/sizeof(MYFLT));

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

          if (spos > (MYFLT)(sizefrs/2) && p->curbuf == 0) {
            fillbuf(csound, p, size/2);
          } else if (spos < (MYFLT)(sizefrs/2) && p->curbuf == 1) {
            fillbuf(csound, p, size/2);
          }

          for (j = 0; j < nchans; j++) {
            pos = spos;
            bwin = (MYFLT *) p->bwin[j].auxp;
            fwin = (MYFLT *) p->fwin[j].auxp;
            nwin = (MYFLT *) p->nwin[j].auxp;
            prev = (MYFLT *)p->prev[j].auxp;
            framecnt  = (int32_t *)p->framecount[j].auxp;
            outframe= (MYFLT *) p->outframe[j].auxp;

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

            tmp_real = tmp_im = (MYFLT) 1e-20;
            for (i=2; i < N; i++) {
              tmp_real += nwin[i]*nwin[i] + nwin[i+1]*nwin[i+1];
              tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
            }
            powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
            if (powrat > dbtresh) p->tscale=0;
            /*else tscale=1;*/

            fwin[N] = fwin[1];
            fwin[N+1] = FL(0.0);

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
              if (lock) {
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

              tmp_real += 1e-15;
              div =  FL(1.0)/(HYPOT(tmp_real, tmp_im));

              ph_real = tmp_real*div;
              ph_im = tmp_im*div;

              tmp_real =   fwin[i] * ph_real - fwin[i+1] * ph_im;
              tmp_im =   fwin[i] * ph_im + fwin[i+1] * ph_real;

              prev[i] = fwin[i] = tmp_real;
              prev[i+1] = fwin[i+1] = tmp_im;
            }

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
          outframe  = (MYFLT *) p->outframe[j].auxp;
          out[n] = (MYFLT) 0;

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
  MYFLT *klock;
  MYFLT  *file;
  uint32 lastframe;
}PVSLOCK;

static int32_t pvslockset(CSOUND *csound, PVSLOCK *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, "%s", Str("Unsafe to have same fsig as in and out"));
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
      return csound->InitError(csound, "%s", Str("pvsfreeze: signal format "
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
  MYFLT *out[2], *in, *ifftsize, *ihopsize;
  AUXCH fftdata, inframe, outframe;
  AUXCH win, iframecnt, oframecnt;
  int32_t off, cnt, decim;
  int32_t N, hop;
} HILB;

static int32_t hilbert_init(CSOUND *csound, HILB *p) {
    int32_t N = (int32_t) *p->ifftsize;
    int32_t h = (int32_t) *p->ihopsize;
    uint32_t size;
    int32_t *p1, *p2, i, decim;

    if (h > N) h = N;

    for (i=0; N; i++) {
      N >>= 1;
    }
    N = (int32_t)intpow1(2, i-1);

    for (i=0; h; i++) {
      h >>= 1;
    }
    h = (int32_t)intpow1(2, i-1);
    decim = N/h;

    size = (N*decim)*sizeof(MYFLT);
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

    size = N*sizeof(MYFLT);
    if (p->win.auxp == NULL || p->win.size < size) {
      MYFLT x = FL(2.0)*PI_F/N;
      csound->AuxAlloc(csound, size, &p->win);
      for (i=0; i < N; i++)
        ((MYFLT *)p->win.auxp)[i] = FL(0.5) - FL(0.5)*COS((MYFLT)i*x);
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
    MYFLT *fftdata = (MYFLT *) p->fftdata.auxp;
    MYFLT *inframe = (MYFLT *) p->inframe.auxp;
    MYFLT *outframe = (MYFLT *) p->outframe.auxp;
    MYFLT *win = (MYFLT *) p->win.auxp;
    MYFLT **out = p->out;
    MYFLT *in = p->in;
    MYFLT scal = decim < 4 ? 1 : 16./(3*decim);

    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j=0; j < 2; j++) {
        memset(&out[j][nsmps], '\0', early*sizeof(MYFLT));
      }
    }
    if (UNLIKELY(offset)) {
      for (j=0; j < 2; j++) {
        memset(out[j], '\0', offset*sizeof(MYFLT));
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
        fftdata[0] *= 0.5;
        fftdata[1] *= 0.5;
        memset(fftdata+fftsize, 0, fftsize*sizeof(MYFLT));
        csound->InverseComplexFFT(csound, fftdata, fftsize);
        for(i = j = 0; i < fftsize; i++, j+=2) {
          outframe[j+2*off] = fftdata[j]*win[i]*scal;
          outframe[j+1+2*off] = fftdata[j+1]*win[i]*scal;
        }
        off += fftsize;
        p->off = off = off%(fftsize*decim);
      }
      out[0][n] = out[1][n] = FL(0.0);
      for (i = 0; i < decim; i++) {
        inframe[iframecnt[i]+i*fftsize] = in[n];
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

typedef struct amfm {
  OPDS h;
  MYFLT *am, *fm;
  MYFLT *re, *im;
  double ph;
  double scal;
} AMFM;


int32_t am_fm_init(CSOUND *csound, AMFM *p) {
    p->ph = FL(0.0);
    p->scal = CS_ESR/(2*PI);
    return OK;
}

int32_t am_fm(CSOUND *csound, AMFM *p) {
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t n, nsmps = CS_KSMPS;
    double oph = p->ph, f, ph;
    MYFLT *fm = p->fm;
    MYFLT *am = p->am;
    MYFLT *re = p->re;
    MYFLT *im = p->im;
    MYFLT scal = p->scal;

    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&am[nsmps], '\0', early*sizeof(MYFLT));
      memset(&fm[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (UNLIKELY(offset)) {
      memset(&am[nsmps], '\0', offset*sizeof(MYFLT));
      memset(&fm[nsmps], '\0', offset*sizeof(MYFLT));
    }

    for (n=offset; n < nsmps; n++) {
      am[n] = HYPOT(re[n], im[n]);
      ph = ATAN2(im[n], re[n]);
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
   {"mincer", sizeof(DATASPACEM), 0, "a", "akkkkoo",
    (SUBR)sinit1m,(SUBR)sprocess1m },
   {"mincer", sizeof(DATASPACE), 0, "mm", "akkkkoo",
    (SUBR)sinit1,(SUBR)sprocess1 },
   {"temposcal", sizeof(DATASPACEM), 0, "a", "kkkkkooPOP",
    (SUBR)sinit2m,(SUBR)sprocess2m },
   {"temposcal", sizeof(DATASPACE), 0, "mm", "kkkkkooPOP",
    (SUBR)sinit2,(SUBR)sprocess2 },
   {"filescal", sizeof(DATASPACE), 0, "mm", "kkkSkooPOP",
    (SUBR)sinit3,(SUBR)sprocess3 },
   {"hilbert2", sizeof(HILB), 0, "aa", "aii", (SUBR) hilbert_init,
    (SUBR) hilbert_proc},
   {"fmanal", sizeof(AMFM), 0, "aa", "aa", (SUBR) am_fm_init,
    (SUBR) am_fm},
   {"pvslock", sizeof(PVSLOCK), 0, "f", "fk", (SUBR) pvslockset,
    (SUBR) pvslockproc},
};

LINKAGE_BUILTIN(pvlock_localops)
