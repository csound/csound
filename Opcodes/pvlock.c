#include "csdl.h"

typedef struct dats{
  OPDS h;
  MYFLT *out, *time, *kamp, *kpitch, *knum, *klock, *iN, *idecim, *dbthresh;
  int cnt, hsize, curframe, N, decim, pos, tscale;
  MYFLT accum;
  AUXCH outframe, win, bwin, fwin, nwin, prev, framecount;
} DATASPACE;


static int sinit(CSOUND *csound, DATASPACE *p)
{

    int N =  *p->iN, i,size;
    int decim = *p->idecim;

    if(N) {
      for(i=0; N; i++){
        N >>= 1;
      }
      N = (int) pow(2., i-1.);
    } else N = 2048;
    if(decim == 0) decim = 4;

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
    return OK;
}


static int sprocess(CSOUND *csound, DATASPACE *p) {

    MYFLT pitch = *p->kpitch, *time = p->time, lock = *p->klock,
          *out = p->out, amp =*p->kamp;
    MYFLT *tab, frac,scale;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt;
    int ksmps = csound->GetKsmps(csound), n;
    int size, post, i, j, spos = p->pos;
    double pos;
    MYFLT *fwin = (MYFLT *)p->fwin.auxp, *bwin = (MYFLT *)p->bwin.auxp, in,
      *prev = (MYFLT *)p->prev.auxp, *win = (MYFLT *) p->win.auxp;
    MYFLT *outframe= (MYFLT *) p->outframe.auxp;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt  = (int *)p->framecount.auxp;
    int curframe = p->curframe, decim = p->decim;

    for(n=0; n < ksmps; n++) {

      if(cnt == hsize){
        /* audio samples are stored in a function table */
        ft = csound->FTnp2Find(csound,p->knum);
        tab = ft->ftable;
        size = ft->flen;
        /* spos is the reading position in samples, hsize is hopsize,
           time[n] is current read position in secs
           esr is sampling rate
        */
        spos  = hsize*(int)(time[n]*csound->esr/hsize);
        while(spos > size) spos -= size;
        while(spos <= 0)  spos += size;
        pos = spos;
        /* this loop fills two frames/windows with samples from table,
           reading is linearly-interpolated,
           frames are separated by 1 hopsize
        */
        for(i=0; i < N; i++) {
          /* front window, fwin */
          post = (int) pos;
          frac = pos  - post;
          if(post >= 0 && post < size)
            in = tab[post] + frac*(tab[post+1] - tab[post]);
          else in =  (MYFLT) 0;
          fwin[i] = in * win[i]; /* window it */
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
        bwin[N] = bwin[1];
        bwin[N+1] = 0.0;
        csound->RealFFT(csound, fwin, N);
        fwin[N] = fwin[1];
        fwin[N+1] = 0.0;

        /* phase vocoder processing */

        for(i=0; i < N + 2; i+=2) {
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

        for(i=0; i < N + 2; i+=2) {
          if(lock) {  /* phase-locking */
            if(i > 0) {
              if(i < N){
                tmp_real = bwin[i] + bwin[i-2] + bwin[i+2];
                tmp_im = bwin[i+1] + bwin[i-1] + bwin[i+3];
              }
              else { /* Nyquist */
                tmp_real = bwin[i] + bwin[i-2];
                tmp_im = (MYFLT) 0;
              }
            }
            else { /* 0 Hz */
              tmp_real = bwin[i] + bwin[i+2];
              tmp_im = (MYFLT) 0;
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
        for(i=0;i<N;i++) outframe[framecnt[curframe]+i] = win[i]*fwin[i];
        cnt=0;
        curframe++;
        if(curframe == decim) curframe = 0;
      }
      out[n] = (MYFLT)0;
      /* write output */
      for(i = 0; i < decim; i++){
        out[n] += outframe[framecnt[i]];
        framecnt[i]++;
      }
      /* scale output */
      out[n] *= amp*(2./3.);
      cnt++;
    }

    p->cnt = cnt;
    p->curframe = curframe;
    return OK;

}

static int sinit2(CSOUND *csound, DATASPACE *p){

    int size;
    sinit(csound, p);
    size = p->N*sizeof(MYFLT);
    if (p->nwin.auxp == NULL || p->nwin.size < size)
      csound->AuxAlloc(csound, size, &p->nwin);
    p->pos = 0;
    p->tscale  = 0;
    p->accum = 0;
    return OK;
}

static int sprocess2(CSOUND *csound, DATASPACE *p) {

    MYFLT pitch = *p->kpitch, time = *p->time, lock = *p->klock;
    MYFLT *out = p->out, amp =*p->kamp;
    MYFLT *tab,frac,scale,  dbtresh = *p->dbthresh;
    FUNC *ft;
    int N = p->N, hsize = p->hsize, cnt = p->cnt;
    int  ksmps = csound->GetKsmps(csound), n;
    int size, post, i, j, spos = p->pos;
    double pos;
    MYFLT *fwin = (MYFLT *)p->fwin.auxp, *bwin = (MYFLT *)p->bwin.auxp;
    MYFLT in, *nwin = (MYFLT *)p->nwin.auxp, *prev = (MYFLT *)p->prev.auxp;
    MYFLT *win = (MYFLT *) p->win.auxp, *outframe= (MYFLT *) p->outframe.auxp;
    MYFLT powrat;
    MYFLT ph_real, ph_im, tmp_real, tmp_im, div;
    int *framecnt  = (int *)p->framecount.auxp, curframe = p->curframe;
    int decim = p->decim;

    for(n=0; n < ksmps; n++) {

      if(cnt == hsize){
        ft = csound->FTnp2Find(csound,p->knum);
        tab = ft->ftable;
        size = ft->flen;
        if(time < 0) {
          spos += hsize*time;
        }
        else if(p->tscale) {
          spos += hsize*time;
          //if(p->accum)p->accum-=time;
        }
        else  {
          spos += hsize;
          //p->accum+=time;
        }
        while(spos > size) spos -= size;
        while(spos <= 0)  spos += size;
        pos = spos;
        for(i=0; i < N; i++) {
          post = (int) pos;
          frac = pos  - post;
          if(post >= 0 && post < size)
            in = tab[post] + frac*(tab[post+1] - tab[post]);
          else in =  (MYFLT) 0;
          fwin[i] = in * win[i];
          post = (int) (pos - hsize*pitch);
          if(post >= 0 && post < size)
            in =  tab[post] + frac*(tab[post+1] - tab[post]);
          else in =  (MYFLT) 0;
          bwin[i] = in * win[i];
          post = (int) pos + hsize;
          if(post >= 0 && post < size) in =  tab[post];
          else in =  (MYFLT) 0;
          nwin[i] = in * win[i];
          pos += pitch;
        }

        csound->RealFFT(csound, bwin, N);
        bwin[N] = bwin[1];
        bwin[N+1] = 0.;
        csound->RealFFT(csound, fwin, N);
        csound->RealFFT(csound, nwin, N);

        tmp_real = tmp_im = (MYFLT) 1e-20;
        for(i=2; i < N; i++){
          tmp_real += nwin[i]*nwin[i] + nwin[i+1]*nwin[i+1];
          tmp_im += fwin[i]*fwin[i] + fwin[i+1]*fwin[i+1];
        }
        powrat = FL(20.0)*LOG10(tmp_real/tmp_im);
        if(powrat > dbtresh) p->tscale=0;
        else p->tscale=1;

        fwin[N] = fwin[1];
        fwin[N+1] = FL(0.0);

        for(i=0; i < N + 2; i+=2) {

          div =  FL(1.0)/(HYPOT(prev[i], prev[i+1]) + 1.0e-20);
          ph_real  =    prev[i]*div;
          ph_im =       prev[i+1]*div;

          tmp_real =   bwin[i] * ph_real + bwin[i+1] * ph_im;
          tmp_im =   bwin[i] * ph_im - bwin[i+1] * ph_real;
          bwin[i] = tmp_real;
          bwin[i+1] = tmp_im;
        }

        for(i=0; i < N + 2; i+=2) {
          if(lock) {
            if(i > 0) {
              if(i < N){
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
          } else {
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
      out[n] *= amp*(2.0/3.0);
      cnt++;
    }


    p->cnt = cnt;
    p->curframe = curframe;
    p->pos = spos;
    return OK;

}

static OENTRY localops[] = {
{"mincer", sizeof(DATASPACE), 5, "a", "akkkkoo", (SUBR)sinit, NULL,(SUBR)sprocess },
{"temposcal", sizeof(DATASPACE), 5, "a", "kkkkkoop",
                                               (SUBR)sinit2, NULL,(SUBR)sprocess2 },
};

LINKAGE
