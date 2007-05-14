/*
    pvsbasic.c:
    basic opcodes for transformation of streaming PV signals

    (c) Victor Lazzarini, 2004

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

#include "csdl.h"
#include "pvsbasic.h"
#include "pvfileio.h"
#include <math.h>

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2);

static int pvsinit(CSOUND *csound, PVSINI *p)
{
    int     i;
    float   *bframe;
    long    N = (long) *p->framesize;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = (long)(*p->olap ? *p->olap : N/4);
    p->fout->winsize = (long)(*p->winsize ? *p->winsize : N);
    p->fout->wintype = (long) *p->wintype;
    p->fout->format = (long) *p->format;
    p->fout->framecount = 1;
    bframe = (float *) p->fout->frame.auxp;
    for (i = 0; i < N + 2; i += 2) {
      bframe[i] = 0.0f;
      bframe[i + 1] = (i / 2) * N * csound->onedsr;
    }
    return OK;
}

typedef struct {
  OPDS h;
  PVSDAT *fin;
  MYFLT  *file;
  int    pvfile;
  AUXCH  frame;
  unsigned long lastframe;
}PVSFWRITE;

static int pvsfwrite_destroy(CSOUND *csound, void *p){
 csound->PVOC_CloseFile(csound,((PVSFWRITE *) p)->pvfile);
 return OK;
}

static int pvsfwriteset(CSOUND *csound, PVSFWRITE *p)
{
    int N;
    char *fname = csound->strarg2name(csound, NULL, p->file,
                                      "pvoc.",p->XSTRCODE);
    p->pvfile= -1;
    N = p->fin->N;
    if((p->pvfile  = csound->PVOC_CreateFile(csound, fname,
                                             p->fin->N,
                                             p->fin->overlap, 1, p->fin->format,
                                             csound->esr, STYPE_16,
                                             p->fin->wintype, 0.0f, NULL,
                                             p->fin->winsize)) == -1)
      return csound->InitError(csound,
                               Str("pvsfwrite: could not open file %s\n"),
                               fname);
    if (p->frame.auxp == NULL || p->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->frame);
    csound->RegisterDeinitCallback(csound, p, pvsfwrite_destroy);
    p->lastframe = 0;
    return OK;
}

static int pvsfwrite(CSOUND *csound, PVSFWRITE *p)
{
    float *fout = p->frame.auxp;
    float *fin = p->fin->frame.auxp;

    if (p->lastframe < p->fin->framecount) {
      long framesize = p->fin->N + 2, i;
      MYFLT scale = csound->e0dbfs;
      for(i=0;i < framesize; i+=2) {
        fout[i] = fin[i]/scale;
        fout[i+1] = fin[i+1];
      }
      if (!csound->PVOC_PutFrames(csound, p->pvfile, fout, 1))
        return csound->PerfError(csound, Str("pvsfwrite: could not write data\n"));
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
  MYFLT  pos;
  int oldpos;
  int chans, chn;
  int pvfile;
  int scnt;
  unsigned long  flen;
  AUXCH buffer;
} pvsdiskin;

#define FSIGBUFRAMES 2

static int pvsdiskinset(CSOUND *csound, pvsdiskin *p)
{
    int N;
    WAVEFORMATEX fmt;
    PVOCDATA   pvdata;
    char *fname = csound->strarg2name(csound, NULL, p->file,
                                      "pvoc.",p->XSTRCODE);
   
    if((p->pvfile  = csound->PVOC_OpenFile(csound, fname,
					   &pvdata, &fmt)) < 0)
      return csound->InitError(csound,
                               Str("pvsdiskin: could not open file %s\n"),
                               fname);

    N = (pvdata.nAnalysisBins-1)*2;
    p->chans = fmt.nChannels;  

    if(p->fout->frame.auxp == NULL || 
         p->fout->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

    if(p->buffer.auxp == NULL || 
         p->buffer.size < sizeof(float) * (N + 2) * FSIGBUFRAMES * p->chans)
        csound->AuxAlloc(csound, 
          (N + 2) * sizeof(float) * FSIGBUFRAMES * p->chans, 
           &p->buffer);

    p->flen = csound->PVOC_FrameCount(csound, p->pvfile) - 1;

  
    p->fout->N = N;
    p->fout->overlap =  pvdata.dwOverlap;
    p->fout->winsize = pvdata.dwWinlen;
    switch ((pv_wtype) pvdata.wWindowType) {
      case PVOC_DEFAULT:
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
       p->pos = *p->ioff * csound->esr/N;
       p->oldpos = -1;
      
       p->chn = (int) (*p->ichn <= p->chans ? *p->ichn : p->chans) -1;
       if(p->chn < 0) p->chn = 0;
    return OK;
}

  static int pvsdiskinproc(CSOUND *csound, pvsdiskin *p){
    int overlap = p->fout->overlap, frames, i, posi;
    MYFLT pos = p->pos;
    long N = p->fout->N;
    MYFLT frac;
    float *fout = (float *)  p->fout->frame.auxp;
    float *buffer = (float *) p->buffer.auxp;
    float *frame1 = buffer + (N+2)*p->chn;
    float *frame2 = buffer + (N+2)*(p->chans + p->chn);
    float amp = (float) (*p->kgain * csound->e0dbfs);
    
    
    if(p->scnt >= overlap){
      posi = (int) pos;
      if(posi != p->oldpos) {
	/* 
           read new frame
           PVOC_Rewind() is now PVOC_fseek() adapted to work
           as fseek(), using the last argument as
           offset
	*/
        while(pos >= p->flen) pos -= p->flen;
        while(pos < 0) pos += p->flen;
        csound->PVOC_fseek(csound,p->pvfile, pos);
        frames = csound->PVOC_GetFrames(csound, p->pvfile, buffer, 2*p->chans);
        p->oldpos = posi = (int)pos;
       
      }
      // interpolate        
      frac = pos - posi;
      for(i=0; i < N+2; i+=2){
        fout[i] = amp*(frame1[i] + frac*(frame2[i] - frame1[i]));
        fout[i+1] =  frame1[i+1] + frac*(frame2[i+1] - frame1[i+1]);
      }
      p->pos += (*p->kspeed * p->chans);
      p->scnt -= overlap;
      p->fout->framecount++;
    }
    p->scnt += csound->ksmps;

    return OK;
  }




