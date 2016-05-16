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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"
#include "interlocks.h"
#include "pstream.h"
#include "soundio.h"
#define MAXOUTS 2

typedef struct dats{
  OPDS h;
  MYFLT *out[MAXOUTS], *time, *kamp, *kpitch, *knum, *klock, *iN,
        *idecim, *konset, *offset, *dbthresh;
  int cnt, hsize, curframe, N, decim,tscale;
  unsigned int nchans;
  double pos;
  MYFLT accum;
  AUXCH outframe[MAXOUTS], win, bwin[MAXOUTS], fwin[MAXOUTS],
    nwin[MAXOUTS], prev[MAXOUTS], framecount[MAXOUTS], fdata;
  MYFLT *indata[2];
  MYFLT *tab;
  int curbuf;
  SNDFILE *sf;
  FDCH    fdch;
  MYFLT resamp;
  double tstamp, incr;
  void *fwdsetup, *invsetup;
} DATASPACE;



static int sinit(CSOUND *csound, DATASPACE *p)
{

    int N =  *p->iN, ui;
    unsigned int nchans, i;
    unsigned int size;
    int decim = *p->idecim;

    if (N) {
      for (i=0; N; i++) {
        N >>= 1;
      }
      N = (int) pow(2.0, i-1);
    } else N = 2048;
    if (decim == 0) decim = 4;

    p->hsize = N/decim;
    p->cnt = p->hsize;
    p->curframe = 0;
    p->pos = 0;

    nchans = p->nchans;

  if (UNLIKELY(nchans < 1 || nchans > MAXOUTS))
      csound->InitError(csound, Str("invalid number of output arguments"));
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
    {
      int k=0;
      for (k=0; k < decim; k++) {
        ((int *)(p->framecount[i].auxp))[k] = k*N;
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

    p->fwdsetup = csound->RealFFT2Setup(csound, N, FFT_FWD);
    p->invsetup = csound->RealFFT2Setup(csound, N, FFT_INV);

    return OK;
}

static int sinit1(CSOUND *csound, DATASPACE *p){
    p->nchans = csound->GetOutputArgCnt(p);
    return sinit(csound, p);
}

static int sprocess1(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch, *time = p->time, lock = *p->klock,
      *out, amp =*p->kamp;
    MYFLT *tab, frac;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, nchans = p->nchans;
    int nsmps = CS_KSMPS, n;
    int sizefrs, size, post, i, j;
    long spos = p->pos;
    double pos;
    MYFLT *fwin, *bwin, in,
      *prev, *win = (MYFLT *) p->win.auxp;
    MYFLT *outframe;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt;
    int curframe = p->curframe, decim = p->decim;
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
        ft = csound->FTnp2Find(csound,p->knum);
        tab = ft->ftable;
        size = ft->flen;

        if (UNLIKELY((int) ft->nchanls != nchans))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));



        /* spos is the reading position in samples, hsize is hopsize,
           time[n] is current read position in secs
           esr is sampling rate
        */
        spos  = hsize*(long)((time[n])*CS_ESR/hsize);
        sizefrs = size/nchans;
        while(spos > sizefrs) spos -= sizefrs;
        while(spos <= 0)  spos += sizefrs;

        for (j = 0; j < nchans; j++) {
          pos = spos;
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
            while (post < 0) post += size;
            while (post >= size) post -= size;
            if(post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i]; /* window it */
            /* back windo, bwin */
            post = (int) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while(post < 0) post += size;
            while(post >= size) post -= size;
            if(post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            bwin[i] = in * win[i];  /* window it */
            /* increment read pos according to pitch transposition */
            pos += pitch;
          }

          /* take the FFT of both frames
             re-order Nyquist bin from pos 1 to N
          */
          csound->RealFFT2(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = 0.0;
          csound->RealFFT2(csound,  p->fwdsetup, fwin);
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
          csound->RealFFT2(csound, p->invsetup, fwin);
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
        out[n] *= amp*scaling;
      }
      cnt++;
    }

    p->cnt = cnt;
    p->curframe = curframe;
    return OK;

}

