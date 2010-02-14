#include "csdl.h"
#include "csdl.h"

typedef struct dats{
  OPDS h;
  MYFLT *out, *ktime, *kamp, *kpitch, *knum, *klock, *iN, *idecim;
  int cnt, hsize, curframe, N, decim, pos;
  AUXCH outframe, win, bwin, fwin, prev, framecount;
} DATASPACE;


static int sinit(CSOUND *csound, DATASPACE *p){
  
  int N =  *p->iN, i,size;
  int decim = *p->idecim;

  if(N) {
  for(i=0; N; i++){ 
    N >>= 1;
  }
   N = (int) pow(2., i-1.); 
  } else N = 2048;
  if(decim == 0) decim = 4;

  csound->Message(csound,"%d: %d\n", N, decim);
   
   p->hsize = N/decim;
   p->cnt = p->hsize;
   p->curframe = 0;

  size = (N+2)*sizeof(MYFLT);
  if (p->fwin.auxp == NULL || p->fwin.size < size)
    csound->AuxAlloc(csound, size, &p->fwin);  
  if (p->bwin.auxp == NULL || p->bwin.size < size)
      csound->AuxAlloc(csound, size, &p->bwin);
  if (p->prev.auxp == NULL || p->prev.size < size)
      csound->AuxAlloc(csound, size, &p->prev); 
  size = N*sizeof(MYFLT);
  if (p->win.auxp == NULL || p->win.size < size)
      csound->AuxAlloc(csound, size, &p->win); 
  size = decim*sizeof(int);
  if (p->framecount.auxp == NULL || p->framecount.size < size)
      csound->AuxAlloc(csound, size, &p->framecount);
  memset(p->framecount.auxp,0,size);
  size = decim*sizeof(MYFLT)*N;
  if (p->outframe.auxp == NULL || p->outframe.size < size)
    csound->AuxAlloc(csound, size, &p->outframe);  
  memset(p->outframe.auxp,0,size);

  for(i=0; i < N; i++) 
    ((MYFLT *)p->win.auxp)[i] = 0.5 - 0.5*cos(i*2*PI/N);

  p->N = N;
  p->decim = decim;
  p->pos =0;
  return OK;
}


static int sprocess(CSOUND *csound, DATASPACE *p) {

  MYFLT pitch = *p->kpitch, time = *p->ktime, lock = *p->klock, *out = p->out, amp =*p->kamp;
  MYFLT *tab, frac,scale;
  FUNC *ft;
  int N = p->N, hsize = p->hsize, cnt = p->cnt, ksmps = csound->GetKsmps(csound), n;
  int size, post, i, j, spos = p->pos;
  double pos;
  MYFLT *fwin = (MYFLT *)p->fwin.auxp, *bwin = (MYFLT *)p->bwin.auxp, in,
    *prev = (MYFLT *)p->prev.auxp, *win = (MYFLT *) p->win.auxp, *outframe= (MYFLT *) p->outframe.auxp;
  MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
  int *framecnt  = (int *)p->framecount.auxp, curframe = p->curframe, decim = p->decim;

  for(n=0; n < ksmps; n++) {

    if(cnt == hsize){
      ft = csound->FTnp2Find(csound,p->knum);
      tab = ft->ftable;
      size = ft->flen;
      spos  = hsize*(int)(time*csound->esr/hsize);
      while(spos > size) spos -= size;
      while(spos <= 0)  spos += size;
      pos = spos; 
      for(i=0; i < N; i++) {
	post = (int) pos; 
	frac = pos  - post;
	if(post >= 0 && post < size) in = tab[post] + frac*(tab[post+1] - tab[post]);
	else in =  (MYFLT) 0;
	fwin[i] = in * win[i];
	post = (int) pos - hsize;
	if(post >= 0 && post < size) in =  tab[post] + frac*(tab[post+1] - tab[post]);
	else in =  (MYFLT) 0;
	bwin[i] = in * win[i]; 
	pos += pitch;
      }
      
      csound->RealFFT(csound, bwin, N);
      bwin[N] = bwin[1];
      bwin[N+1] = 0.; 
      csound->RealFFT(csound, fwin, N);
      
      fwin[N] = fwin[1];
      fwin[N+1] = 0.; 
      
       for(i=0; i < N + 2; i+=2) {

	div =  1./(sqrt(prev[i]*prev[i] + prev[i+1]*prev[i+1]) + 1e-20); 
	ph_real  =    prev[i]*div;
	ph_im =       prev[i+1]*div;

	tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
	tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
        bwin[i] = tmp_real;
        bwin[i+1] = tmp_im;

	if(lock) {
	  if(i > 0) {
	    if(i < N){
	      tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
	      tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
	    }
	    else {
	      tmp_real = bwin[i] + bwin[i-2];
	      tmp_im = (MYFLT) 0;
	    }
	  }    
	  else {
	    tmp_real = bwin[i] + bwin[i+2];
	    tmp_im = (MYFLT) 0;
	  }
	} else {
	  tmp_real = bwin[i];
	  tmp_im = bwin[i+1];
	}

	tmp_real += 1e-15;  
	tmp_im += 1e-15;  
	div =  1./(sqrt(tmp_real*tmp_real + tmp_im*tmp_im ) + 1e-20); 

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
      
      for(i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];
      cnt=0;
      curframe++;
      if(curframe == decim) curframe = 0; 
    }
    out[n] = (MYFLT)0;
    for(i = 0; i < decim; i++){
      out[n] += outframe[framecnt[i]];
      framecnt[i]++;
    }
    out[n] *= amp;
    cnt++;
  }

  
  p->cnt = cnt;
  p->curframe = curframe;
  p->pos = spos;
  return OK;

}

static OENTRY localops[] = {
{"mincer", sizeof(DATASPACE), 5, "a", "kkkkkoo", (SUBR)sinit, NULL,(SUBR)sprocess },
};

LINKAGE