static int pvsfreezeset(CSOUND *csound, PVSFREEZE *p)
{
    long    N = p->fin->N;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    if (p->freez.auxp == NULL || p->freez.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->freez);

    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format == PVS_AMP_FREQ) ||
        (p->fout->format == PVS_AMP_PHASE))
      return csound->InitError(csound, Str("pvsfreeze: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsfreezeprocess(CSOUND *csound, PVSFREEZE *p)
{
    int     i;
    long    framesize;
    MYFLT   freeza, freezf;
    float   *fout, *fin, *freez;
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
    float   *bframe;
    long    N = (long) *p->framesize;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = (long)(*p->olap ? *p->olap : N/4);
    p->fout->winsize = (long)(*p->winsize ? *p->winsize : N);
    p->fout->wintype = (long) *p->wintype;
    p->fout->format = (long) *p->format;
    p->fout->framecount = 0;
    bframe = (float *) p->fout->frame.auxp;
    for (i = 0; i < N + 2; i += 2) {
      bframe[i] = 0.0f;
      bframe[i + 1] = (i / 2) * N * csound->onedsr;
    }
    p->lastframe = 1;
    p->incr = (MYFLT)csound->ksmps/p->fout->overlap;
    return OK;
}

static int pvsoscprocess(CSOUND *csound, PVSOSC *p)
{
    int     i, harm, type;
    long    framesize;
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

    if (p->lastframe > p->fout->framecount) {
      w = csound->esr/p->fout->N;
      harm = (int)(csound->esr/(2*ffun));
      for (i = 0; i < framesize; i ++) fout[i] = 0.f;

      if(type==1) famp *= (MYFLT)(1.456/pow(harm, 1./2.4));
      else if(type==2) famp *= (MYFLT)(1.456/pow(harm, 1./4));
      else if(type==3) famp *= (MYFLT)(1.456/pow(harm, 1./160.));
      else {
        harm = 1;
        famp *= (MYFLT)1.456;
      }
      for(n=1; n <= harm; n++){
        if(type == 3) amp = famp/(harm);
        else amp = (famp/n);
        freq = ffun*n;
        cfbin = freq/w;
        cbin = (int)MYFLT2LRND(cfbin);
        for(i=cbin-1;i < cbin+3 &&i < framesize/2 ; i++){
          k = i<<1;
          a = sin(i-cfbin)/(i-cfbin);
          fout[k] = amp*a*a*a;
          fout[k+1] = freq;
        }
        if(type==2) n++;
      }
      p->fout->framecount = p->lastframe;
    }
    p->incr += p->incr;
    if(p->incr > 1){
      p->incr = (MYFLT)csound->ksmps/p->fout->overlap;
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
    long    framesize, pos;
    float   *fin;
    fin = (float *) p->fin->frame.auxp;
    if (p->lastframe < p->fin->framecount) {
       framesize = p->fin->N + 2;
       pos=*p->kbin*2;
       if(pos >= 0 && pos < framesize){
         *p->kamp = (MYFLT)fin[pos];
         *p->kfreq = (MYFLT)fin[pos+1];
       }
       p->lastframe = p->fin->framecount;
    }
    return OK;
}

static int pvsmoothset(CSOUND *csound, PVSMOOTH *p)
{
    long    N = p->fin->N;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    if (p->del.auxp == NULL || p->del.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->del);

    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format == PVS_AMP_FREQ) ||
        (p->fout->format == PVS_AMP_PHASE))
      return csound->InitError(csound, Str("pvsmooth: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsmoothprocess(CSOUND *csound, PVSMOOTH *p)
{
    int     i;
    long    framesize;
    MYFLT   ffa, ffr;
    float   *fout, *fin, *del;

    ffa = *p->kfra;
    ffr = *p->kfrf;
    fout = (float *) p->fout->frame.auxp;
    fin = (float *) p->fin->frame.auxp;
    del = (float *) p->del.auxp;

    framesize = p->fin->N + 2;

    if (p->lastframe < p->fin->framecount) {
      double  costh1, costh2, coef1, coef2;

      ffa = ffa < 0 ? FL(0.0) : (ffa > 1 ? FL(1.0) : ffa);
      ffr = ffr < 0 ? FL(0.0) : (ffr > 1 ? FL(1.0) : ffr);
      costh1 = 2. - cos(PI * ffa);
      costh2 = 2. - cos(PI * ffr);
      coef1 = sqrt(costh1 * costh1 - 1.) - costh1;
      coef2 = sqrt(costh2 * costh2 - 1.) - costh2;

      for (i = 0; i < framesize; i += 2) {
        /* amp smoothing */
        fout[i] = (float) (fin[i] * (1 + coef1) - del[i] * coef1);
        /* freq smoothing */
        fout[i + 1] = (float) (fin[i + 1] * (1 + coef2) - del[i + 1] * coef1);
        del[i] = fout[i];
        del[i + 1] = fout[i + 1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

static int pvsmixset(CSOUND *csound, PVSMIX *p)
{
    long    N = p->fa->N;

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

    if (!(p->fout->format == PVS_AMP_FREQ) ||
        (p->fout->format == PVS_AMP_PHASE))
      return csound->InitError(csound, Str("pvsmix: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsmix(CSOUND *csound, PVSMIX *p)
{
    int     i;
    long    framesize;
    int     test;
    float   *fout, *fa, *fb;

    if (!fsigs_equal(p->fa, p->fb))
      return csound->PerfError(csound, Str("pvsmix: formats are different."));
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
      p->fout->framecount = p->lastframe = p->fa->framecount;
    }
    return OK;
}

/* pvsfilter  */

static int pvsfilterset(CSOUND *csound, PVSFILTER *p)
{
    long    N = p->fin->N;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format == PVS_AMP_FREQ) ||
        (p->fout->format == PVS_AMP_PHASE))
      return csound->InitError(csound, Str("pvsfilter: signal format "
                                           "must be amp-phase or amp-freq."));
    return OK;
}

static int pvsfilter(CSOUND *csound, PVSFILTER *p)
{
    long    i, N = p->fout->N;
    float   g = (float) *p->gain;
    MYFLT   dirgain, kdepth = (MYFLT) *p->kdepth;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    float   *fil = (float *) p->fil->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvsfilter: not initialised"));
    if (!fsigs_equal(p->fin, p->fil))
      return csound->PerfError(csound,
                               Str("pvsfilter: formats are different."));

    if (p->lastframe < p->fin->framecount) {
      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : FL(1.0)) : FL(0.0);
      dirgain = 1 - kdepth;
      for (i = 0; i < N + 2; i += 2) {
        fout[i] = (float) (fin[i] * (dirgain + fil[i] * kdepth)) * g;
        fout[i + 1] = fin[i + 1];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

/* pvscale  */

static int pvsscaleset(CSOUND *csound, PVSSCALE *p)
{
    long    N = p->fin->N;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    return OK;
}

static int pvsscale(CSOUND *csound, PVSSCALE *p)
{
    long    i, chan, newchan, N = p->fout->N;
    float   max = 0.0f;
    MYFLT   pscal = (MYFLT) fabs(*p->kscal);
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvscale: not initialised"));

    if (p->lastframe < p->fin->framecount) {

      fout[0] = fin[0];
      fout[N] = fin[N];

      for (i = 2; i < N; i += 2) {
        max = max < fin[i] ? fin[i] : max;
        fout[i] = 0.f;
        fout[i + 1] = -1.0f;
      }

      for (i = 2, chan = 1; i < N; chan++, i += 2) {

        newchan = (int) (chan * pscal) << 1;

        if (newchan < N && newchan > 0) {
          fout[newchan] = keepform ?
              (keepform == 1 ||
               !max ? fin[newchan] : fin[i] * (fin[newchan] / max))
              : fin[i];
          fout[newchan + 1] = (float) (fin[i + 1] * pscal);
        }
      }

      for (i = 2; i < N; i += 2) {
        if (fout[i + 1] == -1.0f)
          fout[i] = 0.0f;
        else
          fout[i] *= g;
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

/* pvshift */

static int pvsshiftset(CSOUND *csound, PVSSHIFT *p)
{
    long    N = p->fin->N;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))  /* RWD MUST be 32bit */
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    return OK;
}

static int pvsshift(CSOUND *csound, PVSSHIFT *p)
{
    long    i, chan, newchan, N = p->fout->N;
    MYFLT   pshift = (MYFLT) *p->kshift;
    int     lowest = abs((int) (*p->lowest * N * csound->onedsr));
    float   max = 0.0f;
    int     cshift = (int) (pshift * N * csound->onedsr);
    int     keepform = (int) *p->keepform;
    float   g = (float) *p->gain;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvshift: not initialised"));

    if (p->lastframe < p->fin->framecount) {

      lowest = lowest ? (lowest > N / 2 ? N / 2 : lowest << 1) : 2;

      fout[0] = fin[0];
      fout[N] = fin[N];

      for (i = 2; i < N; i += 2) {
        max = max < fin[i] ? fin[i] : max;

        if (i < lowest) {
          fout[i] = fin[i];
          fout[i + 1] = fin[i + 1];
        }
        else {
          fout[i] = 0.0f;
          fout[i + 1] = -1.0f;
        }
      }

      for (i = lowest, chan = lowest >> 1; i < N; chan++, i += 2) {

        newchan = (chan + cshift) << 1;

        if (newchan < N && newchan > lowest) {
          fout[newchan] = keepform ?
              (keepform == 1 ||
               !max ? fin[newchan] : fin[i] * (fin[newchan] / max))
              : fin[i];
          fout[newchan + 1] = (float) (fin[i + 1] + pshift);
        }
      }

      for (i = lowest; i < N; i += 2) {
        if (fout[i + 1] == -1.0f)
          fout[i] = 0.0f;
        else
          fout[i] *= g;
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

/* pvsblur */

static int pvsblurset(CSOUND *csound, PVSBLUR *p)
{
    float   *delay;
    long    N = p->fin->N, i, j;
    int     olap = p->fin->overlap;
    int     delayframes, framesize = N + 2;

    p->frpsec = csound->esr / olap;

    delayframes = (int) (*p->maxdel * p->frpsec);

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

    if (p->delframes.auxp == NULL)
      csound->AuxAlloc(csound, (N + 2) * sizeof(float) * delayframes,
                       &p->delframes);

    delay = (float *) p->delframes.auxp;

    for (j = 0; j < framesize * delayframes; j += framesize)
      for (i = 0; i < N + 2; i += 2) {
        delay[i + j] = 0.f;
        delay[i + j + 1] = i * csound->esr / N;
      }

    p->fout->N = N;
    p->fout->overlap = olap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->count = 0;
    return OK;
}

static int pvsblur(CSOUND *csound, PVSBLUR *p)
{
    long    j, i, N = p->fout->N, first, framesize = N + 2;
    long    countr = p->count;
    double  amp = 0.0, freq = 0.0;
    int     delayframes = (int) (*p->kdel * p->frpsec);
    int     kdel = delayframes * framesize;
    int     mdel = (int) (*p->maxdel * p->frpsec) * framesize;
    float   *fin = (float *) p->fin->frame.auxp;
    float   *fout = (float *) p->fout->frame.auxp;
    float   *delay = (float *) p->delframes.auxp;

    if (fout == NULL || delay == NULL)
      return csound->PerfError(csound, Str("pvsblur: not initialised"));

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
}

/* pvstencil  */

static int pvstencilset(CSOUND *csound, PVSTENCIL *p)
{
    long    N = p->fin->N, i;
    long    chans = N / 2 + 1;
    MYFLT   *ftable;

    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

    p->fout->N = N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format == PVS_AMP_FREQ) ||
        (p->fout->format == PVS_AMP_PHASE))
      return csound->InitError(csound, Str("pvstencil: signal format "
                                           "must be amp-phase or amp-freq."));

    p->func = csound->FTFind(csound, p->ifn);
    if (p->func == NULL)
      return OK;

    if (p->func->flen + 1 < chans)
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
    long    framesize, i, j;
    int     test;
    float   *fout, *fin;
    MYFLT   *ftable;
    float   g = (float) fabs(*p->kgain);
    float   masklevel = (float) fabs(*p->klevel);

    fout = (float *) p->fout->frame.auxp;
    fin = (float *) p->fin->frame.auxp;
    ftable = p->func->ftable;

    framesize = p->fin->N + 2;

    if (fout == NULL)
      return csound->PerfError(csound, Str("pvstencil: not initialised"));

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
    return OK;
}

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
    if ((f1->overlap == f2->overlap) &&
        (f1->winsize == f2->winsize) &&
        (f1->wintype == f2->wintype) &&     /* harsh, maybe... */
        (f1->N == f2->N) &&
        (f1->format == f2->format))
      return 1;
    return 0;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {

    {"pvsfwrite", S(PVSFWRITE), 3, "", "fT", (SUBR) pvsfwriteset,
         (SUBR) pvsfwrite},
    {"pvscale", S(PVSSCALE), 3, "f", "fkop", (SUBR) pvsscaleset,
         (SUBR) pvsscale},
    {"pvshift", S(PVSSHIFT), 3, "f", "fkkop", (SUBR) pvsshiftset,
         (SUBR) pvsshift},
    {"pvsmix", S(PVSMIX), 3, "f", "ff", (SUBR) pvsmixset, (SUBR) pvsmix, NULL},
    {"pvsfilter", S(PVSFILTER), 3, "f", "ffkp", (SUBR) pvsfilterset,
         (SUBR) pvsfilter},
    {"pvsblur", S(PVSBLUR), 3, "f", "fki", (SUBR) pvsblurset, (SUBR) pvsblur,
         NULL},
    {"pvstencil", S(PVSTENCIL), 3, "f", "fkki", (SUBR) pvstencilset,
         (SUBR) pvstencil},
    {"pvsinit", S(PVSINI), 1, "f", "ioopo", (SUBR) pvsinit, NULL, NULL},
    {"pvsbin", S(PVSBIN), 3, "kk", "fk", (SUBR) pvsbinset,
         (SUBR) pvsbinprocess, NULL},
    {"pvsfreeze", S(PVSFREEZE), 3, "f", "fkk", (SUBR) pvsfreezeset,
         (SUBR) pvsfreezeprocess, NULL},
    {"pvsmooth", S(PVSFREEZE), 3, "f", "fkk", (SUBR) pvsmoothset,
     (SUBR) pvsmoothprocess, NULL},
{"pvsosc", S(PVSOSC), 3, "f", "kkkioopo", (SUBR) pvsoscset,
 (SUBR) pvsoscprocess, NULL},
    {"pvsdiskin", S(pvsdiskin), 3, "f", "Skkop",(SUBR) pvsdiskinset,
     (SUBR) pvsdiskinproc, NULL}
    
};

int pvsbasic_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

