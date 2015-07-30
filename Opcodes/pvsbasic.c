/*
  pvsbasic.c:
  basic opcodes for transformation of streaming PV signals

  (c) Victor Lazzarini, 2004
  John ffitch, 2007 (slifing form)

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

/* pvsmix */

#include "pvs_ops.h"
#include "pvsbasic.h"
#include "pvfileio.h"
#include <math.h>
#define MAXOUTS 16

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2);

typedef struct _pvsgain {
  OPDS    h;
  PVSDAT  *fout;
  PVSDAT  *fa;
  MYFLT   *kgain;
  uint32  lastframe;
} PVSGAIN;

static int pvsgainset(CSOUND *csound, PVSGAIN *p){

    int32    N = p->fa->N;
    p->fout->sliding = 0;
    if (p->fa->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->fout->frame);
      p->fout->NB = p->fa->NB;
      p->fout->sliding = 1;
    }
    else
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fa->overlap;
    p->fout->winsize = p->fa->winsize;
    p->fout->wintype = p->fa->wintype;
    p->fout->format = p->fa->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                 (p->fout->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvsgain: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsgain(CSOUND *csound, PVSGAIN *p)
{
    int     i;
    int32    framesize;
    float   *fout, *fa;
    MYFLT gain = *p->kgain;

    if (p->fa->sliding) {
      CMPLX * fout, *fa;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int NB = p->fa->NB;
      for (n=0; n<offset; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        for (i = 0; i < NB; i++)
          fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=nsmps-early; n<nsmps; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        for (i = 0; i < NB; i++)
          fout[i].re = fout[i].im = FL(0.0);
      }
      nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        fa = (CMPLX*) p->fa->frame.auxp +NB*n;
        for (i = 0; i < NB; i++) {
          fout[i].re = fa[i].re*gain;
          fout[i].im = fa[i].im;
        }
      }
      return OK;
    }
    fout = (float *) p->fout->frame.auxp;
    fa = (float *) p->fa->frame.auxp;

    framesize = p->fa->N + 2;

    if (p->lastframe < p->fa->framecount) {
      for (i = 0; i < framesize; i += 2){
        fout[i] = fa[i]*gain;
        fout[i+1] = fa[i+1];
      }
        p->fout->framecount = p->fa->framecount;
        p->lastframe = p->fout->framecount;
    }
    return OK;
}



static int pvsinit(CSOUND *csound, PVSINI *p)
{
    int     i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    float   *bframe;
    int32    N = (int32) *p->framesize;

    p->fout->N = N;
    p->fout->overlap = (int32)(*p->olap ? *p->olap : N/4);
    p->fout->winsize = (int32)(*p->winsize ? *p->winsize : N);
    p->fout->wintype = (int32) *p->wintype;
    p->fout->format = (int32) *p->format;
    p->fout->framecount = 1;
    p->fout->sliding = 0;
    if (p->fout->overlap < (int)nsmps || p->fout->overlap <=10) {
      int NB = 1+N/2;
      MYFLT *bframe;
      p->fout->NB = NB;
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size * CS_KSMPS < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * nsmps * sizeof(float),
                         &p->fout->frame);
      p->fout->sliding = 1;
      bframe = (MYFLT *) p->fout->frame.auxp;
      for (n=0; n<nsmps; n++)
        for (i = 0; i < N + 2; i += 2) {
          bframe[i+n*NB] = FL(0.0);
          bframe[i+n*NB + 1] =
            (n<offset || n>nsmps-early ? FL(0.0) :(i >>1) * N * csound->onedsr);
        }
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2)) {
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
        }
        bframe = (float *) p->fout->frame.auxp;
        for (i = 0; i < N + 2; i += 2) {
          //bframe[i] = 0.0f;
          bframe[i + 1] = (i >>1) * N * csound->onedsr;
        }
      }
    p->lastframe = 0;
    return OK;
}


typedef struct {
  OPDS h;
  PVSDAT *fin;
  MYFLT  *file;
  int    pvfile;
  AUXCH  frame;
  AUXCH buf;
  AUXCH dframe;
  CSOUND *csound;
  void *cb;
  int async;
#ifndef __EMSCRIPTEN__
  pthread_t thread;
#endif
  int N;
  uint32 lastframe;
}PVSFWRITE;

void *pvs_io_thread(void *pp);

static int pvsfwrite_destroy(CSOUND *csound, void *pp)
{
  PVSFWRITE *p = (PVSFWRITE *) pp;
#ifndef __EMSCRIPTEN__
  if(p->async){
    p->async = 0;
    pthread_join(p->thread, NULL);
    csound->DestroyCircularBuffer(csound, p->cb);
  }
#endif
  csound->PVOC_CloseFile(csound,p->pvfile);
  return OK;
}

static int pvsfwriteset_(CSOUND *csound, PVSFWRITE *p, int stringname)
{
    int N;
    char fname[MAXNAME];

    if (stringname==0){
      if (ISSTRCOD(*p->file))
        strncpy(fname,get_arg_string(csound, *p->file), MAXNAME-1);
      else csound->strarg2name(csound, fname, p->file, "pvoc.",0);
    }
    else strncpy(fname, ((STRINGDAT *)p->file)->data, MAXNAME-1);

    if (UNLIKELY(p->fin->sliding))
      return csound->InitError(csound,Str("SDFT Not implemented in this case yet"));
    p->pvfile= -1;
    N = p->N = p->fin->N;
    if ((p->pvfile  = csound->PVOC_CreateFile(csound, fname,
                                              p->fin->N,
                                              p->fin->overlap, 1, p->fin->format,
                                              CS_ESR, STYPE_16,
                                              p->fin->wintype, 0.0f, NULL,
                                              p->fin->winsize)) == -1)

      return csound->InitError(csound,
                               Str("pvsfwrite: could not open file %s\n"),
                               fname);
#ifndef __EMSCRIPTEN__
    if(csound->realtime_audio_flag) {
      int bufframes = 16;
      p->csound = csound;
      if (p->frame.auxp == NULL || p->frame.size < sizeof(MYFLT) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT), &p->frame);
      if (p->buf.auxp == NULL || p->buf.size < sizeof(MYFLT) * (N + 2))
               csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT), &p->buf);
      if (p->dframe.auxp == NULL || p->dframe.size < sizeof(float) * (N + 2))
               csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->dframe);
      p->cb = csound->CreateCircularBuffer(csound, (N+2)*sizeof(float)*bufframes,
                                           sizeof(MYFLT));
      pthread_create(&p->thread, NULL, pvs_io_thread, (void *) p);
      p->async = 1;
    } else
#endif
    {
      p->async = 0;
    }
    csound->RegisterDeinitCallback(csound, p, pvsfwrite_destroy);
    p->lastframe = 0;
    return OK;
}

static int pvsfwriteset(CSOUND *csound, PVSFWRITE *p){
  return pvsfwriteset_(csound,p,0);
}

static int pvsfwriteset_S(CSOUND *csound, PVSFWRITE *p){
  return pvsfwriteset_(csound,p,1);
}


void *pvs_io_thread(void *pp){
  PVSFWRITE *p = (PVSFWRITE *) pp;
  CSOUND *csound = p->csound;
  MYFLT  *buf = (MYFLT *) p->buf.auxp;
  float  *frame = (float *) p->dframe.auxp;
  int  *on = &p->async;
  int lc,n, N2=p->N+2;
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  while (*on) {
      lc = csound->ReadCircularBuffer(csound, p->cb, buf, N2);
      if(lc)  {
      for (n=0; n < N2; n++) frame[n] = (float) buf[n];
        csound->PVOC_PutFrames(csound, p->pvfile, frame, 1);
      }
    }
  return NULL;
}


static int pvsfwrite(CSOUND *csound, PVSFWRITE *p)
{
    float *fin = p->fin->frame.auxp;

    if (p->lastframe < p->fin->framecount) {
      int32 framesize = p->fin->N+2,i;
      if(p->async == 0) {
      if (UNLIKELY(!csound->PVOC_PutFrames(csound, p->pvfile, fin, 1)))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("pvsfwrite: could not write data\n"));
      }
      else {
      MYFLT *fout = p->frame.auxp;
      for (i=0;i < framesize; i++) fout[i] = (MYFLT) fin[i];
      csound->WriteCircularBuffer(csound, p->cb, fout, framesize);
      }
      p->lastframe = p->fin->framecount;
    }
    return OK;
}

typedef struct _pvsdiskin {
  OPDS h;
  PVSDAT *fout;
  MYFLT  *file;
  MYFLT  *kspeed;
  MYFLT  *kgain;
  MYFLT *ioff;
  MYFLT *ichn;
  MYFLT *interp;
  double  pos;
  uint32 oldpos;
  int chans, chn;
  int pvfile;
  int scnt;
  uint32  flen;
  AUXCH buffer;
} pvsdiskin;

#define FSIGBUFRAMES 2

