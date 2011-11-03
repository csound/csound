/*
    crossfm.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"
#include "interlocks.h"
#include "pstream.h"
#define MAXOUTS 2

typedef struct dats{
  OPDS h;
  MYFLT *out[MAXOUTS], *time, *kamp, *kpitch, *knum, *klock, *iN, *idecim, *konset, *offset, *dbthresh;
  int cnt, hsize, curframe, N, decim,tscale,nchans;
  double pos;
  MYFLT accum;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS], nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS];
} DATASPACE;


static int sinit(CSOUND *csound, DATASPACE *p)
{

  int N =  *p->iN, i,size, nchans;
    int decim = *p->idecim;

    if (N) {
      for (i=0; N; i++){
        N >>= 1;
      }
      N = (int) pow(2., i-1.);
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;

    nchans = csound->GetOutputArgCnt(p);
  if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      csound->Die(csound, Str("invalid number of output arguments"));
    p->nchans = nchans;

    for (i=0; i < nchans; i++){
    size = (N+2)*sizeof(MYFLT);
    if (p->fwin[i].auxp == NULL || p->fwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->fwin[i]);
    if (p->bwin[i].auxp == NULL || p->bwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->bwin[i]);
    if (p->prev[i].auxp == NULL || p->prev[i].size < size)
      csound->AuxAlloc(csound, size, &p->prev[i]);
    size = decim*sizeof(int);
    if (p->framecount[i].auxp == NULL || p->framecount[i].size < size)
      csound->AuxAlloc(csound, size, &p->framecount[i]);
    memset(p->framecount[i].auxp,0,size);
    size = decim*sizeof(MYFLT)*N;
    if (p->outframe[i].auxp == NULL || p->outframe[i].size < size)
      csound->AuxAlloc(csound, size, &p->outframe[i]);
    memset(p->outframe[i].auxp,0,size);
    }
   size = N*sizeof(MYFLT);
    if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win);

    for (i=0; i < N; i++)
      ((MYFLT *)p->win.auxp)[i] = 0.5 - 0.5*cos(i*2*PI/N);

    p->N = N;
    p->decim = decim;
    return OK;
}


static int sprocess(CSOUND *csound, DATASPACE *p)
{
    MYFLT pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    MYFLT *tab, frac,scale;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int ksmps = csound->GetKsmps(csound), n;
    int sizefrs, size, post, i, j, spos = p->pos;
    double pos;
    MYFLT *fwin, *bwin, in,
      *prev, *win = (MYFLT *) p->win.auxp;
    MYFLT *outframe;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt;
    int curframe = p->curframe, decim = p->decim;

    for (n=0; n < ksmps; n++) {

      if (cnt == hsize) {
        /* audio samples are stored in a function table */
        ft = csound->FTnp2Find(csound,p->knum);
        tab = ft->ftable;
        size = ft->flen;
       
        if (UNLIKELY((int) ft->nchanls != nchans))
          return csound->PerfError(csound, Str("number of output arguments "
                                               "inconsistent with number of "
                                               "sound file channels"));

       

        /* spos is the reading position in samples, hsize is hopsize,
           time[n] is current read position in secs
           esr is sampling rate
        */
        spos  = hsize*(int)((time[n])*csound->esr/hsize);
        sizefrs = size/nchans;
        while(spos > sizefrs - N) spos -= sizefrs;
        while(spos <= hsize)  spos += sizefrs;
        pos = spos;

        for (j = 0; j < nchans; j++) {

          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;
          /* this loop fills two frames/windows with samples from table,
             reading is linearly-interpolated,
             frames are separated by 1 hopsize
          */
          for (i=0; i < N; i++) {
            /* front window, fwin */
            post = (int) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;
            if (post >= 0 && post < size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in =  (MYFLT) 0;
            fwin[i] = in * win[i]; /* window it */
            /* back windo, bwin */
            post = (int) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            if (post >= 0 && post < size)
              in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in =  (MYFLT) 0;
            bwin[i] = in * win[i];  /* window it */
            /* increment read pos according to pitch transposition */
            pos += pitch;
          }

          /* take the FFT of both frames
             re-order Nyquist bin from pos 1 to N
          */
          csound->RealFFT(csound, bwin, N);
          bwin[N] = bwin[1];
          bwin[N+1] = 0.0;
          csound->RealFFT(csound, fwin, N);
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
                if (i < N){
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
          csound->InverseRealFFT(csound, fwin, N);
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
        framecnt  = (int *) p->framecount[j].auxp;
        outframe  = (MYFLT *) p->outframe[j].auxp;
        out = p->out[j];
        out[n] = (MYFLT)0;
        /* write output */
        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;
        }
        /* scale output */
        out[n] *= amp*(2./3.);
      }
      cnt++;
    }
 
    p->cnt = cnt;
    p->curframe = curframe;
    return OK;

}

