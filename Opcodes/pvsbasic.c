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

#include "csdl.h"
#include "pvsbasic.h"
#include "pvfileio.h"
#include <math.h>

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2);

static int pvsinit(CSOUND *csound, PVSINI *p)
{
  int     i;
  float   *bframe;
  int32    N = (int32) *p->framesize;

  p->fout->N = N;
  p->fout->overlap = (int32)(*p->olap ? *p->olap : N/4);
  p->fout->winsize = (int32)(*p->winsize ? *p->winsize : N);
  p->fout->wintype = (int32) *p->wintype;
  p->fout->format = (int32) *p->format;
  p->fout->framecount = 1;
#ifndef OLPC
  p->fout->sliding = 0;
  if (p->fout->overlap < csound->ksmps || p->fout->overlap <=10) {
    int n;
    int NB = 1+N/2;
    MYFLT *bframe;
    p->fout->NB = NB;
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size * csound->ksmps < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * csound->ksmps * sizeof(float),
		       &p->fout->frame);
    p->fout->sliding = 1;
    bframe = (MYFLT *) p->fout->frame.auxp;
    for (n=0; n<csound->ksmps; n++)
      for (i = 0; i < N + 2; i += 2) {
	bframe[i+n*NB] = FL(0.0);
	bframe[i+n*NB + 1] = (i >>1) * N * csound->onedsr;
      }
  }
  else
#endif
    {
      if (p->fout->frame.auxp == NULL ||
	  p->fout->frame.size < sizeof(float) * (N + 2)) {
	csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
      }
      bframe = (float *) p->fout->frame.auxp;
      for (i = 0; i < N + 2; i += 2) {
	bframe[i] = 0.0f;
	bframe[i + 1] = (i >>1) * N * csound->onedsr;
      }
    }
  return OK;
}

typedef struct {
  OPDS h;
  PVSDAT *fin;
  MYFLT  *file;
  int    pvfile;
  AUXCH  frame;
  uint32 lastframe;
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
#ifndef OLPC
  if (UNLIKELY(p->fin->sliding))
    return csound->InitError(csound,Str("SDFT Not implemented in this case yet"));
#endif
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
    int32 framesize = p->fin->N+2, i;
    MYFLT scale = csound->e0dbfs;
    for(i=0;i < framesize; i+=2) {
      fout[i] = fin[i]/scale;
      fout[i+1] = fin[i+1];
    }
    if (UNLIKELY(!csound->PVOC_PutFrames(csound, p->pvfile, fout, 1)))
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

static int pvsdiskinset(CSOUND *csound, pvsdiskin *p)
{
  int N;
  WAVEFORMATEX fmt;
  PVOCDATA   pvdata;
  char *fname = csound->strarg2name(csound, NULL, p->file,
				    "pvoc.",p->XSTRCODE);

#ifndef OLPC
  if (UNLIKELY(p->fout->sliding))
    return csound->InitError(csound,Str("SDFT Not implemented in this case yet"));
#endif
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

static int pvsdiskinproc(CSOUND *csound, pvsdiskin *p)
{
  int overlap = p->fout->overlap, frames, i, posi;
  double pos = p->pos;
  int32 N = p->fout->N;
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
    if(*p->interp){
      /* interpolate */
      frac = pos - posi;
      for(i=0; i < N+2; i+=2){
        fout[i] = amp*(frame1[i] + frac*(frame2[i] - frame1[i]));
	fout[i+1] =  frame1[i+1] + frac*(frame2[i+1] - frame1[i+1]);
      }
    } else  /* don't */
      for(i=0; i < N+2; i+=2){
        fout[i] = amp*(frame1[i]);
	fout[i+1] =  frame1[i+1];
      } 


    p->pos += (*p->kspeed * p->chans);
    p->scnt -= overlap;
    p->fout->framecount++;
  }
  p->scnt += csound->ksmps;

  return OK;
}

typedef struct _pvst {
  OPDS h;
  PVSDAT *fout;
  MYFLT  *ktime;
  MYFLT  *kamp;
  MYFLT  *kpitch;
  MYFLT  *knum;
  MYFLT  *fftsize, *hsize;
  uint32 scnt;
  float factor, fund, rotfac, scale;
  AUXCH bwin;
  AUXCH fwin;
  AUXCH win;
} PVST;

int pvstanalset(CSOUND *csound, PVST *p){

  int i, N = p->fout->N = (*p->fftsize > 0 ? *p->fftsize : 2048);
  int hsize = p->fout->overlap = (*p->hsize > 0 ? *p->hsize : 512);
    
  p->fout->wintype = PVS_WIN_HANN;
  p->fout->winsize = N;
  p->fout->framecount = 1;
  if(p->fout->frame.auxp == NULL ||
     p->fout->frame.size < sizeof(float) * (N + 2))
    csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame); 
  if(p->bwin.auxp == NULL ||
     p->bwin.size < sizeof(float) * (N + 2))
    csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->bwin);
  if(p->fwin.auxp == NULL ||
     p->fwin.size < sizeof(float) * (N + 2))
    csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fwin);
  if(p->win.auxp == NULL ||
     p->fwin.size < sizeof(float) * (N))
    csound->AuxAlloc(csound, (N) * sizeof(float), &p->win);
  p->scale = 0.f;
  for(i=0; i < N; i++)
    p->scale += ((MYFLT *)p->win.auxp)[i] = 0.5 - 0.5*cos(i*2*PI/N);

  memset(p->fwin.auxp, 0, sizeof(float)*(N+2));
  memset(p->bwin.auxp, 0, sizeof(float)*(N+2));
  memset(p->fout->frame.auxp, 0, sizeof(float)*(N+2));
  p->rotfac = hsize*TWOPI/N;
  p->factor = csound->esr/(hsize*TWOPI);
  p->fund = csound->esr/N;
  p->scnt = p->fout->overlap;
  return OK;
}