static int pvsdiskinset_(CSOUND *csound, pvsdiskin *p, int stringname)
{
    int N;
    WAVEFORMATEX fmt;
    PVOCDATA   pvdata;
    char fname[MAXNAME];

    if (stringname==0){
      if (ISSTRCOD(*p->file))
        strncpy(fname,get_arg_string(csound, *p->file), MAXNAME-1);
      else csound->strarg2name(csound, fname, p->file, "pvoc.",0);
    }
    else strncpy(fname, ((STRINGDAT *)p->file)->data, MAXNAME-1);

    if (UNLIKELY(p->fout->sliding))
      return csound->InitError(csound,
                               Str("SDFT Not implemented in this case yet"));
    if ((p->pvfile  = csound->PVOC_OpenFile(csound, fname,
                                            &pvdata, &fmt)) < 0)
      return csound->InitError(csound,
                               Str("pvsdiskin: could not open file %s\n"),
                               fname);

    N = (pvdata.nAnalysisBins-1)*2;
    p->chans = fmt.nChannels;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

    if (p->buffer.auxp == NULL ||
        p->buffer.size < sizeof(float) * (N + 2) * FSIGBUFRAMES * p->chans)
      csound->AuxAlloc(csound,
                       (N + 2) * sizeof(float) * FSIGBUFRAMES * p->chans,
                       &p->buffer);

    p->flen = csound->PVOC_FrameCount(csound, p->pvfile) - 1;


    p->fout->N = N;
    p->fout->overlap =  pvdata.dwOverlap;
    p->fout->winsize = pvdata.dwWinlen;
    switch ((pv_wtype) pvdata.wWindowType) {
    case PVOC_HAMMING:
      p->fout->wintype = PVS_WIN_HAMMING;
      break;
    case PVOC_HANN:
      p->fout->wintype = PVS_WIN_HANN;
      break;
    case PVOC_KAISER:
      p->fout->wintype = PVS_WIN_KAISER;
      break;
    default:
      p->fout->wintype = PVS_WIN_HAMMING;
      break;
    }
    p->fout->format = pvdata.wAnalFormat;
    p->fout->framecount = 1;
    p->scnt = p->fout->overlap;
    p->pos = *p->ioff * CS_ESR/N;
    p->oldpos = -1;

    p->chn = (int) (*p->ichn <= p->chans ? *p->ichn : p->chans) -1;
    if (p->chn < 0) p->chn = 0;
    return OK;
}

static int pvsdiskinset(CSOUND *csound, pvsdiskin *p){
  return pvsdiskinset_(csound, p, 0);
}

static int pvsdiskinset_S(CSOUND *csound, pvsdiskin *p){
  return pvsdiskinset_(csound, p, 1);
}

static int pvsdiskinproc(CSOUND *csound, pvsdiskin *p)
{
    int overlap = p->fout->overlap, i;
    unsigned int posi;
    double pos = p->pos;
    int32 N = p->fout->N;
    MYFLT frac;
    float *fout = (float *)  p->fout->frame.auxp;
    float *buffer = (float *) p->buffer.auxp;
    float *frame1 = buffer + (N+2)*p->chn;
    float *frame2 = buffer + (N+2)*(p->chans + p->chn);
    float amp = (float) (*p->kgain * csound->e0dbfs);

    if (p->scnt >= overlap) {
      posi = (unsigned int) pos;
      if (posi != p->oldpos) {
        /*
          read new frame
          PVOC_Rewind() is now PVOC_fseek() adapted to work
          as fseek(), using the last argument as
          offset
        */
        while(pos >= p->flen) pos -= p->flen;
        while(pos < 0) pos += p->flen;
        csound->PVOC_fseek(csound,p->pvfile, pos);
        (void)csound->PVOC_GetFrames(csound, p->pvfile, buffer, 2*p->chans);
        p->oldpos = posi = (unsigned int)pos;

      }
      if (*p->interp) {
        /* interpolate */
        frac = pos - posi;
        for (i=0; i < N+2; i+=2) {
          fout[i] = amp*(frame1[i] + frac*(frame2[i] - frame1[i]));
          fout[i+1] =  frame1[i+1] + frac*(frame2[i+1] - frame1[i+1]);
        }
      } else  /* do not */
        for (i=0; i < N+2; i+=2) {
          fout[i] = amp*(frame1[i]);
          fout[i+1] =  frame1[i+1];
        }


      p->pos += (*p->kspeed * p->chans);
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += CS_KSMPS;

    return OK;
}

typedef struct _pvst {
  OPDS h;
  PVSDAT *fout[MAXOUTS];
  MYFLT  *ktime;
  MYFLT  *kamp;
  MYFLT  *kpitch;
  MYFLT  *knum;
  MYFLT  *konset;
  MYFLT  *wrap, *offset;
  MYFLT  *fftsize, *hsize, *dbthresh;
  uint32 scnt;
  int tscale;
  MYFLT accum;
  double pos;
  float factor, fund, rotfac, scale;
  AUXCH bwin[MAXOUTS];
  AUXCH fwin[MAXOUTS], nwin[MAXOUTS];
  AUXCH win;
  int nchans;
  int init;
} PVST;

int pvstanalset(CSOUND *csound, PVST *p)
{

    int i, N, hsize, nChannels;
    N = (*p->fftsize > 0 ? *p->fftsize : 2048);
    hsize = (*p->hsize > 0 ? *p->hsize : 512);
    p->init = 0;
    nChannels = csound->GetOutputArgCnt(p);
    if (UNLIKELY(nChannels < 1 || nChannels > MAXOUTS))
      return csound->InitError(csound, Str("invalid number of output arguments"));
    p->nchans = nChannels;
    for (i=0; i < p->nchans; i++) {
      p->fout[i]->N = N;
      p->fout[i]->overlap = hsize;
      p->fout[i]->wintype = PVS_WIN_HANN;
      p->fout[i]->winsize = N;
      p->fout[i]->framecount = 1;
      if (p->fout[i]->frame.auxp == NULL ||
          p->fout[i]->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout[i]->frame);
      else
        memset(p->fout[i]->frame.auxp, 0, sizeof(float)*(N+2));
      if (p->bwin[i].auxp == NULL ||
          p->bwin[i].size < sizeof(MYFLT) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT), &p->bwin[i]);
      else
        memset(p->bwin[i].auxp, 0, p->bwin[i].size);
      if (p->fwin[i].auxp == NULL ||
          p->fwin[i].size < sizeof(MYFLT) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT), &p->fwin[i]);
      else
        memset(p->fwin[i].auxp, 0, sizeof(MYFLT)*(N+2));
      if (p->nwin[i].auxp == NULL ||
          p->nwin[i].size < sizeof(MYFLT) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT), &p->nwin[i]);
      else
        memset(p->nwin[i].auxp, 0, sizeof(MYFLT)*(N+2));
    }

    if (p->win.auxp == NULL ||
        p->win.size < sizeof(MYFLT) * (N))
      csound->AuxAlloc(csound, (N) * sizeof(MYFLT), &p->win);
    p->scale = 0.0f;
    for (i=0; i < N; i++)
      p->scale += (((MYFLT *)p->win.auxp)[i] = 0.5 - 0.5*cos(i*2*PI/N));
    for (i=0; i < N; i++)
      ((MYFLT *)p->win.auxp)[i] *= 2./p->scale;

    p->rotfac = hsize*TWOPI/N;
    p->factor = CS_ESR/(hsize*TWOPI);
    p->fund = CS_ESR/N;
    p->scnt = p->fout[0]->overlap;
    p->tscale  = 1;
    p->pos =  *p->offset*CS_ESR;
    p->accum = 0.0;
    return OK;
}


int pvstanal(CSOUND *csound, PVST *p)
{
    int hsize = p->fout[0]->overlap, i, k;
    unsigned int j;
    uint32_t sizefrs, nchans = p->nchans;
    int32 N = p->fout[0]->N, post, size;
    double frac, spos = p->pos, pos;
    MYFLT *tab, dbtresh = *p->dbthresh;
    FUNC *ft;
    float *fout;
    MYFLT *bwin, *fwin, *nwin, *win = (MYFLT *) p->win.auxp;
    float amp = (float) (*p->kamp), factor = p->factor, fund = p->fund;
    float pitch = (float) (*p->kpitch), rotfac = p->rotfac;
    MYFLT time = *p->ktime;
    float tmp_real, tmp_im, powrat;

    if ((int)p->scnt >= hsize) {

      /* audio samples are stored in a function table */
      ft = csound->FTnp2Find(csound,p->knum);
      if (ft == NULL){
        csound->PerfError(csound, p->h.insdshead,
                          "could not find table number %d\n", (int) *p->knum);
        return NOTOK;

      }

      tab = ft->ftable;
      size = ft->flen;
      /* nchans = ft->nchanls; */
      /* spos is the reading position in samples, hsize is hopsize,
         time is current read rate
         esr is sampling rate
      */
      if (UNLIKELY(ft->nchanls != (int32)nchans))
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("number of output arguments "
                                     "inconsistent with number of "
                                     "sound file channels"));

      sizefrs = size/nchans;
      if (!*p->wrap && spos >= sizefrs) {
        for (j=0; j < nchans; j++) {
          memset(p->fout[j]->frame.auxp, 0, sizeof(float)*(N+2));
           p->fout[j]->framecount++;
        }
         goto end;
      }

      while (spos >= sizefrs) spos -= sizefrs;
      while (spos < hsize)  spos += (sizefrs + hsize);
      pos = spos;
      for (j=0; j < nchans; j++) {

        fout = (float *)  p->fout[j]->frame.auxp;
        bwin = (MYFLT *) p->bwin[j].auxp;
        fwin = (MYFLT *) p->fwin[j].auxp;
        nwin = (MYFLT *) p->nwin[j].auxp;

        /* this loop fills two frames/windows with samples from table,
           reading is linearly-interpolated,
           frames are separated by 1 hopsize
        */
        for (i=0; i < N; i++) {
          /* front window, fwin */
          MYFLT in;
          post = (int) pos;
          frac = pos  - post;
          post *= nchans;
          post += j;
          while (post >= size ) post -= size;
          while (post < 0) post += size;
          in = tab[post] + frac*(tab[post+nchans] - tab[post]);
          fwin[i] = amp * in * win[i]; /* window it */
          /* back windo, bwin */
          post = (int) (pos - hsize*pitch);
          post *= nchans;
          post += j;
          while (post >= size ) post -= size;
          while (post < 0) post += size;
          in =  tab[post] + frac*(tab[post+nchans] - tab[post]);
          bwin[i] = in * win[i];  /* window it */
          if (*p->konset){
          post = (int) pos + hsize;
          post *= nchans;
          post += j;
          while (post >= size ) post -= size;
          while (post < 0) post += size;
          in =  tab[post];
          nwin[i] = amp * in * win[i];
          }
          /* increment read pos according to pitch transposition */
          pos += pitch;
        }
        /* take the FFT of both frames
           re-order Nyquist bin from pos 1 to N
        */
        csound->RealFFT(csound, bwin, N);
        csound->RealFFT(csound, fwin, N);
        if (*p->konset){
        csound->RealFFT(csound, nwin, N);
        tmp_real = tmp_im = (MYFLT) 1e-20;
        for (i=2; i < N; i++) {
          tmp_real += nwin[i]*nwin[i] + nwin[i+1]*nwin[i+1];
          tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
        }
        powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
        if (powrat > dbtresh) p->tscale=0;
        } else p->tscale=1;

        fwin[N+1] = fwin[1] = 0.0;

        for (i=2,k=1; i < N; i+=2, k++) {
          float bph, fph, dph;
          /* freqs */
          bph = atan2(bwin[i+1],bwin[i]);
          fph = atan2(fwin[i+1],fwin[i]);
          /* pdiff, compensate for rotation */
          dph = fph - bph - rotfac*k;
          while(dph > PI) dph -= TWOPI;
          while(dph < -PI) dph += TWOPI;
          fout[i+1] = dph*factor + k*fund;
          /* mags */
          fout[i] = sqrt(fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1]);
        }

        p->fout[j]->framecount++;
      }
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
    end:
      p->scnt -= hsize;
      p->pos = spos;
    }
    p->scnt += CS_KSMPS;
    return OK;

}