static int sinit2(CSOUND *csound, DATASPACE *p)
{
    int size,i;
    sinit(csound, p);
    size = p->N*sizeof(MYFLT);
    for (i=0; i < p->nchans; i++)
    if (p->nwin[i].auxp == NULL || p->nwin[i].size < size)
      csound->AuxAlloc(csound, size, &p->nwin[i]);
    p->pos = *p->offset*csound->esr + p->hsize;
    p->tscale  = 0;
    p->accum = 0;
    return OK;
}

static int sprocess2(CSOUND *csound, DATASPACE *p)
{
    MYFLT pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,scale,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int  ksmps = csound->GetKsmps(csound), n;
    int size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt, curframe = p->curframe;
    int decim = p->decim;

    for (n=0; n < ksmps; n++) {
      
      if (cnt == hsize){
        ft = csound->FTnp2Find(csound,p->knum);
        tab = ft->ftable;
        size = ft->flen;

        if (time < 0 || time >= 1 || !*p->konset) {
          spos += hsize*time;
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
        if (UNLIKELY((int) ft->nchanls != nchans))
          return csound->PerfError(csound, Str("number of output arguments "
                                               "inconsistent with number of "
                                               "sound file channels"));

        sizefrs = size/nchans;
        while(spos > sizefrs - N - hsize) spos -= sizefrs;
        while(spos <= hsize)  spos += sizefrs;
        pos = spos;

        for (j = 0; j < nchans; j++) {

          bwin = (MYFLT *) p->bwin[j].auxp;
          fwin = (MYFLT *) p->fwin[j].auxp;
          nwin = (MYFLT *) p->nwin[j].auxp;
          prev = (MYFLT *)p->prev[j].auxp;
          framecnt  = (int *)p->framecount[j].auxp;
          outframe= (MYFLT *) p->outframe[j].auxp;
        
          for (i=0; i < N; i++) {
            post = (int) pos;
            frac = pos  - post;
            post *= nchans;
            post += j;
            if (post >= 0 && post < size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in =  (MYFLT) 0;
            fwin[i] = in * win[i]; 
         
            post = (int) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            if (post >= 0 && post < size)
              in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in =  (MYFLT) 0;
            bwin[i] = in * win[i];  
            post = (int) pos + hsize;
            post *= nchans;
            post += j;
            if (post >= 0 && post < size) in =  tab[post];
            else in =  (MYFLT) 0;
            nwin[i] = in * win[i];
            pos += pitch;
          }
         
          csound->RealFFT(csound, bwin, N);
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          csound->RealFFT(csound, fwin, N);
          csound->RealFFT(csound, nwin, N);

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
          csound->InverseRealFFT(csound, fwin, N);

          framecnt[curframe] = curframe*N;

          for (i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];
        
        }
        cnt=0;
        curframe++;
        if (curframe == decim) curframe = 0;
      }
           
      for (j=0; j < nchans; j++) {
        out = p->out[j];
        framecnt  = (int *) p->framecount[j].auxp;
        outframe  = (MYFLT *) p->outframe[j].auxp;
 
        out[n] = (MYFLT) 0;
     
        for (i = 0; i < decim; i++) {
          out[n] += outframe[framecnt[i]];
          framecnt[i]++;        
        }
        out[n] *= amp*(2./3.);
      }
      cnt++;  
    }
    p->cnt = cnt;
    p->curframe = curframe;
    p->pos = spos;
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

static int pvslockset(CSOUND *csound, PVSLOCK *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
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
          return csound->InitError(csound, Str("pvsfreeze: signal format "
                                               "must be amp-freq."));

   return OK;
}


static int pvslockproc(CSOUND *csound, PVSLOCK *p)
{
    int i,n,k,j, maxmagi;
    float mag,  maxmag, *fout = (float *) p->fout->frame.auxp, 
      *fin = (float *) p->fin->frame.auxp;
    int N = p->fin->N;
  
    if (p->lastframe < p->fin->framecount) {
      memcpy(fout,fin, sizeof(float)*(N+2));
  
      if (*p->klock) {
        for (i=2, n = 0; i < N-4; i+=2){
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
          }
        }
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

static OENTRY pvlock_localops[] = {
  {"mincer", sizeof(DATASPACE), 5, "mm", "akkkkoo",
                                               (SUBR)sinit, NULL,(SUBR)sprocess },
  {"temposcal", sizeof(DATASPACE), 5, "mm", "kkkkkooPOP",
                                               (SUBR)sinit2, NULL,(SUBR)sprocess2 },
  {"pvslock", sizeof(PVSLOCK), 3, "f", "fk", (SUBR) pvslockset,
         (SUBR) pvslockproc},
};

LINKAGE1(pvlock_localops)
