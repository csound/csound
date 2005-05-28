
/*  sndloop.c sndloop flooper pvsarp pvsvoc

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

/*
SNDLOOP

asig, krec  sndloop  ain, kpitch, ktrig, idur, ifad

A sound looper with pitch control.

INIT

idur - loop duration in seconds
ifad - crossfade duration in seconds

PERFORMANCE

asig - output signal
krec - 'rec on' signal, 1 when recording, 0 otherwise
kpitch - pitch control (transposition ratio)
kon - on signal: when 0, processing is bypassed. When switched on (kon >= 1),
the opcode starts recording until the loop memory is full. It then plays
the looped sound until it is switched off again (kon = 0). Another recording
can start again with kon >= 1.

FLOOPER

asig flooper kamp, kpitch, istart, idur, ifad, ifn

Function-table crossfading looper.

INIT

istart - starting point of loop (in secs)
idur - loop duration (secs)
ifad - crossfade duration (secs)

PERFORMANCE

asig - output signal
kamp - amplitude scaling
kpitch - pitch control (transposition ratio)

PVSARP

fsig pvsarp fin, kcf, kdepth, kgain

Spectral arpeggiator

PERFORMANCE

fin - input pv streaming signal
kcf - centre freq of arpeggiation (normalised 0 - 1.0, corresponding to 0 - Nyquist)
kdepth - depth of attenuation of surrounding frequency bins
kgain - gain applied to the bin at the centre frequency

PVSVOC

fsig pvsvoc fenv,fexc,kdepth, kgain

Applies the spectral envelope of one sound to the frequencies (excitation) of
another.

PERFORMANCE

fenv - spectral envelope signal
fexc - excitation signal
kdepth - depth of effect (0-1)
kgain - signal gain

*/

#include "csdl.h"
#include "pstream.h"

typedef struct _sndloop {
    OPDS h;
    MYFLT *out, *recon;  /* output, record on */
    MYFLT *sig, *pitch, *on, *dur, *cfd;  /* in, pitch, sound on, duration, crossfade  */
    AUXCH buffer; /* loop memory */
    long  wp;     /* writer pointer */
    MYFLT rp;     /* read pointer  */
    long  cfds;   /* crossfade in samples */
    long durs;    /* duration in samples */
    int  rst;     /* reset indicator */
    MYFLT  inc;     /* fade in/out increment/decrement */
    MYFLT  a;       /* fade amp */
} sndloop;

typedef struct _flooper {
    OPDS h;
    MYFLT *out;  /* output */
    MYFLT *amp, *pitch, *start, *dur, *cfd, *ifn;
    AUXCH buffer; /* loop memory */
    FUNC  *sfunc;  /* function table */
    long strts;   /* start in samples */
    long  durs;    /* duration in samples */
    MYFLT  ndx;    /* table lookup ndx */
    int   loop_off;
} flooper;

typedef struct _pvsarp {
    OPDS h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    MYFLT   *cf;
    MYFLT   *kdepth;
    MYFLT   *gain;
    unsigned long   lastframe;
}
pvsarp;

typedef struct _pvsvoc {
    OPDS h;
    PVSDAT  *fout;
    PVSDAT  *fin;
    PVSDAT  *ffr;
    MYFLT   *kdepth;
    MYFLT   *gain;
    unsigned long   lastframe;
}
pvsvoc;

int sndloop_init(ENVIRON *csound, sndloop *p){

    p->durs = (long) (*(p->dur)*csound->esr); /* dur in samps */
    p->cfds = (long) (*(p->cfd)*csound->esr); /* fade in samps */
    p->inc =  (MYFLT)1/p->cfds;    /* inc/dec */
    p->a  = (MYFLT) 0;
    p->wp = 0;   /* intialise write pointer */
    p->rst = 1;       /* reset the rec control */
    if(p->buffer.auxp==NULL)   /* allocate memory if necessary */
      csound->AuxAlloc(csound, p->durs*sizeof(MYFLT), &p->buffer);
    return OK;
}