int pvstanal(CSOUND *csound, PVST *p){

  int hsize = p->fout->overlap, i, k;
  uint32 pos, size, post;
  int32 N = p->fout->N;
  double frac, spos;
  MYFLT *tab;
  FUNC *ft;
  float *fout = (float *)  p->fout->frame.auxp, *win = (float *) p->win.auxp;
  float *bwin = (float *) p->bwin.auxp, *fwin = (float *) p->fwin.auxp ;
  float amp = (float) (*p->kamp), factor = p->factor, fund = p->fund;
  float pitch = (float) (*p->kpitch), rotfac = p->rotfac, scale = p->scale;

  if(p->scnt >= hsize){
        
    /* audio samples are stored in a function table */
    ft = csound->FTnp2Find(csound,p->knum);
    tab = ft->ftable;
    size = ft->flen;
    /* spos is the reading position in samples, hsize is hopsize,
       time is current read position in secs
       esr is sampling rate
    */
    spos  = hsize*(int)(*p->ktime*csound->esr/hsize);
    while(spos > size) spos -= size;
    while(spos <= 0)  spos += size;
    pos = spos;
    /* this loop fills two frames/windows with samples from table,
       reading is linearly-interpolated,
       frames are separated by 1 hopsize
    */
    for(i=0; i < N; i++) {
      /* front window, fwin */
      MYFLT in; 
      post = (int) pos;
      frac = pos  - post;
      if(post >= 0 && post < size)
	in = tab[post] + frac*(tab[post+1] - tab[post]);
      else in =  (MYFLT) 0;
      fwin[i] = amp * in * win[i]; /* window it */
      /* back windo, bwin */
      post = (int) (pos - hsize*pitch);
      if(post >= 0 && post < size)
	in =  tab[post] + frac*(tab[post+1] - tab[post]);
      else in =  (MYFLT) 0;
      bwin[i] = in * win[i];  /* window it */
      /* increment read pos according to pitch transposition */
      pos += pitch;
    }
    /* take the FFT of both frames
       re-order Nyquist bin from pos 1 to N
    */
    csound->RealFFT(csound, bwin, N);
    csound->RealFFT(csound, fwin, N);
    fwin[N] = fwin[1]/scale; fwin[0] = fwin[0]/scale;
    fwin[N+1] = fwin[1] = 0.0;

    for(i=2,k=1; i < N; i+=2, k++){
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
      fwin[i] /= scale; fwin[i+1] /= scale;
      fout[i] = sqrt(fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1]);
    }

    p->scnt -= hsize;
    p->fout->framecount++;
  }
  p->scnt += csound->ksmps;

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

#ifndef OLPC
  p->fout->NB = (N/2)+1;
  p->fout->sliding = p->fin->sliding;
  if (p->fin->sliding) {
    int nsmps = csound->ksmps;
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * (N + 2) * nsmps)
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * nsmps,
		       &p->fout->frame);
    if (p->freez.auxp == NULL ||
	p->freez.size < sizeof(MYFLT) * (N + 2) * nsmps)
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * nsmps, &p->freez);
  }
  else