static int pvsfreezeset(CSOUND *csound, PVSFREEZE *p)
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

    p->fout->NB = (N/2)+1;
    p->fout->sliding = p->fin->sliding;
    if (p->fin->sliding) {
      uint32_t nsmps = CS_KSMPS;
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * (N + 2) * nsmps)
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * nsmps,
                         &p->fout->frame);
      if (p->freez.auxp == NULL ||
          p->freez.size < sizeof(MYFLT) * (N + 2) * nsmps)
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * nsmps, &p->freez);
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
        if (p->freez.auxp == NULL || p->freez.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->freez);

        if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                     (p->fout->format == PVS_AMP_PHASE)))
          return csound->InitError(csound, Str("pvsfreeze: signal format "
                                               "must be amp-phase or amp-freq."));
      }
    return OK;
}

static int pvssfreezeprocess(CSOUND *csound, PVSFREEZE *p)
{
    int i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t n, nsmps = CS_KSMPS;
    int NB = p->fin->NB;
    MYFLT freeza = *p->kfra, freezf = *p->kfrf;
    CMPLX *fz = (CMPLX*)p->freez.auxp;

    for (n=0; n<offset; n++) {
      CMPLX *fo = (CMPLX*)p->fout->frame.auxp + n*NB;
      for (i = 0; i < NB; i++) fo[i].re = fo[i].im = FL(0.0);
    }
    for (n=offset; n<nsmps; n++) {
      CMPLX *fo = (CMPLX*)p->fout->frame.auxp + n*NB;
      CMPLX *fi = (CMPLX*)p->fin->frame.auxp + n*NB;
      for (i = 0; i < NB; i++) {
        if (freeza < 1)
          fz[i].re = fi[i].re;
        if (freezf < 1)
          fz[i].im = fi[i].im;
        fo[i] = fz[i];
      }
    }
    return OK;
}