int sndloop_process(ENVIRON *csound, sndloop *p){

    int i, on = (int) *(p->on), recon, n = csound->ksmps;
    long durs = p->durs, cfds = p->cfds, wp = p->wp;
    MYFLT rp = p->rp, a = p->a, inc = p->inc;
    MYFLT *out = p->out, *sig = p->sig, *buffer = p->buffer.auxp;
    MYFLT pitch = *(p->pitch);

    if(on) recon = p->rst; /* restart recording if switched on again */
    else recon = 0;  /* else do not record */

    for(i=0; i < n; i++){
      if(recon){ /* if the recording is ON */
        /* fade in portion */
        if(wp < cfds){
          buffer[wp] = sig[i]*a;
          a += inc;
        }
        else {
          if(wp >= durs){ /* fade out portion */
            buffer[wp-durs] += sig[i]*a;
            a -= inc;
          }
          else buffer[wp] = sig[i];  /* middle of loop */
        }
        /* while recording connect input to output directly */
        out[i] = sig[i];
        wp++; /* increment writer pointer */
        if(wp == durs+cfds){  /* end of recording */
          recon = 0;  /* OFF */
          p->rst = 0; /* reset to 0 */
          p->rp = (MYFLT) wp; /* rp pointer to start from here */
        }
      }
      else {
        if(on){ /* if opcode is ON */
          out[i] = buffer[(int)rp]; /* output the looped sound */
          rp += pitch;        /* read pointer increment */
          while(rp >= durs) rp -= durs; /* wrap-around */
          while(rp < 0) rp += durs;
        }
        else {   /* if opocde is OFF */
          out[i] = sig[i]; /* copy input to the output */
          p->rst = 1;   /* reset: ready for new recording */
          wp = 0; /* zero write pointer */
        }
      }
    }
    p->rp = rp; /* keep the values */
    p->wp = wp;
    p->a = a;
    *(p->recon) = (MYFLT) recon; /* output 'rec on light' */

    return OK;
}

int flooper_init(ENVIRON *csound, flooper *p){

    MYFLT *tab, *buffer, a = (MYFLT) 0, inc;
    long cfds = (long) (*(p->cfd)*csound->esr);     /* fade in samps  */
    long starts = (long) (*(p->start)*csound->esr); /* start in samps */
    long durs = (long)  (*(p->dur)*csound->esr);    /* dur in samps   */
    long len, i;

    if(cfds > durs){
      csound->InitError(csound, "crossfade longer than loop duration\n");
      return NOTOK;
    }

    inc =  (MYFLT)1/cfds;    /* inc/dec */
    p->sfunc = csound->FTnp2Find(csound, p->ifn);  /* function table */
    if(p->sfunc==NULL){
      csound->InitError(csound,"function table not found\n");
      return NOTOK;
    }
    tab = p->sfunc->ftable,  /* func table pointer */
    len = p->sfunc->flen;    /* function table length */
    if(starts > len){
      csound->InitError(csound,"start time beyond end of table\n");
      return NOTOK;
    }

    if(starts+durs+cfds > len){
      csound->InitError(csound,"table not long enough for loop\n");
      return NOTOK;
    }

    if(p->buffer.auxp==NULL)   /* allocate memory if necessary */
      csound->AuxAlloc(csound,(durs+1)*sizeof(float), &p->buffer);

    inc = (MYFLT)1/cfds;       /* fade envelope incr/decr */
    buffer = p->buffer.auxp;   /* loop memory */

    /* we now write the loop into memory */
    for(i=0; i < durs; i++){
      if(i < cfds){
        buffer[i] = a*tab[i+starts];
        a += inc;
      }
      else buffer[i] = tab[i+starts];;
       }
    /*  crossfade section */
    for(i=0; i  < cfds; i++){
      buffer[i] += a*tab[i+starts+durs];
      a -= inc;
    }

    buffer[durs] = buffer[0]; /* for wrap-around interpolation */
    p->strts = starts;
    p->durs = durs;
    p->ndx = (MYFLT) 0;      /* lookup index */
    p->loop_off  = 0;
    return OK;
}