#endif
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

#ifndef OLPC
static int pvssfreezeprocess(CSOUND *csound, PVSFREEZE *p)
{
  int i, n, k, nsmps = csound->ksmps;
  int NB = p->fin->NB;
  MYFLT freeza = *p->kfra, freezf = *p->kfrf;

  for (n=0, k=nsmps-1; n<nsmps; n++, k=(k+1)%nsmps) {
    CMPLX *fz = (CMPLX*)p->freez.auxp;
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
#endif

static int pvsfreezeprocess(CSOUND *csound, PVSFREEZE *p)
{
  int     i;
  int32    framesize;
  MYFLT   freeza, freezf;
  float   *fout, *fin, *freez;
#ifndef OLPC
  if (p->fin->sliding)
    return pvssfreezeprocess(csound, p);
#endif
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
#ifndef OLPC
  p->fout->sliding = 0;
  if (p->fout->overlap<csound->ksmps || p->fout->overlap<=10) {
    CMPLX *bframe;
    int NB = 1+N/2, n;
    return csound->InitError(csound, Str("pvsosc does not work while sliding"));
    p->fout->NB = NB;
    p->fout->sliding = 1;
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < csound->ksmps*sizeof(MYFLT) * (N + 2))
      csound->AuxAlloc(csound,
		       (N + 2) * csound->ksmps* sizeof(MYFLT), &p->fout->frame);
    else memset(p->fout->frame.auxp,
		'\0', (N + 2) * csound->ksmps* sizeof(MYFLT));
    bframe = (CMPLX *)p->fout->frame.auxp;
    for (n=0; n<csound->ksmps; n++)
      for (i = 0; i < NB; i++) {
	bframe[i+NB*n].re = FL(0.0);
	bframe[i+NB*n].im = i * N * csound->onedsr;
      }
    return OK;
  }
  else
#endif
    {
      float   *bframe;
      if (p->fout->frame.auxp == NULL ||
	  p->fout->frame.size < sizeof(float) * (N + 2))
	csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);
      bframe = (float *) p->fout->frame.auxp;
      for (i = 0; i < N + 2; i += 2) {
	bframe[i] = 0.0f;
	bframe[i + 1] = (i / 2) * N * csound->onedsr;
      }
      p->lastframe = 1;
      p->incr = (MYFLT)csound->ksmps/p->fout->overlap;
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

#ifndef OLPC
  if (p->fout->sliding) {
    CMPLX *fout;
    int m, nsmps = csound->ksmps;
    int NB = p->fout->NB;
    harm = (int)(csound->esr/(2*ffun));
    if (type==1) famp *= FL(1.456)/POWER((MYFLT)harm, FL(1.0)/FL(2.4));
    else if (type==2) famp *= FL(1.456)/POWER((MYFLT)harm, FL(0.25));
    else if (type==3) famp *= FL(1.456)/POWER((MYFLT)harm, FL(1.0)/FL(160.0));
    else {
      harm = 1;
      famp *= FL(1.456);
    }

    for (n=0; n<nsmps; n++) {
      fout = (CMPLX*) p->fout->frame.auxp + n*NB;
      w = csound->esr/p->fout->N;
      /*         harm = (int)(csound->esr/(2*ffun)); */
      memset(fout, '\0', NB*sizeof(CMPLX));
      for (m=1; m <= harm; m++){
	if (type == 3) amp = famp/(harm);
	else amp = (famp/m);
	freq = ffun*m;
	cfbin = freq/w;
	cbin = (int)MYFLT2LRND(cfbin);
	for (i=cbin-1;i < cbin+3 &&i < NB ; i++){
	  a = sin(i-cfbin)/(i-cfbin);
	  fout[i].re = amp*a*a*a;
	  fout[i].im = freq;
	}
	if (type==2) m++;
      }
    }
    return OK;
  }
#endif
  if (p->lastframe > p->fout->framecount) {
    w = csound->esr/p->fout->N;
    harm = (int)(csound->esr/(2*ffun));
    if (type==1) famp *= FL(1.456)/pow(harm, FL(1.0)/FL(2.4));
    else if (type==2) famp *= FL(1.456)/POWER(harm, FL(0.25));
    else if (type==3) famp *= FL(1.456)/POWER(harm, FL(1.0)/FL(160.0));
    else {
      harm = 1;
      famp *= FL(1.456);
    }
    memset(fout, 0, sizeof(float)*framesize);
    /* for (i = 0; i < framesize; i ++) fout[i] = 0.f; */

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
  int32    framesize, pos;
#ifndef OLPC
  if (p->fin->sliding) {
    CMPLX *fin = (CMPLX *) p->fin->frame.auxp;
    framesize = p->fin->NB;
    pos=*p->kbin;
    if(pos >= 0 && pos < framesize){
      *p->kamp = (MYFLT)fin[pos].re;
      *p->kfreq = (MYFLT)fin[pos].im;
    }
  }
  else
#endif
    {
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
    }
  return OK;
}

#ifndef OLPC
static int pvsbinprocessa(CSOUND *csound, PVSBIN *p)
{
  int32    framesize, pos, k;
  if (p->fin->sliding) {
    CMPLX *fin = (CMPLX *) p->fin->frame.auxp;
    int NB = p->fin->NB;
    pos = *p->kbin;
    if (pos >= 0 && pos < NB) {
      for (k=0; k<csound->ksmps; k++) {
	p->kamp[k] = (MYFLT)fin[pos+NB*k].re;
	p->kfreq[k] = (MYFLT)fin[pos+NB*k].im;
      }
    }
  }
  else {
    float   *fin;
    fin = (float *) p->fin->frame.auxp;
    if (p->lastframe < p->fin->framecount) {
      framesize = p->fin->N + 2;
      pos=*p->kbin*2;
      if(pos >= 0 && pos < framesize){
	for (k=0; k<csound->ksmps; k++) {
	  p->kamp[k] = (MYFLT)fin[pos];
	  p->kfreq[k] = (MYFLT)fin[pos+1];
	}
	p->lastframe = p->fin->framecount;
      }
    }
  }
  return OK;
}
#endif

static int pvsmoothset(CSOUND *csound, PVSMOOTH *p)
{
  int32    N = p->fin->N;

  if (UNLIKELY(p->fin == p->fout))
    csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
#ifndef OLPC
  p->fout->NB = (N/2)+1;
  p->fout->sliding = p->fin->sliding;
  if (p->fin->sliding) {
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * csound->ksmps * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * csound->ksmps,
		       &p->fout->frame);
    if (p->del.auxp == NULL ||
	p->del.size < sizeof(MYFLT) * csound->ksmps * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * csound->ksmps,
		       &p->del);
  }
  else
#endif
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

#ifndef OLPC
  if (p->fin->sliding) {
    CMPLX *fout, *fin, *del;
    double  costh1, costh2, coef1, coef2;
    int n, nsmps=csound->ksmps;
    int NB = p->fin->NB;
    ffa = ffa < 0.0 ? 0.0 : (ffa > 1.0 ? 1.0 : ffa);
    ffr = ffr < 0.0 ? 0.0 : (ffr > 1.0 ? 1.0 : ffr);
    costh1 = 2.0 - cos(PI * ffa);
    costh2 = 2.0 - cos(PI * ffr);
    coef1 = sqrt(costh1 * costh1 - 1.0) - costh1;
    coef2 = sqrt(costh2 * costh2 - 1.0) - costh2;

    for (n=0; n<nsmps; n++) {
      fout = (CMPLX*) p->fout->frame.auxp +NB*n;
      fin = (CMPLX*) p->fin->frame.auxp +NB*n;
      del = (CMPLX*) p->del.auxp +NB*n;
      if (XINARG2) {
	ffa = (double)  p->kfra[n];
	ffa = ffa < 0.0 ? 0.0 : (ffa > 1.0 ? 1.0 : ffa);
	costh1 = 2.0 - cos(PI * ffa);
	coef1 = sqrt(costh1 * costh1 - 1.0) - costh1;
      }
      if (XINARG3) {
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
#endif
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

  if (UNLIKELY(p->fa == p->fout || p->fb == p->fout))
    csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
#ifndef OLPC
  p->fout->sliding = 0;
  if (p->fa->sliding) {
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * csound->ksmps * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * csound->ksmps,
		       &p->fout->frame);
    p->fout->NB = p->fa->NB;
    p->fout->sliding = 1;
  }
  else
#endif
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
#ifndef OLPC
  if (p->fa->sliding) {
    CMPLX * fout, *fa, *fb;
    int n, nsmps=csound->ksmps;
    int NB = p->fa->NB;
    for (n=0; n<nsmps; n++) {
      fout = (CMPLX*) p->fout->frame.auxp +NB*n;
      fa = (CMPLX*) p->fa->frame.auxp +NB*n;
      fb = (CMPLX*) p->fb->frame.auxp +NB*n;
      for (i = 0; i < NB; i++) {
	fout[i] = (fa[i].re >= fb[i].re) ? fa[i] : fb[i];
      }
    }
    return OK;
  }
#endif
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
 err1:
  return csound->PerfError(csound, Str("pvsmix: formats are different."));
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
#ifndef OLPC
  p->fout->sliding = 0;
  if (p->fin->sliding) {
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * csound->ksmps * (N + 2))
      csound->AuxAlloc(csound, sizeof(MYFLT) * csound->ksmps * (N + 2),
		       &p->fout->frame);
    p->fout->NB = p->fin->NB;
    p->fout->sliding = 1;
  }
  else
#endif
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

#ifndef OLPC
  if (p->fin->sliding) {
    int NB = p->fout->NB;
    int n, nsmps = csound->ksmps;
    CMPLX *fin, *fout, *fil;
    MYFLT g = *p->gain;
    kdepth = kdepth >= FL(0.0) ? (kdepth <= FL(1.0) ? kdepth*g : g) : FL(0.0);
    dirgain = (FL(1.0) - kdepth)*g;
    for (n=0; n<nsmps; n++) {
      fin = (CMPLX *)p->fin->frame.auxp + NB*n;
      fout = (CMPLX *)p->fout->frame.auxp + NB*n;
      fil = (CMPLX *)p->fil->frame.auxp + NB*n;
      if (XINARG3) {
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
#endif
  if (p->lastframe < p->fin->framecount) {
    kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth*g : g) : FL(0.0);
    dirgain = (1 - kdepth)*g;
    for (i = 0; i < N + 2; i += 2) {
      fout[i] = (float) (fin[i] * (dirgain + fil[i] * kdepth));
      fout[i + 1] = fin[i + 1];
    }

    p->fout->framecount = p->lastframe = p->fin->framecount;
  }
  return OK;
 err1:
  return csound->PerfError(csound, Str("pvsfilter: not initialised"));
 err2:
  return csound->PerfError(csound,
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
  MYFLT   *cf;
  AUXCH   fenv, ceps, peak;
  uint32  lastframe;
} PVSSCALE;

static int pvsscaleset(CSOUND *csound, PVSSCALE *p)
{
  int32    N = p->fin->N;

  if (UNLIKELY(p->fin == p->fout))
    csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
#ifndef OLPC
  p->fout->NB = p->fin->NB;
  p->fout->sliding = p->fin->sliding;
  if (p->fin->sliding) {
    if (p->fout->frame.auxp == NULL ||
        p->fout->frame.size < csound->ksmps * sizeof(MYFLT) * (N + 2))
      csound->AuxAlloc(csound, csound->ksmps * sizeof(MYFLT) * (N + 2),
                       &p->fout->frame);
  }
  else
#endif
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
  memset(p->ceps.auxp, 0, sizeof(MYFLT)*(N+2));
  if (p->fenv.auxp == NULL ||
      p->fenv.size < sizeof(MYFLT) * (N+2)) 
    csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
  memset(p->fenv.auxp, 0, sizeof(MYFLT)*(N+2));


  if (p->peak.auxp == NULL ||
      p->peak.size < sizeof(int) * (N+2)/2)  
    csound->AuxAlloc(csound, sizeof(int) * (N + 2)/2, &p->peak);
  memset(p->peak.auxp, 0, sizeof(int)*(N+2)/2);

  return OK;
}




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
  int   *peaks = (int *) p->peak.auxp;
  int coefs = (int) *p->coefs;
  MYFLT cf = *p->cf;
 
  if(cf < 1) cf = 6000.f*pscal;
  else cf *= pscal; 

  if (UNLIKELY(fout == NULL)) goto err1;

#ifndef OLPC
  if (p->fout->sliding) {
    int n, nsmps = csound->ksmps;
    int NB    = p->fout->NB;
    MYFLT   g = *p->gain;
    for (n=0; n<nsmps; n++) {
      MYFLT    max = FL(0.0);
      CMPLX   *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
      CMPLX   *fout = (CMPLX *) p->fout->frame.auxp + n*NB;

      fout[0] = fin[0];
      fout[NB-1] = fin[NB-1];
      if (XINARG2) {
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
	if (fout[i].im>=csound->esr*0.5 ||
	    fout[i].im<= -csound->esr*0.5)
	  fout[i].re=0.0;
      }

      for (i = 1; i < NB; i++) {
	fout[i].re *= g;
      }
    }
    return OK;
  }
#endif
  if (p->lastframe < p->fin->framecount) {
    int n, k=1; 
    peaks[0] = fin[0];
    fout[0] = fin[0];
    fout[N] = fin[N];
      
    for (i = 2, n=1; i < N; i += 2, n++) {
      fout[i] = 0.0f;
      fout[i + 1] = -1.0f;
      fenv[n] = 0.f;
    }
   
    if(keepform >= 3){ /* modes 3 & 4 */
      int cond = 1; 
      for(i=0; i < N; i+=2) fenv[i/2] = log(fin[i]);
      if(coefs < 1) coefs = 80;
      while(cond){
	cond = 0;
	for(i=0; i < N; i+=2){
	  ceps[i] = fenv[i/2];
          ceps[i+1] = 0.0;
	} 
	csound->InverseComplexFFT(csound, ceps, N/2);
        for(i=coefs; i < N-coefs; i++) ceps[i] = 0.0;   
        csound->ComplexFFT(csound, ceps, N/2);
        for(i=0; i < N; i+=2) {
          if(keepform > 3){
	    if(fenv[i/2] < ceps[i]) 
	      fenv[i/2] = ceps[i];
	    if((log(fin[i]) - ceps[i]) > 0.23) cond = 1;
          }
	  else 
	    {
	      fenv[i/2] = exp(ceps[i]);
	      max = max < fenv[i/2] ? fenv[i/2] : max;
	    }
	}
      }
       if(keepform > 3)
        for(i=0; i<N/2; i++) {
	  fenv[i] = exp(fenv[i]);
	  max = max < fenv[i] ? fenv[i] : max;
	  } 
      if(max)
	for(i=0; i<N/2; i++) fenv[i]/=max;
	
    }

    if(keepform == 2) {
      for(i=2, n = 1; i < N-2; i+=2, n++){
      
	float p2 = fin[i];
	float p3 = fin[i+2];
	float p1 = fin[i-2];
	if(p3 > p2 && p2  > p1){
	  peaks[k] = i; k++;
	}
      }
     
      for(i=1; i < k; i++){
	int len; float incr, start, m1, m2;
        len = (peaks[i] - peaks[i-1])/2;
        m1 = log(fin[peaks[i]]);
	m2 = log(fin[peaks[i-1]]);
        incr = (m1 - m2)/len;
        start = m2;
	for(n=peaks[i-1]/2; n < len+peaks[i-1]/2; n+=1){
	  start += incr; fenv[n] = exp(start);
	}
      }
      
    }
     
    for (i = 2, chan = 1; i < N; chan++, i += 2) {

      int newchan; 
      newchan  = (int) ((chan * pscal)+0.5) << 1;

      if (newchan < N && newchan > 0) {
	float sr = csound->esr, binf,fad;
	binf = (newchan/2)*sr/N;
	fad = (binf - cf)/(sr/2 - cf);
	fout[newchan] = keepform ?
	  (keepform == 1  ? fin[newchan] : (binf > cf ? fin[i]*fad + fin[i]*fenv[newchan/2]*(1.f-fad): 
					    fin[i]*fenv[newchan/2]))
	  : fin[i];
          
	fout[newchan + 1] = (float) (fin[i + 1] * pscal);
	   
      }
    }

    for (i = 2; i < N; i += 2) {
      if(isnan(fout[i])) fout[i] = 0.0f;
      if (fout[i + 1] == -1.0f){
	fout[i] = 0.0f; 
      }
      else
	fout[i] *= g;
    }

    p->fout->framecount = p->lastframe = p->fin->framecount;
  }
  return OK;
 err1:
  return csound->PerfError(csound, Str("pvscale: not initialised"));
}

/* pvshift */

static int pvsshiftset(CSOUND *csound, PVSSHIFT *p)
{
  int    N = p->fin->N;

  if (UNLIKELY(p->fin == p->fout))
    csound->Warning(csound, Str("Unsafe to have same fsig as in and out"));
#ifndef OLPC
  if (p->fin->sliding) {
    if (p->fout->frame.auxp==NULL ||
	csound->ksmps*(N+2)*sizeof(MYFLT) > (unsigned int)p->fout->frame.size)
      csound->AuxAlloc(csound, csound->ksmps*(N+2)*sizeof(MYFLT),&p->fout->frame);
    else memset(p->fout->frame.auxp, 0, csound->ksmps*(N+2)*sizeof(MYFLT));
  }
  else
#endif
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
#ifndef OLPC
  p->fout->sliding = p->fin->sliding;
  p->fout->NB = p->fin->NB;
#endif
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

  if (UNLIKELY(fout == NULL)) goto err1;
#ifndef OLPC
  if (p->fin->sliding) {
    int n, nsmps = csound->ksmps;
    int NB  = p->fout->NB;
    MYFLT g = *p->gain;
    lowest = lowest ? (lowest > NB ? NB : lowest) : 1;

    for (n=0; n<nsmps; n++) {
      MYFLT max = FL(0.0);
      CMPLX *fin = (CMPLX *) p->fin->frame.auxp + n*NB;
      CMPLX *fout = (CMPLX *) p->fout->frame.auxp + n*NB;
      fout[0] = fin[0];
      fout[NB-1] = fin[NB-1];
      if (XINARG2) {
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
	if (fout[i].im>=csound->esr*0.5 ||
	    fout[i].im<= -csound->esr*0.5)
	  fout[i].re = 0.0;
      }
      if (g!=1.0f)
	for (i = lowest; i < NB; i++) {
	  fout[i].re *= g;
        }
    }
    return OK;
  }
#endif
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
	  (keepform == 1 || !max ? fin[newchan] : fin[i] * (fin[newchan] / max)) : fin[i];
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
 err1:
  return csound->PerfError(csound, Str("pvshift: not initialised"));
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
#ifndef OLPC
  if (p->fin->sliding) {
    csound->InitError(csound, Str("pvsblur does not work sliding yet"));
    delayframes = (int) (FL(0.5) + *p->maxdel * csound->esr);
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * csound->ksmps * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * csound->ksmps,
		       &p->fout->frame);

    if (p->delframes.auxp == NULL)
      csound->AuxAlloc(csound,
		       (N + 2) * sizeof(MYFLT) * csound->ksmps * delayframes,
		       &p->delframes);
  }
  else
#endif
    {
      p->frpsec = csound->esr / olap;

      delayframes = (int) (*p->maxdel * p->frpsec);

      if (p->fout->frame.auxp == NULL ||
	  p->fout->frame.size < sizeof(float) * (N + 2))
	csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

      if (p->delframes.auxp == NULL)
	csound->AuxAlloc(csound, (N + 2) * sizeof(float) * delayframes,
			 &p->delframes);
    }
  delay = (float *) p->delframes.auxp;

  for (j = 0; j < framesize * delayframes; j += framesize)
    for (i = 0; i < N + 2; i += 2) {
      delay[i + j] = 0.0f;
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
#ifndef OLPC
  p->fout->sliding = p->fin->sliding;
  p->fout->NB = p->fin->NB;
#endif
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

#ifndef OLPC
  if (p->fin->sliding) {
    int n, nsmps = csound->ksmps;
    int NB = p->fin->NB;
    kdel = kdel >= 0 ? (kdel < mdel ? kdel : mdel - framesize) : 0;
    for (n=0; n<nsmps; n++) {
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
#endif
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
  return csound->PerfError(csound, Str("pvsblur: not initialised"));
}

/* pvstencil  */

static int pvstencilset(CSOUND *csound, PVSTENCIL *p)
{
  int32    N = p->fin->N, i;
  int32    chans = N / 2 + 1;
  MYFLT   *ftable;

  p->fout->N = N;
  p->fout->overlap = p->fin->overlap;
  p->fout->winsize = p->fin->winsize;
  p->fout->wintype = p->fin->wintype;
  p->fout->format = p->fin->format;
  p->fout->framecount = 1;
  p->lastframe = 0;

#ifndef OLPC
  p->fout->NB = chans;
  if (p->fin->sliding) {
    if (p->fout->frame.auxp == NULL ||
	p->fout->frame.size < sizeof(MYFLT) * (N + 2) * csound->ksmps)
      csound->AuxAlloc(csound, (N + 2) * sizeof(MYFLT) * csound->ksmps,
		       &p->fout->frame);
    p->fout->sliding = 1;
  }
  else
#endif
    {
      if (p->fout->frame.auxp == NULL ||
          p->fout->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->fout->frame);

      if (UNLIKELY(!(p->fout->format == PVS_AMP_FREQ) ||
                   (p->fout->format == PVS_AMP_PHASE)))
        return csound->InitError(csound, Str("pvstencil: signal format "
                                             "must be amp-phase or amp-freq."));
    }
  p->func = csound->FTFind(csound, p->ifn);
  if (p->func == NULL)
    return OK;

  if (UNLIKELY(p->func->flen + 1 < chans))
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
#ifndef OLPC
  if (p->fin->sliding) {
    MYFLT g = FABS(*p->kgain);
    MYFLT masklevel = FABS(*p->klevel);
    int NB = p->fin->NB, n, i;
    p->fout->NB = NB;
    p->fout->N = p->fin->N;
    p->fout->format = p->fin->format;
    p->fout->wintype = p->fin->wintype;
    ftable = p->func->ftable;
    for (n=0; n<csound->ksmps; n++) {
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
#endif
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
  return csound->PerfError(csound, Str("pvstencil: not initialised"));
}

static int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
  if (
#ifndef OLPC
      (f1->sliding == f2->sliding) &&
#endif
      (f1->overlap == f2->overlap) &&
      (f1->winsize == f2->winsize) &&
      (f1->wintype == f2->wintype) &&     /* harsh, maybe... */
      (f1->N == f2->N) &&
      (f1->format == f2->format))
    return 1;
  return 0;
}


static OENTRY localops[] = {
  {"pvsfwrite", sizeof(PVSFWRITE), 3, "", "fT", (SUBR) pvsfwriteset,
   (SUBR) pvsfwrite},
#ifndef OLPC
  {"pvsfilter", sizeof(PVSFILTER), 3, "f", "ffxp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
  {"pvscale", sizeof(PVSSCALE), 3, "f", "fxOPOO", (SUBR) pvsscaleset,
   (SUBR) pvsscale},
  {"pvshift", sizeof(PVSSHIFT), 3, "f", "fxkop", (SUBR) pvsshiftset,
   (SUBR) pvsshift},
#else
  {"pvsfilter", sizeof(PVSFILTER), 3, "f", "fffp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
  {"pvscale", sizeof(PVSSCALE), 3, "f", "fkOPOO", (SUBR) pvsscaleset, (SUBR) pvsscale},
  {"pvshift", sizeof(PVSSHIFT), 3, "f", "fkkOp", (SUBR) pvsshiftset,
   (SUBR) pvsshift},
#endif
  {"pvsmix", sizeof(PVSMIX), 3, "f", "ff", (SUBR) pvsmixset, (SUBR) pvsmix, NULL},
#ifndef OLPC
  {"pvsfilter", sizeof(PVSFILTER), 3, "f", "ffxp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
#else
  {"pvsfilter", sizeof(PVSFILTER), 3, "f", "ffkp", (SUBR) pvsfilterset,
   (SUBR) pvsfilter},
#endif
  {"pvsblur", sizeof(PVSBLUR), 3, "f", "fki", (SUBR) pvsblurset, (SUBR) pvsblur,
   NULL},
  {"pvstencil", sizeof(PVSTENCIL), 3, "f", "fkki", (SUBR) pvstencilset,
   (SUBR) pvstencil},
  {"pvsinit", sizeof(PVSINI), 1, "f", "ioopo", (SUBR) pvsinit, NULL, NULL},
#ifndef OLPC
  {"pvsbin", sizeof(PVSBIN), 3, "ss", "fk", (SUBR) pvsbinset,
   (SUBR) pvsbinprocess, (SUBR) pvsbinprocessa},
#else
  {"pvsbin", sizeof(PVSBIN), 3, "kk", "fk", (SUBR) pvsbinset,
   (SUBR) pvsbinprocess, NULL},
#endif
  {"pvsfreeze", sizeof(PVSFREEZE), 3, "f", "fkk", (SUBR) pvsfreezeset,
   (SUBR) pvsfreezeprocess, NULL},
#ifndef OLPC
  {"pvsmooth", sizeof(PVSFREEZE), 3, "f", "fxx", (SUBR) pvsmoothset,
   (SUBR) pvsmoothprocess, NULL},
#else
  {"pvsmooth", sizeof(PVSFREEZE), 3, "f", "fkk", (SUBR) pvsmoothset,
   (SUBR) pvsmoothprocess, NULL},
#endif
  {"pvsosc", sizeof(PVSOSC), 3, "f", "kkkioopo", (SUBR) pvsoscset,
   (SUBR) pvsoscprocess, NULL},
  {"pvsdiskin", sizeof(pvsdiskin), 3, "f", "SkkopP",(SUBR) pvsdiskinset,
   (SUBR) pvsdiskinproc, NULL},

  {"pvstanal", sizeof(PVST), 3, "f", "kkkkoo",(SUBR) pvstanalset,
   (SUBR) pvstanal, NULL}

};

int pvsbasic_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
			       (int) (sizeof(localops) / sizeof(OENTRY)));
}