static int pvsfreezeprocess(CSOUND *csound, PVSFREEZE *p)
{
    int     i;
    int32    framesize;
    MYFLT   freeza, freezf;
    float   *fout, *fin, *freez;
    if (p->fin->sliding)
      return pvssfreezeprocess(csound, p);
    freeza = *p->kfra;
    freezf = *p->kfrf;
    fout = (float *) p->fout->frame.auxp;
    fin = (float *) p->fin->frame.auxp;
    freez = (float *) p->freez.auxp;

    framesize = p->fin->N + 2;

    if (p->lastframe < p->fin->framecount) {

      for (i = 0; i < framesize; i += 2) {
        if (freeza < 1)
          freez[i] = fin[i];
        if (freezf < 1)
          freez[i + 1] = fin[i + 1];
        fout[i] = freez[i];
        fout[i + 1] = freez[i + 1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

static int pvsoscset(CSOUND *csound, PVSOSC *p)
{
    int     i;
    int32    N = (int32) *p->framesize;

    p->fout->N = N;
    p->fout->overlap = (int32)(*p->olap ? *p->olap : N/4);
    p->fout->winsize = (int32)(*p->winsize ? *p->winsize : N);
    p->fout->wintype = (int32) *p->wintype;
    p->fout->format = (int32) *p->format;
    p->fout->framecount = 0;
    p->fout->sliding = 0;
    if (p->fout->overlap<(int)CS_KSMPS || p->fout->overlap<=10) {
      return csound->InitError(csound, Str("pvsosc does not work while sliding"));
#ifdef SOME_FINE_DAY
      CMPLX *bframe;
      int NB = 1+N/2;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;

      p->fout->NB = NB;
      p->fout->sliding = 1;
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < CS_KSMPS*sizeof(MYFLT) * (N + 2))
        csound->AuxAlloc(csound,
                         (N + 2) * CS_KSMPS* sizeof(MYFLT), &p->fout->frame);
      else memset(p->fout->frame.auxp,
                  '\0', (N + 2) * CS_KSMPS* sizeof(MYFLT));
      bframe = (CMPLX *)p->fout->frame.auxp;
      for (n=0; n<nsmps; n++)
        for (i = 0; i < NB; i++) {
          bframe[i+NB*n].re = FL(0.0);
          bframe[i+NB*n].im = (n<offset ? FL(0.0) : i * N * csound->onedsr);
        }
      return OK;
#endif
    }
    else
      {
        float   *bframe;
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
        bframe = (float *) p->fout->frame.auxp;
        for (i = 0; i < N + 2; i += 2) {
          //bframe[i] = 0.0f;
          bframe[i + 1] = (i / 2) * N * csound->onedsr;
        }
        p->lastframe = 1;
        p->incr = (MYFLT)CS_KSMPS/p->fout->overlap;
      }
    return OK;
}

static int pvsoscprocess(CSOUND *csound, PVSOSC *p)
{
    int     i, harm, type;
    int32    framesize;
    MYFLT   famp, ffun,w;
    float   *fout;
    double  cfbin,a;
    float   amp, freq;
    int     cbin, k, n;

    famp = *p->ka;
    ffun = *p->kf;
    type = (int)MYFLT2LRND(*p->type);
    fout = (float *) p->fout->frame.auxp;

    framesize = p->fout->N + 2;

    if (p->fout->sliding) {
      CMPLX *fout;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB = p->fout->NB;
      harm = (int)(CS_ESR/(2*ffun));
      if (type==1) famp *= FL(1.456)/POWER((MYFLT)harm, FL(1.0)/FL(2.4));
      else if (type==2) famp *= FL(1.456)/POWER((MYFLT)harm, FL(0.25));
      else if (type==3) famp *= FL(1.456)/POWER((MYFLT)harm, FL(1.0)/FL(160.0));
      else {
        harm = 1;
        famp *= FL(1.456);
      }

      for (n=0; n<nsmps; n++) {
        int m;
        fout = (CMPLX*) p->fout->frame.auxp + n*NB;
        w = CS_ESR/p->fout->N;
        /*         harm = (int)(CS_ESR/(2*ffun)); */
        memset(fout, '\0', NB*sizeof(CMPLX));
        if (n<offset) continue;
        for (m=1; m <= harm; m++) {
          if (type == 3) amp = famp/(harm);
          else amp = (famp/m);
          freq = ffun*m;
          cfbin = freq/w;
          cbin = (int)MYFLT2LRND(cfbin);
          if (cbin != 0)     {
            for (i=cbin-1;i < cbin+3 &&i < NB ; i++) {
              if (i-cfbin == 0) a = 1;
              else a = sin(i-cfbin)/(i-cfbin);
              fout[i].re = amp*a*a*a;
              fout[i].im = freq;
            }
            if (type==2) m++;
          }
        }
      }
      return OK;
    }
    if (p->lastframe > p->fout->framecount) {
      w = CS_ESR/p->fout->N;
      harm = (int)(CS_ESR/(2*ffun));
      if (type==1) famp *= FL(1.456)/pow(harm, FL(1.0)/FL(2.4));
      else if (type==2) famp *= FL(1.456)/POWER(harm, FL(0.25));
      else if (type==3) famp *= FL(1.456)/POWER(harm, FL(1.0)/FL(160.0));
      else {
        harm = 1;
        famp *= FL(1.456);
      }
      memset(fout, 0, sizeof(float)*framesize);
      /* for (i = 0; i < framesize; i ++) fout[i] = 0.f; */

      for (n=1; n <= harm; n++) {
        if (type == 3) amp = famp/(harm);
        else amp = (famp/n);
        freq = ffun*n;
        cfbin = freq/w;
        cbin = (int)MYFLT2LRND(cfbin);
         if (cbin != 0)     {
        for (i=cbin-1;i < cbin+3 &&i < framesize/2 ; i++) {
          k = i<<1;
          if (i-cfbin == 0) a = 1;
          else a = sin(i-cfbin)/(i-cfbin);
          fout[k] = amp*a*a*a;
          fout[k+1] = freq;
        }
        if (type==2) n++;
         }
      }
      p->fout->framecount = p->lastframe;
    }
    p->incr += p->incr;
    if (p->incr > 1) {
      p->incr = (MYFLT)CS_KSMPS/p->fout->overlap;
      p->lastframe++;
    }
    return OK;
}


static int pvsbinset(CSOUND *csound, PVSBIN *p)
{
    p->lastframe = 0;
    return OK;
}

static int pvsbinprocess(CSOUND *csound, PVSBIN *p)
{
    int32    framesize, pos;
    if (p->fin->sliding) {
      CMPLX *fin = (CMPLX *) p->fin->frame.auxp;
      framesize = p->fin->NB;
      pos=*p->kbin;
      if (pos >= 0 && pos < framesize) {
        *p->kamp = (MYFLT)fin[pos].re;
        *p->kfreq = (MYFLT)fin[pos].im;
      }
    }
    else
      {
        float   *fin;
        fin = (float *) p->fin->frame.auxp;
        if (p->lastframe < p->fin->framecount) {
          framesize = p->fin->N + 2;
          pos=*p->kbin*2;
          if (pos >= 0 && pos < framesize) {
            *p->kamp = (MYFLT)fin[pos];
            *p->kfreq = (MYFLT)fin[pos+1];
          }
          p->lastframe = p->fin->framecount;
        }
      }
    return OK;
}

static int pvsbinprocessa(CSOUND *csound, PVSBIN *p)
{
    int32    framesize, pos;
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t k, nsmps = CS_KSMPS;
      CMPLX *fin = (CMPLX *) p->fin->frame.auxp;
      int NB = p->fin->NB;
      pos = *p->kbin;
      if (pos >= 0 && pos < NB) {
        for (k=0; k<offset; k++)  p->kamp[k]  = p->kfreq[k] = FL(0.0);
        for (k=offset; k<nsmps; k++) {
          p->kamp[k]  = (MYFLT)fin[pos+NB*k].re;
          p->kfreq[k] = (MYFLT)fin[pos+NB*k].im;
        }
      }
    }
    else {
      float   *fin;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t k, nsmps = CS_KSMPS;
      fin = (float *) p->fin->frame.auxp;
      if (p->lastframe < p->fin->framecount) {
        framesize = p->fin->N + 2;
        pos=*p->kbin*2;
        if (pos >= 0 && pos < framesize) {
          memset(p->kamp, '\0', offset*sizeof(MYFLT));
          memset(p->kfreq, '\0', offset*sizeof(MYFLT));
          for (k=offset; k<nsmps; k++) {
            p->kamp[k]  = (MYFLT)fin[pos];
            p->kfreq[k] = (MYFLT)fin[pos+1];
          }
          p->lastframe = p->fin->framecount;
        }
      }
    }
    return OK;
}

static int pvsmoothset(CSOUND *csound, PVSMOOTH *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    p->fout->NB = (N/2)+1;
    p->fout->sliding = p->fin->sliding;
    if (p->fin->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->fout->frame);
      if (p->del.auxp == NULL ||
          p->del.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->del);
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
        if (p->del.auxp == NULL || p->del.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->del);
      }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                 (p->fout->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvsmooth: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsmoothprocess(CSOUND *csound, PVSMOOTH *p)
{
    int     i;
    int32    framesize;
    double  ffa, ffr;

    ffa = (double) *p->kfra;
    ffr = (double) *p->kfrf;


    framesize = p->fin->N + 2;

    if (p->fin->sliding) {
      CMPLX *fout, *fin, *del;
      double  costh1, costh2, coef1, coef2;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB = p->fin->NB;
      ffa = ffa < 0.0 ? 0.0 : (ffa > 1.0 ? 1.0 : ffa);
      ffr = ffr < 0.0 ? 0.0 : (ffr > 1.0 ? 1.0 : ffr);
      costh1 = 2.0 - cos(PI * ffa);
      costh2 = 2.0 - cos(PI * ffr);
      coef1 = sqrt(costh1 * costh1 - 1.0) - costh1;
      coef2 = sqrt(costh2 * costh2 - 1.0) - costh2;

      for (n=0; n<offset; n++)
        for (i=0; i<NB; i++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        del = (CMPLX*) p->del.auxp +NB*n;
        fout[i].re = fout[i].im = del[i].re = del[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        fin = (CMPLX*) p->fin->frame.auxp +NB*n;
        del = (CMPLX*) p->del.auxp +NB*n;
        if (IS_ASIG_ARG(p->kfra)) {
          ffa = (double)  p->kfra[n];
          ffa = ffa < 0.0 ? 0.0 : (ffa > 1.0 ? 1.0 : ffa);
          costh1 = 2.0 - cos(PI * ffa);
          coef1 = sqrt(costh1 * costh1 - 1.0) - costh1;
        }
        if (IS_ASIG_ARG(p->kfrf)) {
          ffr = (double)  p->kfrf[n];
          ffr = ffr < 0.0 ? 0.0 : (ffr > 1.0 ? 1.0 : ffr);
          costh2 = 2.0 - cos(PI * ffr);
          coef2 = sqrt(costh2 * costh2 - 1.0) - costh2;
        }
        for (i=0; i<NB; i++) {
          /* amp smoothing */
          fout[i].re = fin[i].re * (1.0 + coef1) - del[i].re * coef1;
          /* freq smoothing */
          fout[i].im = fin[i].im * (1.0 + coef2) - del[i].im * coef1;
          del[i] = fout[i];
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      float   *fout, *fin, *del;
      double  costh1, costh2, coef1, coef2;
      fout = (float *) p->fout->frame.auxp;
      fin = (float *) p->fin->frame.auxp;
      del = (float *) p->del.auxp;

      ffa = ffa < FL(0.0) ? FL(0.0) : (ffa > FL(1.0) ? FL(1.0) : ffa);
      ffr = ffr < FL(0.0) ? FL(0.0) : (ffr > FL(1.0) ? FL(1.0) : ffr);
      costh1 = 2.0 - cos(PI * ffa);
      costh2 = 2.0 - cos(PI * ffr);
      coef1 = sqrt(costh1 * costh1 - 1.0) - costh1;
      coef2 = sqrt(costh2 * costh2 - 1.0) - costh2;

      for (i = 0; i < framesize; i += 2) {
        /* amp smoothing */
        fout[i] = (float) (fin[i] * (1.0 + coef1) - del[i] * coef1);
        /* freq smoothing */
        fout[i + 1] = (float) (fin[i + 1] * (1.0 + coef2) - del[i + 1] * coef1);
        del[i] = fout[i];
        del[i + 1] = fout[i + 1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

static int pvsmixset(CSOUND *csound, PVSMIX *p)
{
    int32    N = p->fa->N;

    /* if (UNLIKELY(p->fa == p->fout || p->fb == p->fout))
       csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));*/
    p->fout->sliding = 0;
    if (p->fa->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->fout->frame);
      p->fout->NB = p->fa->NB;
      p->fout->sliding = 1;
    }
    else
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fa->overlap;
    p->fout->winsize = p->fa->winsize;
    p->fout->wintype = p->fa->wintype;
    p->fout->format = p->fa->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                 (p->fout->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvsmix: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsmix(CSOUND *csound, PVSMIX *p)
{
    int     i;
    int32    framesize;
    int     test;
    float   *fout, *fa, *fb;

    if (UNLIKELY(!fsigs_equal(p->fa, p->fb))) goto err1;
    if (p->fa->sliding) {
      CMPLX * fout, *fa, *fb;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB = p->fa->NB;
      for (n=0; n<offset; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {
        fout = (CMPLX*) p->fout->frame.auxp +NB*n;
        fa = (CMPLX*) p->fa->frame.auxp +NB*n;
        fb = (CMPLX*) p->fb->frame.auxp +NB*n;
        for (i = 0; i < NB; i++) {
          fout[i] = (fa[i].re >= fb[i].re) ? fa[i] : fb[i];
        }
      }
      return OK;
    }
    fout = (float *) p->fout->frame.auxp;
    fa = (float *) p->fa->frame.auxp;
    fb = (float *) p->fb->frame.auxp;

    framesize = p->fa->N + 2;

    if (p->lastframe < p->fa->framecount) {
      for (i = 0; i < framesize; i += 2) {
        test = fa[i] >= fb[i];
        if (test) {
          fout[i] = fa[i];
          fout[i + 1] = fa[i + 1];
        }
        else {
          fout[i] = fb[i];
          fout[i + 1] = fb[i + 1];
        }
      }
      p->fout->framecount =  p->fa->framecount;
      p->lastframe = p->fout->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsmix: formats are different."));
}

/* pvsfilter  */

static int pvsfilterset(CSOUND *csound, PVSFILTER *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout || p->fil == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                 (p->fout->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvsfilter: signal format "
                                           "must be amp-phase or amp-freq."));
    p->fout->sliding = 0;
    if (p->fin->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, sizeof(MYFLT) * CS_KSMPS * (N + 2),
                         &p->fout->frame);
      p->fout->NB = p->fin->NB;
      p->fout->sliding = 1;
    }
    else
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, sizeof(float) * (N + 2), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    return OK;
}

static int pvsfilter(CSOUND *csound, PVSFILTER *p)
{
    int32    i, N = p->fout->N;
    float   g = (float) *p->gain;
    MYFLT   dirgain, kdepth = *p->kdepth;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    float   *fil = (float *) p->fil->frame.auxp;

    if (UNLIKELY(fout == NULL)) goto err1;
    if (UNLIKELY(!fsigs_equal(p->fin, p->fil))) goto err2;

    if (p->fin->sliding) {
      int NB = p->fout->NB;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      CMPLX *fin, *fout, *fil;
      MYFLT g = *p->gain;
      kdepth = kdepth >= FL(0.0) ? (kdepth <= FL(1.0) ? kdepth*g : g) : FL(0.0);
      dirgain = (FL(1.0) - kdepth)*g;
      for (n=0; n<offset; n++) {
        fout = (CMPLX *)p->fout->frame.auxp + NB*n;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {
        fin = (CMPLX *)p->fin->frame.auxp + NB*n;
        fout = (CMPLX *)p->fout->frame.auxp + NB*n;
        fil = (CMPLX *)p->fil->frame.auxp + NB*n;
        if (IS_ASIG_ARG(p->kdepth)) {
          kdepth = p->kdepth[n] >= FL(0.0) ?
            (p->kdepth[n] <= FL(1.0) ? p->kdepth[n]*g : g) : FL(0.0);
          dirgain = (FL(1.0) - kdepth)*g;
        }
        for (i = 0; i < NB; i++) {
          fout[i].re = fin[i].re * (dirgain + fil[i].re * kdepth);
          fout[i].im = fin[i].im;
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : 1) : FL(0.0);
      dirgain = (1 - kdepth);
      for (i = 0; i < N + 2; i += 2) {
        fout[i] = (float) (fin[i] * (dirgain + fil[i] * kdepth))*g;
        fout[i + 1] = fin[i + 1];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsfilter: not initialised"));
 err2:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsfilter: formats are different."));
}

/* pvscale  */
typedef struct _pvscale {
  OPDS    h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  MYFLT   *kscal;
  MYFLT   *keepform;
  MYFLT   *gain;
  MYFLT   *coefs;
  AUXCH   fenv, ceps;
  uint32  lastframe;
} PVSSCALE;

static int pvsscaleset(CSOUND *csound, PVSSCALE *p)
{
  int32    N = p->fin->N, tmp;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    p->fout->NB = p->fin->NB;
    p->fout->sliding = p->fin->sliding;
    if (p->fin->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < CS_KSMPS * sizeof(MYFLT) * (N + 2))
        csound->AuxAlloc(csound, CS_KSMPS * sizeof(MYFLT) * (N + 2),
                         &p->fout->frame);
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
          csound->AuxAlloc(csound, sizeof(float) * (N + 2), &p->fout->frame);
      }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    tmp = N + N%2;
    if (p->ceps.auxp == NULL ||
        p->ceps.size < sizeof(MYFLT) * (tmp+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (tmp + 2), &p->ceps);
    memset(p->ceps.auxp, 0, sizeof(MYFLT)*(tmp+2));
    if (p->fenv.auxp == NULL ||
        p->fenv.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
    memset(p->fenv.auxp, 0, sizeof(MYFLT)*(N+2));
    return OK;
}

void csoundInverseComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);
void csoundComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);

static int pvsscale(CSOUND *csound, PVSSCALE *p)
{
    int     i, chan, N = p->fout->N;
    float   max = 0.0f;
    MYFLT   pscal = FABS(*p->kscal);
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    MYFLT   *fenv = (MYFLT *) p->fenv.auxp;
    MYFLT   *ceps = (MYFLT *) p->ceps.auxp;
    float sr = CS_ESR, binf;
    int coefs = (int) *p->coefs;

    if (UNLIKELY(fout == NULL)) goto err1;

    if (p->fout->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB    = p->fout->NB;
      MYFLT   g = *p->gain;
      for (n=0; n<offset; n++) {
        CMPLX   *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {
        MYFLT    max = FL(0.0);
        CMPLX   *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX   *fout = (CMPLX *) p->fout->frame.auxp + n*NB;

        fout[0] = fin[0];
        fout[NB-1] = fin[NB-1];
        if (IS_ASIG_ARG(p->kscal)) {
          pscal = FABS(p->kscal[n]);
        }
        if (keepform)
          for (i = 1; i < NB-1; i++) {
            max = max < fin[i].re ? fin[i].re : max;
          }

        for (i = 1; i < NB-1; i++) {
          if (keepform == 0 || keepform == 1 || !max)
            fout[i].re = fin[i].re;
          else
            fout[i].re = fin[i].re * (fin[i].re / max);
          fout[i].im = fin[i].im * pscal;
          /* Remove aliases */
          if (fout[i].im>=CS_ESR*0.5 ||
              fout[i].im<= -CS_ESR*0.5)
            fout[i].re=0.0;
        }

        for (i = 1; i < NB; i++) {
          fout[i].re *= g;
        }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {
      int n;
      fout[0] = fin[0];
      fout[N] = fin[N];

      for (i = 2, n=1; i < N; i += 2, n++) {
        fout[i] = 0.0f;
        fout[i + 1] = -1.0f;
        fenv[n] = 0.f;
      }

      if (keepform) {
        int cond = 1;
        for (i=0; i < N; i+=2)
          fenv[i/2] = log(fin[i] > 0.0 ? fin[i] : 1e-20);


        if (keepform > 2) { /* experimental mode 3 */
          int j;
          int w = 5, w2  = w*2;
          for (i=0; i < w; i++) ceps[i] = fenv[i];
          for (i=w; i < N/2-w; i++) {
            ceps[i] = 0.0;
            for (j=-w; j < w; j++)
              ceps[i] += fenv[i+j];
            ceps[i] /= w2;
          }
          for (i=0; i<N/2; i++) {
            fenv[i] = exp(ceps[i]);
            max = max < fenv[i] ? fenv[i] : max;
          }
          if (max)
            for (i=0; i<N; i+=2) {
              fenv[i/2]/=max;
              binf = (i/2)*sr/N;
              if (fenv[i/2] && binf < pscal*sr/2 )
                fin[i] /= fenv[i/2];
            }
        }
        else {  /* new modes 1 & 2 */
          int tmp = N/2;
          tmp = tmp + tmp%2;
          if (coefs < 1) coefs = 80;
          while(cond) {
            cond = 0;
            for (i=0; i < N; i+=2) {
              ceps[i] = fenv[i/2];
              ceps[i+1] = 0.0;
            }
            if (!(N & (N - 1)))
              csound->InverseComplexFFT(csound, ceps, N/2);
            else
              csoundInverseComplexFFTnp2(csound, ceps, tmp);
            for (i=coefs; i < N-coefs; i++) ceps[i] = 0.0;
            if (!(N & (N - 1)))
              csound->ComplexFFT(csound, ceps, N/2);
            else
              csoundComplexFFTnp2(csound, ceps, tmp);
            for (i=0; i < N; i+=2) {
              if (keepform > 1) {
                if (fenv[i/2] < ceps[i])
                  fenv[i/2] = ceps[i];
                if ((log(fin[i]) - ceps[i]) > 0.23) cond = 1;
              }
              else
                {
                  fenv[i/2] = exp(ceps[i]);
                  max = max < fenv[i/2] ? fenv[i/2] : max;
                }
            }
          }
          if (keepform > 1)
            for (i=0; i<N; i+=2) {
              fenv[i/2] = exp(ceps[i]);
              max = max < fenv[i/2] ? fenv[i/2] : max;
            }
          if (max)
            for (i=2; i<N; i+=2) {
              fenv[i/2]/=max;
              binf = (i/2)*sr/N;
              if (fenv[i/2] && binf < pscal*sr/2 )
                fin[i] /= fenv[i/2];
            }
        }
      }
      for (i = 2, chan = 1; i < N; chan++, i += 2) {
        int newchan;
        newchan  = (int) ((chan * pscal)+0.5) << 1;
        if (newchan < N && newchan > 0) {
          fout[newchan] = keepform ? fin[i]*fenv[newchan/2] : fin[i];
          fout[newchan + 1] = (float) (fin[i + 1] * pscal);
        }
      }

      for (i = 2; i < N; i += 2) {
        if (isnan(fout[i])) fout[i] = 0.0f;
        if (fout[i + 1] == -1.0f) {
          fout[i] = 0.f;
        }
        else
          fout[i] *= g;
        binf = (i/2)*sr/N;
        if (fenv[i/2] && binf < pscal*sr/2 )
          fin[i] *= fenv[i/2];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvscale: not initialised"));
}

/* pvshift */

typedef struct _pvshift {
  OPDS    h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  MYFLT   *kshift;
  MYFLT   *lowest;
  MYFLT   *keepform;
  MYFLT   *gain;
  MYFLT   *coefs;
  AUXCH   fenv, ceps;
  uint32  lastframe;
} PVSSHIFT;

static int pvsshiftset(CSOUND *csound, PVSSHIFT *p)
{
    int    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    if (p->fin->sliding) {
      if (p->fout->frame.auxp==NULL ||
          CS_KSMPS*(N+2)*sizeof(MYFLT) > (unsigned int)p->fout->frame.size)
        csound->AuxAlloc(csound, CS_KSMPS*(N+2)*sizeof(MYFLT),&p->fout->frame);
      else memset(p->fout->frame.auxp, 0, CS_KSMPS*(N+2)*sizeof(MYFLT));
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
        else memset(p->fout->frame.auxp, 0, (N+2)*sizeof(float));
      }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = p->fin->NB;

    if (p->ceps.auxp == NULL ||
        p->ceps.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->ceps);
    else
      memset(p->ceps.auxp, 0, sizeof(MYFLT)*(N+2));
    if (p->fenv.auxp == NULL ||
        p->fenv.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
    else
      memset(p->fenv.auxp, 0, sizeof(MYFLT)*(N+2));

    return OK;
}

static int pvsshift(CSOUND *csound, PVSSHIFT *p)
{
    int    i, chan, newchan, N = p->fout->N;
    MYFLT   pshift = (MYFLT) *p->kshift;
    int     lowest = abs((int) (*p->lowest * N * csound->onedsr));
    float   max = 0.0f;
    int     cshift = (int) (pshift * N * csound->onedsr);
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    MYFLT   *fenv = (MYFLT *) p->fenv.auxp;
    MYFLT   *ceps = (MYFLT *) p->ceps.auxp;
    float sr = CS_ESR, binf;
    int coefs = (int) *p->coefs;

    if (UNLIKELY(fout == NULL)) goto err1;
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB  = p->fout->NB;
      MYFLT g = *p->gain;
      lowest = lowest ? (lowest > NB ? NB : lowest) : 1;

      for (n=0; n<offset; n++) {
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {  MYFLT max = FL(0.0);
        CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        fout[0] = fin[0];
        fout[NB-1] = fin[NB-1];
        if (IS_ASIG_ARG(p->kshift)) {
          pshift = (p->kshift)[n];
        }
        for (i = 1; i < NB-1; i++) {
          if (keepform && (max < fin[i].re)) max = fin[i].re;
          if (i < lowest) {
            fout[i] = fin[i];
          }
        }
        for (i = lowest; i < NB; i++) {
          if (keepform == 0 || keepform == 1 || !max)
            fout[i].re = fin[i].re;
          else
            fout[i].re = fin[i].re * (fin[i].re / max);
          fout[i].im = (fin[i].im + pshift);
          /* Remove aliases */
          if (fout[i].im>=CS_ESR*0.5 ||
              fout[i].im<= -CS_ESR*0.5)
            fout[i].re = 0.0;
        }
        if (g!=1.0f)
          for (i = lowest; i < NB; i++) {
            fout[i].re *= g;
          }
      }
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {

      lowest = lowest ? (lowest > N / 2 ? N / 2 : lowest << 1) : 2;

      fout[0] = fin[0];
      fout[N] = fin[N];

      for (i = 2; i < N; i += 2) {
        fenv[i/2] = 0.0;
        if (i < lowest) {
          fout[i] = fin[i];
          fout[i + 1] = fin[i + 1];
        }
        else {
          fout[i] = 0.0f;
          fout[i + 1] = -1.0f;
        }
      }
      if (keepform) { /* new modes 1 & 2 */
        int cond = 1;
        int tmp = N/2;
        tmp = tmp + tmp%2;
        for (i=0; i < N; i+=2)
          fenv[i/2] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
        if (coefs < 1) coefs = 80;
        while(cond) {
          cond = 0;
          for (i=0; i < N; i+=2) {
            ceps[i] = fenv[i/2];
            ceps[i+1] = 0.0;
          }
          if (!(N & (N - 1)))
            csound->InverseComplexFFT(csound, ceps, N/2);
          else
            csoundInverseComplexFFTnp2(csound, ceps, tmp);
          for (i=coefs; i < N-coefs; i++) ceps[i] = 0.0;
          if (!(N & (N - 1)))
            csound->ComplexFFT(csound, ceps, N/2);
          else
            csoundComplexFFTnp2(csound, ceps, tmp);
          for (i=0; i < N; i+=2) {
            if (keepform > 1) {
              if (fenv[i/2] < ceps[i])
                fenv[i/2] = ceps[i];
              if ((log(fin[i]) - ceps[i]) > 0.23) cond = 1;
            }
            else
              {
                fenv[i/2] = exp(ceps[i]);
                max = max < fenv[i/2] ? fenv[i/2] : max;
              }
          }
        }
        if (keepform > 1)
          for (i=0; i<N/2; i++) {
            fenv[i] = exp(fenv[i]);
            max = max < fenv[i] ? fenv[i] : max;
          }
        if (max)
          for (i=lowest; i<N; i+=2) {
            fenv[i/2]/=max;
            binf = (i/2)*sr/N;
            if (fenv[i/2] && binf < sr/2+pshift )
              fin[i] /= fenv[i/2];
          }
      }

      for (i = lowest, chan = lowest >> 1; i < N; chan++, i += 2) {

        newchan = (chan + cshift) << 1;

        if (newchan < N && newchan > lowest) {
          fout[newchan] = keepform ?
            fin[i] * fenv[newchan/2] : fin[i];
          fout[newchan + 1] = (float) (fin[i + 1] + pshift);
        }
      }

      for (i = lowest; i < N; i += 2) {
        if (fout[i + 1] == -1.0f)
          fout[i] = 0.0f;
        else
          fout[i] *= g;
        binf = (i/2)*sr/N;
        if (fenv[i/2] && binf < sr/2+pshift ) fin[i] *= fenv[i/2];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvshift: not initialised"));
}

/* pvswarp  */
typedef struct _pvswarp {
  OPDS    h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  MYFLT   *kscal;
  MYFLT   *kshift;
  MYFLT   *klowest;
  MYFLT   *keepform;
  MYFLT   *gain;
  MYFLT   *coefs;
  AUXCH   fenv, ceps;
  uint32  lastframe;
} PVSWARP;

static int pvswarpset(CSOUND *csound, PVSWARP *p)
{
    int32    N = p->fin->N;

    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
        csound->AuxAlloc(csound, sizeof(float) * (N + 2), &p->fout->frame);
    }
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (p->ceps.auxp == NULL ||
        p->ceps.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->ceps);
    else
      memset(p->ceps.auxp, 0, sizeof(MYFLT)*(N+2));
    if (p->fenv.auxp == NULL ||
        p->fenv.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
    else
      memset(p->fenv.auxp, 0, sizeof(MYFLT)*(N+2));

    return OK;
}




static int pvswarp(CSOUND *csound, PVSWARP *p)
{
    int     i, chan, N = p->fout->N;
    float   max = 0.0f;
    MYFLT   pscal = FABS(*p->kscal);
    MYFLT   pshift = (*p->kshift);
    int     cshift = (int) (pshift * N * csound->onedsr);
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    MYFLT   *fenv = (MYFLT *) p->fenv.auxp;
    MYFLT   *ceps = (MYFLT *) p->ceps.auxp;
    float sr = CS_ESR, binf;
    int lowest =  abs((int) (*p->klowest * N * csound->onedsr));;
    int coefs = (int) *p->coefs;

    lowest = lowest ? (lowest > N / 2 ? N / 2 : lowest << 1) : 2;

    if (UNLIKELY(fout == NULL)) goto err1;

    if (p->lastframe < p->fin->framecount) {
      int n;
      fout[0] = fin[0];
      fout[N] = fin[N];

      for (i = 2, n=1; i < N; i += 2, n++) {
        fout[i] = 0.0f;
        fout[i + 1] = -1.0f;
        fenv[n] = 0.f;
      }

      {
        int cond = 1;
        for (i=0; i < N; i+=2) {
          fenv[i/2] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
        }
        if (keepform > 2) { /* experimental mode 3 */
          int j;
          int w = 5;
          for (i=0; i < w; i++) ceps[i] = fenv[i];
          for (i=w; i < N/2-w; i++) {
            ceps[i] = 0.0;
            for (j=-w; j < w; j++)
              ceps[i] += fenv[i+j];
            ceps[i]  /= 2*w;
          }
          for (i=0; i<N/2; i++) {
            fenv[i] = exp(ceps[i]);
            max = max < fenv[i] ? fenv[i] : max;
          }
          if (max)
            for (i=lowest; i<N; i+=2) {
              fenv[i/2]/=max;
              binf = (i/2)*sr/N;
              if (fenv[i/2] && binf < pscal*sr/2+pshift )
                fin[i] /= fenv[i/2];
            }
        }
        else {  /* new modes 1 & 2 */
           int tmp = N/2;
          tmp = tmp + tmp%2;
          if (coefs < 1) coefs = 80;
          while(cond) {
            cond = 0;
            for (i=0; i < N; i+=2) {
              ceps[i] = fenv[i/2];
              ceps[i+1] = 0.0;
            }
            if (!(N & (N - 1)))
              csound->InverseComplexFFT(csound, ceps, N/2);
            else
              csoundInverseComplexFFTnp2(csound, ceps, tmp);
            for (i=coefs; i < N-coefs; i++) ceps[i] = 0.0;
            if (!(N & (N - 1)))
              csound->ComplexFFT(csound, ceps, N/2);
            else
              csoundComplexFFTnp2(csound, ceps, tmp);
            for (i=0; i < N; i+=2) {
              if (keepform > 1) {
                if (fenv[i/2] < ceps[i])
                  fenv[i/2] = ceps[i];
                if ((log(fin[i]) - ceps[i]) > 0.23) cond = 1;
              }
              else
                {
                  fenv[i/2] = exp(ceps[i]);
                  max = max < fenv[i/2] ? fenv[i/2] : max;
                }
            }
          }
          if (keepform > 1)
            for (i=0; i<N; i+=2) {
              fenv[i/2] = exp(ceps[i]);
              max = max < fenv[i/2] ? fenv[i/2] : max;
            }
          if (max)
            for (i=lowest; i<N; i+=2) {
              fenv[i/2]/=max;
              binf = (i/2)*sr/N;
              if (fenv[i/2] && binf < pscal*sr/2+pshift )
                fin[i] /= fenv[i/2];
            }
        }
      }
      for (i = 2, chan = 1; i < N; chan++, i += 2) {
        int newchan;
        newchan  = (int) ((chan * pscal + cshift)+0.5) << 1;
        if (i >= lowest) {
          if (newchan < N && newchan > 0)
            fout[newchan] = fin[newchan]*fenv[i/2];
        } else fout[i] = fin[i];
        fout[i + 1] = fin[i + 1];
      }

      for (i = lowest; i < N; i += 2) {
        if (isnan(fout[i])) fout[i] = 0.0f;
        else fout[i] *= g;
        binf = (i/2)*sr/N;
        if (fenv[i/2] && binf < pscal*sr/2+pshift )
          fin[i] *= fenv[i/2];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvswarp: not initialised"));
}




/* pvsblur */

static int pvsblurset(CSOUND *csound, PVSBLUR *p)
{
    float   *delay;
    int32    N = p->fin->N, i, j;
    int     olap = p->fin->overlap;
    int     delayframes, framesize = N + 2;
    if (UNLIKELY(p->fin == p->fout))
      csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
    if (p->fin->sliding) {
      csound->InitError(csound, Str("pvsblur does not work sliding yet"));
      delayframes = (int) (FL(0.5) + *p->maxdel * CS_ESR);
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * CS_KSMPS * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->fout->frame);

      if (p->delframes.auxp == NULL ||
          p->delframes.size < (N + 2) * sizeof(MYFLT) * CS_KSMPS * delayframes)
        csound->AuxAlloc(csound,
                         (N + 2) * sizeof(MYFLT) * CS_KSMPS * delayframes,
                         &p->delframes);
    }
    else
      {
        p->frpsec = CS_ESR / olap;

        delayframes = (int) (*p->maxdel * p->frpsec);

        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

        if (p->delframes.auxp == NULL ||
          p->delframes.size < (N + 2) * sizeof(float) * CS_KSMPS * delayframes)
          csound->AuxAlloc(csound, (N + 2) * sizeof(float) * delayframes,
                           &p->delframes);
      }
    delay = (float *) p->delframes.auxp;

    for (j = 0; j < framesize * delayframes; j += framesize)
      for (i = 0; i < N + 2; i += 2) {
        delay[i + j] = 0.0f;
        delay[i + j + 1] = i * CS_ESR / N;
      }

    p->fout->N = N;
    p->fout->overlap = olap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->count = 0;
    p->fout->sliding = p->fin->sliding;
    p->fout->NB = p->fin->NB;
    return OK;
}

static int pvsblur(CSOUND *csound, PVSBLUR *p)
{
    int32    j, i, N = p->fout->N, first, framesize = N + 2;
    int32    countr = p->count;
    double  amp = 0.0, freq = 0.0;
    int     delayframes = (int) (*p->kdel * p->frpsec);
    int     kdel = delayframes * framesize;
    int     mdel = (int) (*p->maxdel * p->frpsec) * framesize;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    float   *delay = (float *) p->delframes.auxp;

    if (UNLIKELY(fout == NULL || delay == NULL)) goto err1;

    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t n, nsmps = CS_KSMPS;
      int NB = p->fin->NB;
      kdel = kdel >= 0 ? (kdel < mdel ? kdel : mdel - framesize) : 0;
      for (n=0; n<offset; n++) {
        CMPLX   *fout = (CMPLX *) p->fout->frame.auxp +NB*n;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=offset; n<nsmps; n++) {
        CMPLX   *fin = (CMPLX *) p->fin->frame.auxp +NB*n;
        CMPLX   *fout = (CMPLX *) p->fout->frame.auxp +NB*n;
        CMPLX   *delay = (CMPLX *) p->delframes.auxp +NB*n;

        for (i = 0; i < NB; i++) {
          delay[countr + i] = fin[i];
          if (kdel) {
            if ((first = countr - kdel) < 0)
              first += mdel;

            for (j = first; j != countr; j = (j + framesize) % mdel) {
              amp += delay[j + i].re;
              freq += delay[j + i].im;
            }

            fout[i].re = (MYFLT) (amp / delayframes);
            fout[i].im = (MYFLT) (freq / delayframes);
            amp = freq = FL(0.0);
          }
          else {
            fout[i] = fin[i];
          }
        }
      }
      countr += (N + 2);
      p->count = countr < mdel ? countr : 0;
      return OK;
    }
    if (p->lastframe < p->fin->framecount) {

      kdel = kdel >= 0 ? (kdel < mdel ? kdel : mdel - framesize) : 0;

      for (i = 0; i < N + 2; i += 2) {

        delay[countr + i] = fin[i];
        delay[countr + i + 1] = fin[i + 1];

        if (kdel) {

          if ((first = countr - kdel) < 0)
            first += mdel;

          for (j = first; j != countr; j = (j + framesize) % mdel) {
            amp += delay[j + i];
            freq += delay[j + i + 1];
          }

          fout[i] = (float) (amp / delayframes);
          fout[i + 1] = (float) (freq / delayframes);
          amp = freq = 0.;
        }
        else {
          fout[i] = fin[i];
          fout[i + 1] = fin[i + 1];
        }
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
      countr += (N + 2);
      p->count = countr < mdel ? countr : 0;
    }

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsblur: not initialised"));
}

/* pvstencil  */

static int pvstencilset(CSOUND *csound, PVSTENCIL *p)
{
    int32    N = p->fin->N;
    uint32_t i;
    int32    chans = N / 2 + 1;
    MYFLT   *ftable;

    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    p->fout->NB = chans;
    if (p->fin->sliding) {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(MYFLT) * (N + 2) * CS_KSMPS)
        csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * CS_KSMPS,
                         &p->fout->frame);
      p->fout->sliding = 1;
    }
    else
      {
        if (p->fout->frame.auxp == NULL ||
            p->fout->frame.size < sizeof(float) * (N + 2))
          csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

        if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                     (p->fout->format == PVS_AMP_PHASE)))
          return csound->InitError(csound, Str("pvstencil: signal format "
                                               "must be amp-phase or amp-freq."));
      }
    p->func = csound->FTnp2Find(csound, p->ifn);
    if (p->func == NULL)
      return OK;

    if (UNLIKELY(p->func->flen + 1 < (unsigned int)chans))
      return csound->InitError(csound, Str("pvstencil: ftable needs to equal "
                                           "the number of bins"));

    ftable = p->func->ftable;
    for (i = 0; i < p->func->flen + 1; i++)
      if (ftable[i] < FL(0.0))
        ftable[i] = FL(0.0);

    return OK;
}

static int pvstencil(CSOUND *csound, PVSTENCIL *p)
{
    MYFLT   *ftable;
    if (p->fin->sliding) {
      MYFLT g = FABS(*p->kgain);
      MYFLT masklevel = FABS(*p->klevel);
      int NB = p->fin->NB, i;
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      p->fout->NB = NB;
      p->fout->N = p->fin->N;
      p->fout->format = p->fin->format;
      p->fout->wintype = p->fin->wintype;
      ftable = p->func->ftable;
      for (n=0; n<offset; n++) {
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      for (n=nsmps-early; n<nsmps; n++) {
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        for (i = 0; i < NB; i++) fout[i].re = fout[i].im = FL(0.0);
      }
      nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
        CMPLX *fin  = (CMPLX *) p->fin->frame.auxp  + n*NB;
        for (i = 0; i < NB; i++) {
          if (fin[i].re > ftable[i] * masklevel)
            fout[i].re = fin[i].re;   /* Just copy */
          else {
            fout[i].re = fin[i].re * g; /* or apply gain */
          }
          fout[i].im = fin[i].im * g;
        }
      }
    }
    else
      {
        int32    framesize, i, j;
        int     test;
        float   *fout, *fin;
        float   g = fabsf((float)*p->kgain);
        float   masklevel = fabsf((float)*p->klevel);

        fout = (float *) p->fout->frame.auxp;
        fin = (float *) p->fin->frame.auxp;
        ftable = p->func->ftable;

        framesize = p->fin->N + 2;

        if (UNLIKELY(fout == NULL)) goto err1;

        if (p->lastframe < p->fin->framecount) {

          for (i = 0, j = 0; i < framesize; i += 2, j++) {
            test = fin[i] > ftable[j] * masklevel;
            if (test)
              fout[i] = fin[i];
            else
              fout[i] = fin[i] * g;

            fout[i + 1] = fin[i + 1];
          }
          p->fout->framecount = p->lastframe = p->fin->framecount;
        }
      }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvstencil: not initialised"));
}

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
    if (
        (f1->sliding == f2->sliding) &&
        (f1->overlap == f2->overlap) &&
        (f1->winsize == f2->winsize) &&
        (f1->wintype == f2->wintype) &&     /* harsh, maybe... */
        (f1->N == f2->N) &&
        (f1->format == f2->format))

      return 1;
    return 0;
}


typedef struct _pvsenvw {
  OPDS    h;
  MYFLT  *kflag;
  PVSDAT  *fin;
  MYFLT   *ftab;
  MYFLT   *keepform;
  MYFLT   *gain;
  MYFLT   *coefs;
  AUXCH   fenv, ceps;
  uint32  lastframe;
} PVSENVW;

static int pvsenvwset(CSOUND *csound, PVSENVW *p)
{
    int32    N = p->fin->N;


    p->lastframe = 0;

    if (p->ceps.auxp == NULL ||
        p->ceps.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->ceps);
    else
      memset(p->ceps.auxp, 0, sizeof(MYFLT)*(N+2));
    if (p->fenv.auxp == NULL ||
        p->fenv.size < sizeof(MYFLT) * (N+2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
    else
      memset(p->fenv.auxp, 0, sizeof(MYFLT)*(N+2));

    return OK;
}

static int pvsenvw(CSOUND *csound, PVSENVW *p)
{
    int     i, N = p->fin->N;
    float   max = 0.0f;
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    MYFLT   *fenv = (MYFLT *) p->fenv.auxp;
    MYFLT   *ceps = (MYFLT *) p->ceps.auxp;
    int coefs = (int) *p->coefs;
    FUNC  *ft = csound->FTnp2Find(csound, p->ftab);
    int size;
    MYFLT *ftab;

    if (ft == NULL) {
      csound->PerfError(csound, p->h.insdshead,
                        Str("could not find table number %d\n"), (int) *p->ftab);
      return NOTOK;
    }
    size = ft->flen;
    ftab = ft->ftable;

    *p->kflag = 0.0;
    if (p->lastframe < p->fin->framecount) {
      {
        int cond = 1;
        for (i=0; i < N; i+=2) {
          fenv[i/2] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
        }
        if (keepform > 2) { /* experimental mode 3 */
          int j;
          int w = 5;
          for (i=0; i < w; i++) ceps[i] = fenv[i];
          for (i=w; i < N/2-w; i++) {
            ceps[i] = 0.0;
            for (j=-w; j < w; j++)
              ceps[i] += fenv[i+j];
            ceps[i]  /= 2*w;
          }
          for (i=0; i<N/2; i++) {
            fenv[i] = exp(ceps[i]);
            max = max < fenv[i] ? fenv[i] : max;
          }
          /* if (max)
             for (i=0; i<N; i+=2) {
             fenv[i/2]/=max;
             }*/
        }
        else {  /* new modes 1 & 2 */
          int tmp = N/2;
          tmp = tmp + tmp%2;
          if (coefs < 1) coefs = 80;
          while(cond) {
            cond = 0;
            for (i=0; i < N; i+=2) {
              ceps[i] = fenv[i/2];
              ceps[i+1] = 0.0;
            }
            if (!(N & (N - 1)))
              csound->InverseComplexFFT(csound, ceps, N/2);
            else
              csoundInverseComplexFFTnp2(csound, ceps, tmp);
            for (i=coefs; i < N-coefs; i++) ceps[i] = 0.0;
            if (!(N & (N - 1)))
              csound->ComplexFFT(csound, ceps, N/2);
            else
              csoundComplexFFTnp2(csound, ceps, tmp);
            for (i=0; i < N; i+=2) {
              if (keepform > 1) {
                if (fenv[i/2] < ceps[i])
                  fenv[i/2] = ceps[i];
                if ((log(fin[i]) - ceps[i]) > 0.23) cond = 1;
              }
              else
                {
                  fenv[i/2] = exp(ceps[i]);
                  max = max < fenv[i/2] ? fenv[i/2] : max;
                }
            }
          }
          if (keepform > 1)
            for (i=0; i<N; i+=2) {
              fenv[i/2] = exp(ceps[i]);
              max = max < fenv[i/2] ? fenv[i/2] : max;
            }
          /* if (max)
             for (i=0; i<N/2; i++) fenv[i]/=max; */
        }
      }
      for (i = 0; i < N/2 || i < size; i++) ftab[i] = fenv[i]*g;
      p->lastframe = p->fin->framecount;
      *p->kflag = 1.0;
    }
    return OK;

}

typedef struct pvs2tab_t {
    OPDS h;
    MYFLT *framecount;
    ARRAYDAT *ans;
    PVSDAT *fsig;
} PVS2TAB_T;

int pvs2tab_init(CSOUND *csound, PVS2TAB_T *p)
{
    if (UNLIKELY(!(p->fsig->format == PVS_AMP_FREQ) ||
                 (p->fsig->format == PVS_AMP_PHASE)))
      return csound->InitError(csound, Str("pvs2tab: signal format "
                                           "must be amp-phase or amp-freq."));
    if (LIKELY(p->ans->data)) return OK;
    return csound->InitError(csound, Str("array-variable not initialised"));
}

int  pvs2tab(CSOUND *csound, PVS2TAB_T *p){

  int size = p->ans->sizes[0], N = p->fsig->N, i;
  float *fsig = (float *) p->fsig->frame.auxp;
  for(i = 0; i < size && i < N+2; i++)
      p->ans->data[i] = (MYFLT) fsig[i];
  *p->framecount = (MYFLT) p->fsig->framecount;
  return OK;
}

typedef struct tab2pvs_t {
    OPDS h;
    PVSDAT *fout;
    ARRAYDAT *in;
    MYFLT  *olap, *winsize, *wintype, *format;
    uint32 ktime;
    uint32  lastframe;
} TAB2PVS_T;

int tab2pvs_init(CSOUND *csound, TAB2PVS_T *p)
{
    if (LIKELY(p->in->data)){
      int N;
      p->fout->N = N = p->in->sizes[0] - 2;
      p->fout->overlap = (int32)(*p->olap ? *p->olap : N/4);
      p->fout->winsize = (int32)(*p->winsize ? *p->winsize : N);
      p->fout->wintype = (int32) *p->wintype;
      p->fout->format = 0;
      p->fout->framecount = 1;
      p->lastframe = 0;
      p->ktime = 0;
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2)) {
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
      }
      else
        memset(p->fout->frame.auxp, 0, sizeof(float)*(N+2));
      return OK;
    }
    else return csound->InitError(csound, Str("array-variable not initialised"));
}

int  tab2pvs(CSOUND *csound, TAB2PVS_T *p)
{
    int size = p->in->sizes[0], i;
    float *fout = (float *) p->fout->frame.auxp;

    p->ktime += CS_KSMPS;
    if(p->ktime > (uint32) p->fout->overlap) {
       p->fout->framecount++;
       p->ktime -= p->fout->overlap;
    }

    if (p->lastframe < p->fout->framecount){
      for (i = 0; i < size; i++){
        fout[i] = (float) p->in->data[i];
      }
      p->lastframe = p->fout->framecount;
    }

    return OK;
}




static OENTRY localops[] = {
  {"pvsfwrite", sizeof(PVSFWRITE),0, 3, "", "fS", (SUBR) pvsfwriteset_S,
   (SUBR) pvsfwrite},
  {"pvsfwrite.i", sizeof(PVSFWRITE),0, 3, "", "fi", (SUBR) pvsfwriteset,
   (SUBR) pvsfwrite},
  {"pvsfilter", sizeof(PVSFILTER),0, 3, "f", "ffxp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
  {"pvscale", sizeof(PVSSCALE),0, 3, "f", "fxOPO", (SUBR) pvsscaleset,
   (SUBR) pvsscale},
  {"pvshift", sizeof(PVSSHIFT),0, 3, "f", "fxkOPO", (SUBR) pvsshiftset,
   (SUBR) pvsshift},
  {"pvsfilter", sizeof(PVSFILTER),0, 3, "f", "fffp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
  {"pvscale", sizeof(PVSSCALE),0, 3, "f", "fkOPO",
              (SUBR) pvsscaleset, (SUBR) pvsscale},
  {"pvshift", sizeof(PVSSHIFT),0, 3, "f", "fkkOPO", (SUBR) pvsshiftset,
   (SUBR) pvsshift},
  {"pvsmix", sizeof(PVSMIX),0, 3, "f", "ff", (SUBR) pvsmixset, (SUBR)pvsmix, NULL},
  {"pvsfilter", sizeof(PVSFILTER),0, 3, "f", "ffxp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
  {"pvsblur", sizeof(PVSBLUR),0, 3, "f", "fki", (SUBR) pvsblurset, (SUBR) pvsblur,
   NULL},
  {"pvstencil", sizeof(PVSTENCIL), TR, 3, "f", "fkki", (SUBR) pvstencilset,
   (SUBR) pvstencil},
  {"pvsinit", sizeof(PVSINI),0, 1, "f", "ioopo", (SUBR) pvsinit, NULL, NULL},
  {"pvsbin", sizeof(PVSBIN),0, 3, "ss", "fk", (SUBR) pvsbinset,
   (SUBR) pvsbinprocess, (SUBR) pvsbinprocessa},
  {"pvsfreeze", sizeof(PVSFREEZE),0, 3, "f", "fkk", (SUBR) pvsfreezeset,
   (SUBR) pvsfreezeprocess, NULL},
  {"pvsmooth", sizeof(PVSFREEZE),0, 3, "f", "fxx", (SUBR) pvsmoothset,
   (SUBR) pvsmoothprocess, NULL},
  {"pvsosc", sizeof(PVSOSC),0, 3, "f", "kkkioopo", (SUBR) pvsoscset,
   (SUBR) pvsoscprocess, NULL},
  {"pvsdiskin", sizeof(pvsdiskin),0, 3, "f", "SkkopP",(SUBR) pvsdiskinset_S,
   (SUBR) pvsdiskinproc, NULL},
  {"pvsdiskin.i", sizeof(pvsdiskin),0, 3, "f", "ikkopP",(SUBR) pvsdiskinset,
   (SUBR) pvsdiskinproc, NULL},
  {"pvstanal", sizeof(PVST),0, 3, "FFFFFFFFFFFFFFFF", "kkkkPPoooP",
   (SUBR) pvstanalset, (SUBR) pvstanal, NULL},
  {"pvswarp", sizeof(PVSWARP),0, 3, "f", "fkkOPPO",
                               (SUBR) pvswarpset, (SUBR) pvswarp},
  {"pvsenvftw", sizeof(PVSENVW),0, 3, "k", "fkPPO",
                               (SUBR) pvsenvwset, (SUBR) pvsenvw},
  {"pvsgain", sizeof(PVSGAIN), 0,3, "f", "fk",
                               (SUBR) pvsgainset, (SUBR) pvsgain, NULL},
  {"pvs2tab", sizeof(PVS2TAB_T), 0,3, "k", "k[]f",
                               (SUBR) pvs2tab_init, (SUBR) pvs2tab, NULL},
  {"tab2pvs", sizeof(TAB2PVS_T), 0, 3, "f", "k[]oop", (SUBR) tab2pvs_init,
                                                     (SUBR) tab2pvs, NULL},
  {"pvs2array", sizeof(PVS2TAB_T), 0,3, "k", "k[]f",
                               (SUBR) pvs2tab_init, (SUBR) pvs2tab, NULL},
  {"pvsfromarray", sizeof(TAB2PVS_T), 0, 3, "f", "k[]oop",
                               (SUBR) tab2pvs_init, (SUBR) tab2pvs, NULL}
};

int pvsbasic_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int) (sizeof(localops) / sizeof(OENTRY)));
}