static int sinit2(CSOUND *csound, DATASPACE *p)
{
    unsigned int size,i;
    p->nchans = csound->GetOutputArgCnt(p);
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

static int sprocess2(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int  nsmps = CS_KSMPS, n;
    int size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt, curframe = p->curframe;
    int decim = p->decim;
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

      if (cnt == hsize){
        ft = csound->FTnp2Find(csound,p->knum);
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
        if (UNLIKELY((int) ft->nchanls != nchans))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("number of output arguments "
                                       "inconsistent with number of "
                                       "sound file channels"));

        sizefrs = size/nchans;
        while(spos > sizefrs) spos -= sizefrs;
        while(spos <= 0)  spos += sizefrs;


        for (j = 0; j < nchans; j++) {
          pos = spos;
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

            while(post < 0) post += size;
            while(post >= size) post -= size;
            if(post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];

            fwin[i] = in * win[i];

            post = (int) (pos - hsize*pitch);
            post *= nchans;
            post += j;
            while(post < 0) post += size;
            while(post >= size) post -= size;
            //if (post >= 0 && post < size)
            if(post+nchans <  size)
              in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            //else in =  (MYFLT) 0;

            bwin[i] = in * win[i];
            post = (int) pos + hsize;
            post *= nchans;
            post += j;
            while(post < 0) post += size;
            while(post >= size) post -= size;
            if(post+nchans <  size)
              in = tab[post] + frac*(tab[post+nchans] - tab[post]);
            else in = tab[post];
            nwin[i] = in * win[i];
            pos += pitch;
          }

          csound->RealFFT2(csound, p->fwdsetup, bwin);
          bwin[N] = bwin[1];
          bwin[N+1] = FL(0.0);
          csound->RealFFT2(csound, p->fwdsetup,  fwin);
          csound->RealFFT2(csound,  p->fwdsetup, nwin);

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
          csound->RealFFT2(csound, p->invsetup, fwin);

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
static void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps);
/* file-reading version of temposcal */
static int sinit3(CSOUND *csound, DATASPACE *p)
{
    unsigned int size,i;
    char *name;
    SF_INFO sfinfo;
    // open file
    void *fd;
    name = ((STRINGDAT *)p->knum)->data;
    fd  = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                         "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if(sfinfo.samplerate != CS_ESR)
      p->resamp = sfinfo.samplerate/CS_ESR;
    else
      p->resamp = 1;
    p->nchans = sfinfo.channels;

    if(p->OUTOCOUNT != p->nchans)
      return csound->InitError(csound,
              Str("filescal: mismatched channel numbers. %d outputs, %d inputs\n"),
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
    p->indata[1] = p->fdata.auxp + size/2;

    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));

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

void fillbuf(CSOUND *csound, DATASPACE *p, int nsmps){

    sf_count_t sampsread;
    // fill p->curbuf
    sampsread = sf_read_MYFLT(p->sf, p->indata[p->curbuf],
                              nsmps);
    if(sampsread < nsmps)
      memset(p->indata[p->curbuf]+sampsread, 0,
             sizeof(MYFLT)*(nsmps-sampsread));
    // point to the other
    p->curbuf = p->curbuf ? 0 : 1;
}

static int sprocess3(CSOUND *csound, DATASPACE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    MYFLT pitch = *p->kpitch*p->resamp, time = *p->time, lock = *p->klock;
    MYFLT *out, amp =*p->kamp;
    MYFLT *tab,frac,  dbtresh = *p->dbthresh;
    //FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt, sizefrs, nchans = p->nchans;
    int  nsmps = CS_KSMPS, n;
    int size, post, i, j;
    double pos, spos = p->pos;
    MYFLT *fwin, *bwin;
    MYFLT in, *nwin, *prev;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt, curframe = p->curframe;
    int decim = p->decim;
    double tstamp = p->tstamp, incrt = p->incr;

    if(time < 0) /* negative tempo is not possible */
      time = 0.0;
    time *= p->resamp;

    {
      int outnum = csound->GetOutputArgCnt(p);
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

        if (cnt == hsize){
          tab = p->tab;
          size = p->fdata.size/sizeof(MYFLT);

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

          while(spos > sizefrs) {
            spos -= sizefrs;
          }
          while(spos <= 0) {
            spos += sizefrs;
          }

          if (spos > sizefrs/2 && p->curbuf == 0) {
            fillbuf(csound, p, size/2);
          } else if (spos < sizefrs/2 && p->curbuf == 1) {
            fillbuf(csound, p, size/2);
          }

          for (j = 0; j < nchans; j++) {
            pos = spos;
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

              while (post < 0) post += size;
              while (post >= size) post -= size;
              if (post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else {
                in = tab[post];
              }

              fwin[i] = in * win[i];

              post = (int) (pos - hsize*pitch);
              post *= nchans;
              post += j;
              while(post < 0) post += size;
              while(post >= size) post -= size;
              if(post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else in = tab[post];
              bwin[i] = in * win[i];
              post = (int) pos + hsize;
              post *= nchans;
              post += j;
              while(post < 0) post += size;
              while(post >= size) post -= size;
              if(post+nchans <  size)
                in = tab[post] + frac*(tab[post+nchans] - tab[post]);
              else in = tab[post];
              nwin[i] = in * win[i];
              pos += pitch;
            }

            csound->RealFFT2(csound,  p->fwdsetup,  bwin);
            bwin[N] = bwin[1];
            bwin[N+1] = FL(0.0);
            csound->RealFFT2(csound, p->fwdsetup,  fwin);
            csound->RealFFT2(csound,  p->fwdsetup, nwin);

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
            csound->RealFFT2(csound,  p->invsetup,  fwin);

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
          framecnt  = (int *) p->framecount[j].auxp;
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
    //printf("%f s  \n", tstamp/csound->GetSr(csound));
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
    int i;
    float *fout = (float *) p->fout->frame.auxp,
      *fin = (float *) p->fin->frame.auxp;
    int N = p->fin->N;

    if (p->lastframe < p->fin->framecount) {
      memcpy(fout,fin, sizeof(float)*(N+2));

      if (*p->klock) {
        for (i=2; i < N-4; i+=2){
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
  {"mincer", sizeof(DATASPACE), 0, 5, "mm", "akkkkoo",
                                               (SUBR)sinit1, NULL,(SUBR)sprocess1 },
  {"temposcal", sizeof(DATASPACE), 0, 5, "mm", "kkkkkooPOP",
                                               (SUBR)sinit2, NULL,(SUBR)sprocess2 },
  {"filescal", sizeof(DATASPACE), 0, 5, "mm", "kkkSkooPOP",
                                               (SUBR)sinit3, NULL,(SUBR)sprocess3 },
  {"pvslock", sizeof(PVSLOCK), 0, 3, "f", "fk", (SUBR) pvslockset,
         (SUBR) pvslockproc},
};

LINKAGE_BUILTIN(pvlock_localops)