int flooper_process(ENVIRON *csound, flooper *p){

    int i, n = csound->ksmps;
    long end = p->strts+p->durs, durs = p->durs;
    MYFLT *out = p->out, *buffer = p->buffer.auxp;
    MYFLT amp = *(p->amp), pitch = *(p->pitch);
    MYFLT *tab = p->sfunc->ftable, ndx = p->ndx, frac;
    int tndx, loop_off = p->loop_off;

    for(i=0; i < n; i++){
          tndx = (int) ndx;
          frac = ndx - tndx;
      /* this is the start portion of the sound */
      if(ndx >= 0  && ndx < end && loop_off) {
        out[i] = amp*(tab[tndx] + frac*(tab[tndx+1] - tab[tndx]));
        ndx += pitch;
      }
      /* this is the loop section */
      else {
        if(loop_off) {
          ndx -= end;
          tndx -= end;
          /* wrap-around, if reading backwards */
          while (tndx < 0) tndx += durs;
        }
        loop_off = 0;
        out[i] = amp*(buffer[tndx] + frac*(buffer[tndx+1] - buffer[tndx]));
        ndx += pitch;
        while (ndx < 0) ndx += durs;
        while (ndx >= durs) ndx -= durs;

      }

    }
    p->ndx = ndx;
    p->loop_off = loop_off;
    return OK;
}

int pvsarp_init(ENVIRON *csound, pvsarp *p){
    long N = p->fin->N;

    if (p->fout->frame.auxp==NULL)
      csound->AuxAlloc(csound,(N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format==PVS_AMP_FREQ) || (p->fout->format==PVS_AMP_PHASE)){
     return csound->InitError(csound, "pvsarp: signal format must be amp-phase or amp-freq.\n");
    }

    return OK;
}

int pvsarp_process(ENVIRON *csound, pvsarp *p)
{
    long i,j,N = p->fout->N, bins = N/2 + 1;
    float g = (float) *p->gain;
    MYFLT kdepth = (MYFLT) *(p->kdepth), cf = (MYFLT) *(p->cf);
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if(fout==NULL)
       return csound->PerfError(csound,"pvsarp: not initialised\n");

    if(p->lastframe < p->fin->framecount) {
    cf = cf >= 0 ? (cf < bins ? cf*bins : bins-1) : 0;
      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : (MYFLT)1.0): (MYFLT)0.0;
      for(i=j=0;i < N+2;i+=2, j++) {
        if(j == (int) cf) fout[i] = fin[i]*g;
        else fout[i] = (float)(fin[i]*(1-kdepth));
        fout[i+1] = fin[i+1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
}

int pvsvoc_init(ENVIRON *csound, pvsvoc *p){
    long N = p->fin->N;

    if (p->fout->frame.auxp==NULL)
      csound->AuxAlloc(csound,(N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format==PVS_AMP_FREQ) || (p->fout->format==PVS_AMP_PHASE)){
     return csound->InitError(csound, "pvsvoc: signal format must be amp-phase or amp-freq.\n");
    }

    return OK;
}

int pvsvoc_process(ENVIRON *csound, pvsvoc *p)
{
    long i,N = p->fout->N;
    float g = (float) *p->gain;
    MYFLT kdepth = (MYFLT) *(p->kdepth);
    float *fin = (float *) p->fin->frame.auxp;
    float *ffr = (float *) p->ffr->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if(fout==NULL)
       return csound->PerfError(csound,"pvsarp: not initialised\n");

    if(p->lastframe < p->fin->framecount) {

      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : (MYFLT)1.0): (MYFLT)0.0;
      for(i=0;i < N+2;i+=2) {
        fout[i] = fin[i]*g;
        fout[i+1] = ffr[i+1]*(kdepth) + fin[i+1]*(1-kdepth);
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
}

static OENTRY localops[] = {
    {"sndloop", sizeof(sndloop), 5,
    "ak", "akkii", (SUBR)sndloop_init, 0 ,
    (SUBR)sndloop_process},
    {"flooper", sizeof(flooper), 5,
    "a", "kkiiii", (SUBR)flooper_init, 0 ,
    (SUBR)flooper_process},
    {"pvsarp", sizeof(pvsarp), 3, "f", "fkkk", (SUBR)pvsarp_init,
      (SUBR)pvsarp_process},
    {"pvsvoc", sizeof(pvsarp), 3, "f", "ffkk", (SUBR)pvsvoc_init,
      (SUBR)pvsvoc_process},
};

LINKAGE

