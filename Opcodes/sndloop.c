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

FLOOPER2

asig flooper2 kamp, kpitch, kloopstart, kloopend, kcrossfade,
              ifn [, istart, imode, ifenv]

Function-table crossfading looper with variable loop parameters and
different looping modes.

INIT

ifn - sound source function table. Non-power-of-two and deferred allocation
tables are allowed.
istart - playback starting point in secs, only applicable to loop modes 0 & 2
         [def:0]
imode - loop modes: 0 forward, 1 backward, 2 back-and-forth [def: 0]
ifenv - if non-zero, crossfade envelope shape table number. 0, the default, sets
the crossfade to linear.

PERFORMANCE

kamp - amplitude scaling
kpitch - playback pitch ratio (1 - normal, > 1 faster, < 1 slower). Negative
ratios are not allowed.
kloopstart - loop start point (secs). Note that although k-rate, loop parameters
such as this are only updated once per loop cycle.
kloopend - loop end point (secs), updated once per loop cycle.
kcrossfade - crossfade length (secs), updated once per loop cycle and limited to
loop length.


PVSARP

fsig pvsarp fin, kcf, kdepth, kgain

Spectral arpeggiator

PERFORMANCE

fin - input pv streaming signal
kcf - centre freq of arpeggiation (normalised 0 - 1.0,
corresponding to 0 - Nyquist)
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

#include "stdopcod.h"
#include "pstream.h"

typedef struct _sndloop {
  OPDS h;
  MYFLT *out, *recon;       /* output,record on */
  MYFLT *sig, *pitch, *on;  /* in, pitch, sound on */
  MYFLT *dur, *cfd;         /* duration, crossfade  */
  AUXCH buffer;             /* loop memory */
  int32  wp;                /* writer pointer */
  double rp;                /* read pointer  */
  int32  cfds;              /* crossfade in samples */
  int32 durs;               /* duration in samples */
  int  rst;                 /* reset indicator */
  MYFLT inc;                /* fade in/out increment/decrement */
  MYFLT  a;                 /* fade amp */
} sndloop;

typedef struct _flooper {
  OPDS h;
  MYFLT *out[2];  /* output */
  MYFLT *amp, *pitch, *start, *dur, *cfd, *ifn;
  AUXCH buffer; /* loop memory */
  FUNC  *sfunc;  /* function table */
  int32 strts;   /* start in samples */
  int32  durs;   /* duration in samples */
  double  ndx;   /* table lookup ndx */
  int nchnls;
  int   loop_off;
} flooper;


typedef struct _flooper2 {
  OPDS h;
  MYFLT *out[2];  /* output */
  MYFLT *amp, *pitch, *loop_start, *loop_end,
    *crossfade, *ifn, *start, *imode, *ifn2, *iskip, *ijump;
  FUNC  *sfunc;  /* function table */
  FUNC *efunc;
  MYFLT count;
  int lstart, lend,cfade, mode;
  double  ndx[2];    /* table lookup ndx */
  int firsttime, init;
  MYFLT ostart, oend;
  int nchnls;
} flooper2;


typedef struct _flooper3 {
  OPDS h;
  MYFLT *out;  /* output */
  MYFLT *amp, *pitch, *loop_start, *loop_end,
    *crossfade, *ifn, *start, *imode, *ifn2, *iskip;
  FUNC  *sfunc;  /* function table */
  FUNC *efunc;
  int32 count;
  int lstart, lend,cfade, mode;
  int32  ndx[2];    /* table lookup ndx */
  int firsttime, init;
  int lobits,lomask;
  MYFLT lodiv;
} flooper3;

typedef struct _pvsarp {
  OPDS h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  MYFLT   *cf;
  MYFLT   *kdepth;
  MYFLT   *gain;
  uint32   lastframe;
} pvsarp;

typedef struct _pvsvoc {
  OPDS h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  PVSDAT  *ffr;
  MYFLT   *kdepth;
  MYFLT   *gain;
  MYFLT   *kcoefs;
  AUXCH   fenv, ceps, fexc;
  uint32   lastframe;
} pvsvoc;

typedef struct _pvsmorph {
  OPDS h;
  PVSDAT  *fout;
  PVSDAT  *fin;
  PVSDAT  *ffr;
  MYFLT   *kdepth;
  MYFLT   *gain;
  uint32   lastframe;
} pvsmorph;

static int sndloop_init(CSOUND *csound, sndloop *p)
{
    p->durs = (int32) (*(p->dur)*CS_ESR); /* dur in samps */
    p->cfds = (int32) (*(p->cfd)*CS_ESR); /* fade in samps */
    if (UNLIKELY(p->durs < p->cfds))
      return
        csound->InitError(csound, Str("crossfade cannot be longer than loop\n"));

    p->inc  = FL(1.0)/p->cfds;    /* inc/dec */
    p->a    = FL(0.0);
    p->wp   = 0;                  /* intialise write pointer */
    p->rst  = 1;                  /* reset the rec control */
    if (p->buffer.auxp==NULL ||
       p->buffer.size<p->durs*sizeof(MYFLT)) /* allocate memory if necessary */
      csound->AuxAlloc(csound, p->durs*sizeof(MYFLT), &p->buffer);
    return OK;
}

static int sndloop_process(CSOUND *csound, sndloop *p)
{
    int on = (int) *(p->on), recon;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32 durs = p->durs, cfds = p->cfds, wp = p->wp;
    double rp = p->rp;
    MYFLT a = p->a, inc = p->inc;
    MYFLT *out = p->out, *sig = p->sig, *buffer = p->buffer.auxp;
    MYFLT pitch = *(p->pitch);

    if (on) recon = p->rst; /* restart recording if switched on again */
    else recon = 0;  /* else do not record */

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset; i < nsmps; i++) {
      if (recon) { /* if the recording is ON */
        /* fade in portion */
        if (wp < cfds) {
          buffer[wp] = sig[i]*a;
          a += inc;
        }
        else {
          if (wp >= durs) { /* fade out portion */
            buffer[wp-durs] += sig[i]*a;
            a -= inc;
          }
          else buffer[wp] = sig[i];  /* middle of loop */
        }
        /* while recording connect input to output directly */
        out[i] = sig[i];
        wp++; /* increment writer pointer */
        if (wp == durs+cfds) {  /* end of recording */
          recon = 0;  /* OFF */
          p->rst = 0; /* reset to 0 */
          p->rp = (MYFLT) wp; /* rp pointer to start from here */
        }
      }
      else {
        if (on) { /* if opcode is ON */
          out[i] = buffer[(int)rp]; /* output the looped sound */
          rp += pitch;        /* read pointer increment */
          while (rp >= durs) rp -= durs; /* wrap-around */
          while (rp < 0) rp += durs;
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

static int flooper_init(CSOUND *csound, flooper *p)
{
    MYFLT *tab, *buffer, a = FL(0.0), inc;
    int32 cfds = (int32) (*(p->cfd)*CS_ESR);     /* fade in samps  */
    int32 starts = (int32) (*(p->start)*CS_ESR); /* start in samps */
    int32 durs = (int32)  (*(p->dur)*CS_ESR);    /* dur in samps   */
    int32 len, i, nchnls;

    if (UNLIKELY(cfds > durs))
      return csound->InitError(csound,
                               Str("crossfade longer than loop duration\n"));

    inc =  FL(1.0)/cfds;    /* inc/dec */
    p->sfunc = csound->FTnp2Find(csound, p->ifn);  /* function table */
    if (UNLIKELY(p->sfunc==NULL)) {
      return csound->InitError(csound,Str("function table not found\n"));
    }
    tab = p->sfunc->ftable,  /* func table pointer */
    len = p->sfunc->flen;    /* function table length */
    nchnls = p->sfunc->nchanls;
    if(nchnls != p->OUTCOUNT){
     return
       csound->InitError(csound,
                         Str("function table channel count does not match output"));
    }
    if (UNLIKELY(starts > len)) {
      return csound->InitError(csound,Str("start time beyond end of table\n"));
    }

    if (UNLIKELY(starts+durs+cfds > len)) {
      return csound->InitError(csound,Str("table not long enough for loop\n"));
    }

    if (p->buffer.auxp==NULL ||               /* allocate memory if necessary */
        p->buffer.size<(durs+1)*sizeof(MYFLT)*nchnls)
      csound->AuxAlloc(csound,(durs+1)*sizeof(MYFLT)*nchnls, &p->buffer);

    inc = (MYFLT)1/cfds;       /* fade envelope incr/decr */
    buffer = p->buffer.auxp;   /* loop memory */

    /* we now write the loop into memory */
    durs *= nchnls;
    starts *= nchnls;
    cfds *= nchnls;
    for (i=0; i < durs; i+=nchnls) {
      if (i < cfds) {
        buffer[i] = a*tab[i+starts];
        if(nchnls == 2) buffer[i+1] = a*tab[i+starts+1];
        a += inc;
      }
      else {
        buffer[i] = tab[i+starts];
        if(nchnls == 2) buffer[i+1] = tab[i+starts+1];
      }
    }
    /*  crossfade section */
    for (i=0; i  < cfds; i+=nchnls) {
      buffer[i] += a*tab[i+starts+durs];
      if(nchnls == 2) buffer[i+1] += a*tab[i+starts+durs+1];
      a -= inc;
    }

    buffer[durs] = buffer[0]; /* for wrap-around interpolation */
    p->strts     = starts/nchnls;
    p->durs      = durs/nchnls;
    p->ndx       = FL(0.0);   /* lookup index */
    p->loop_off  = 1;
    p->nchnls = nchnls;
    return OK;
}

static int flooper_process(CSOUND *csound, flooper *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32 end = p->strts+p->durs, durs = p->durs;
    MYFLT **aout = p->out, *buffer = p->buffer.auxp;
    MYFLT amp = *(p->amp), pitch = *(p->pitch);
    MYFLT *tab = p->sfunc->ftable;
    double ndx = p->ndx;
    MYFLT  frac;
    int tndx, loop_off = p->loop_off, nchnls = p->nchnls;

    if (UNLIKELY(offset)) memset(aout[0], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[0][nsmps], '\0', early*sizeof(MYFLT));
      if(nchnls == 2) {
        if (UNLIKELY(offset)) memset(aout[1], '\0', offset*sizeof(MYFLT));
        memset(&aout[1][nsmps], '\0', early*sizeof(MYFLT));
      }
    }

    for (i=offset; i < nsmps; i++) {
      tndx = (int) ndx;
      frac = ndx - tndx;
      /* this is the start portion of the sound */
      if (ndx >= 0  && ndx < end && loop_off) {
        tndx  *= nchnls;
        aout[0][i] = amp*(tab[tndx] + frac*(tab[tndx+nchnls] - tab[tndx]));
        if(nchnls == 2){
          tndx += 1;
          aout[1][i] = amp*(tab[tndx] + frac*(tab[tndx+nchnls] - tab[tndx]));
        }
        ndx += pitch;
      }
      /* this is the loop section */
      else {
        if (loop_off) {
          while(ndx >= end) ndx -= end;
          /* wrap-around, if reading backwards */
          while (tndx < 0) tndx += durs;
          tndx = (int) ndx;
        }
        loop_off = 0;
        tndx *= nchnls;
        aout[0][i] =
          amp*(buffer[tndx] + frac*(buffer[tndx+nchnls] - buffer[tndx]));
        if(nchnls == 2){
          tndx += 1;
          aout[1][i] =
            amp*(buffer[tndx] + frac*(buffer[tndx+nchnls] - buffer[tndx]));
        }
        ndx += pitch;
        while (ndx < 0) ndx += durs;
        while (ndx >= durs) ndx -= durs;

      }

    }
    p->ndx = ndx;
    p->loop_off = loop_off;
    return OK;
}

static int flooper2_init(CSOUND *csound, flooper2 *p)
{

    p->sfunc = csound->FTnp2Find(csound, p->ifn);  /* function table */
    if (UNLIKELY(p->sfunc==NULL)) {
      return csound->InitError(csound,Str("function table not found\n"));
    }
    if (*p->ifn2 != 0) p->efunc = csound->FTnp2Find(csound, p->ifn2);
    else p->efunc = NULL;

    if (*p->iskip == 0){
      p->mode = (int) *p->imode;
      if (p->mode == 0 || p->mode == 2){
        if ((p->ndx[0] = *p->start*CS_ESR) < 0)
          p->ndx[0] = 0;
        if (p->ndx[0] >= p->sfunc->flen)
          p->ndx[0] = (double) p->sfunc->flen - 1.0;
        p->count = 0;
      }
      p->init = 1;
      p->firsttime = 1;
      p->cfade = 1;
    }

    p->nchnls = (int)(p->OUTOCOUNT);
    if(p->nchnls != p->sfunc->nchanls){
      csound->Warning(csound,
       Str("function table channels do not match opcode outputs"));
    }
    return OK;
}

static int flooper2_process(CSOUND *csound, flooper2 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT out[2], **aout = p->out, sr = CS_ESR;
    MYFLT amp = *(p->amp), pitch = *(p->pitch);
    MYFLT *tab;
    double *ndx = p->ndx;
    MYFLT frac0, frac1, *etab;
    int loop_end = p->lend, loop_start = p->lstart,
      crossfade = p->cfade, len;
    MYFLT count = p->count, fadein, fadeout;
    int *firsttime = &p->firsttime, elen, mode=p->mode,
        init = p->init, ijump = *p->ijump;
    uint32 tndx0, tndx1, nchnls, onchnls = p->nchnls;
    FUNC *func;
    func = csound->FTnp2Find(csound, p->ifn);

    if(p->sfunc != func) {
    p->sfunc = func;
    if (UNLIKELY(func == NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("table %d invalid\n"), (int) *p->ifn);
    if (p->ndx[0] >= p->sfunc->flen)
       p->ndx[0] = (double) p->sfunc->flen - 1.0;

    if(p->nchnls != p->sfunc->nchanls){
       csound->Warning(csound,
          Str("function table channels do not match opcode outputs"));
      }
    }
    tab = p->sfunc->ftable;
    len = p->sfunc->flen;

    if (p->efunc != NULL) {
      etab = p->efunc->ftable;
      elen = p->efunc->flen;
    }
    else {
      etab = NULL;
      elen = 0;
    }

    /* loop parameters & check */
    if (pitch < FL(0.0)) pitch = FL(0.0);
    if (UNLIKELY(offset)) memset(aout[0], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&aout[0][nsmps], '\0', early*sizeof(MYFLT));
      if(onchnls == 2) {
        if (UNLIKELY(offset)) memset(aout[1], '\0', offset*sizeof(MYFLT));
        memset(&aout[1][nsmps], '\0', early*sizeof(MYFLT));
      }
    }

    if (*firsttime) {
      int loopsize;
      /* offset non zero only if firsttime */
      if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
      loop_start = (int) (*p->loop_start*sr);
      loop_end =   (int) (*p->loop_end*sr);
      p->lstart = loop_start = loop_start < 0 ? 0 : loop_start;
      p->lend = loop_end =   loop_end > len ? len :
        (loop_end < loop_start ? loop_start : loop_end);
      loopsize = loop_end - loop_start;
      if(*p->crossfade > 0.0)
       crossfade = (int) (*p->crossfade*sr);
      p->ostart = *p->loop_start; p->oend = *p->loop_end;
      if (mode == 1) {
        ndx[0] = (double) loop_end;
        ndx[1] = (double) loop_end;
        count = (MYFLT) crossfade;
        p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
      }
      else if (mode == 2) {
        ndx[1] = (double) loop_start - FL(1.0);
        p->cfade = crossfade = crossfade > loopsize/2 ? loopsize/2-1 : crossfade;

      }
      else {
        ndx[1] = (double) loop_start;
        p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
      }
      *firsttime = 0;
    }
    else {
      if (*p->ijump && (mode == 0) &&
         (p->ostart != *p->loop_start ||
          p->oend != *p->loop_end)) {
        if (*p->ijump > 1)
          loop_end = (int)(ndx[0] + crossfade);
        loop_start = *p->loop_start*sr;
        p->ostart = *p->loop_start;
        p->oend = *p->loop_end;
      }
    }
    onchnls = p->nchnls;
    nchnls = p->sfunc->nchanls;
    for (i=offset; i < nsmps; i++) {
      if (mode == 1){ /* backwards */
        tndx0 = (int) ndx[0];
        frac0 = ndx[0] - tndx0;
        if (ndx[0] > crossfade + loop_start) {
          tndx0 *= nchnls;
          out[0] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          if(onchnls == 2) {
            tndx0 += 1;
            out[1] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          }
        }
        else {
          tndx1 = (int) ndx[1];
          frac1 = ndx[1] - tndx1;
          if (etab==NULL){
            fadeout = count/crossfade;
            fadein = FL(1.0) - fadeout;
          }
          else {
            if(crossfade != FL(0.0))
             fadeout = elen*count/crossfade;
            else fadeout = 0.0;
            fadein = etab[elen - (int)fadeout];
            fadeout = etab[(int)fadeout];
          }
          tndx1 *= nchnls;
          tndx0 *= nchnls;
          out[0] = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                     tab[tndx0]))
                        + fadein*(tab[tndx1] + frac1*(tab[tndx1+nchnls] -
                                                      tab[tndx1])));
          if(onchnls == 2) {
          tndx1 += 1;
          tndx0 += 1;
          out[1] = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                     tab[tndx0]))
                        + fadein*(tab[tndx1] + frac1*(tab[tndx1+nchnls] -
                                                      tab[tndx1])));
          }
          ndx[1]-=pitch;
          count-=pitch;
        }
        ndx[0]-=pitch;

        if (ndx[0] <= loop_start) {
          int loopsize;
          loop_start = (int) (*p->loop_start*sr);
          loop_end =   (int) (*p->loop_end*sr);
          p->lstart = loop_start = loop_start < 0 ? 0 : loop_start;
          p->lend = loop_end =   loop_end > len ? len :
            (loop_end < loop_start ? loop_start : loop_end);
          loopsize = loop_end - loop_start;
          if(*p->crossfade > 0.0)
           crossfade = (int) (*p->crossfade*sr);
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
          ndx[0] = ndx[1];
          ndx[1] =  (double)loop_end;
          count=(MYFLT)crossfade;
          p->oend = *p->loop_end;
          p->ostart = *p->loop_start;
        }
      }
      else if (mode==2) { /* back and forth */
        out[0] = 0.0;
        if(onchnls == 2) out[1] = 0.0;
        /* this is the forward reader */
        if (init && ndx[0] < loop_start + crossfade) {
          tndx0 = (int) ndx[0];
          frac0 = ndx[0] - tndx0;
          tndx0 *= nchnls;
          out[0] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          if(onchnls == 2){
            tndx0 *= nchnls;
            out[1] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          }
          ndx[0] += pitch;
        }
        else if (ndx[0] < loop_start + crossfade) {
          if (etab==NULL) fadein = count/crossfade;
          else fadein = etab[(int)(elen*count/crossfade)];
          tndx0 = (int) ndx[0];
          frac0 = ndx[0] - tndx0;
          tndx0 *= nchnls;
          out[0] += amp*fadein*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                    tab[tndx0]));
          if(onchnls == 2){
            tndx0 += 1;
            out[1] += amp*fadein*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                      tab[tndx0]));
          }
          ndx[0] += pitch;
          count  += pitch;
        }
        else if (ndx[0] < loop_end - crossfade) {
          tndx0 = (int) ndx[0];
          frac0 = ndx[0] - tndx0;
          tndx0 *= nchnls;
          out[0] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          if(onchnls == 2){
           tndx0 += 1;
           out[1] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          }
          ndx[0] += pitch;
          init = 0;
          if (ndx[0] >= loop_end - crossfade) {
            ndx[1] = (double) loop_end;
            count = 0;
          }
        }
        else if (ndx[0] < loop_end) {
          if (etab==NULL) fadeout = FL(1.0) - count/crossfade;
          else  fadeout = etab[(int)(elen*(1.0 - count/crossfade))];
          tndx0 = (int) ndx[0];
          frac0 = ndx[0] - tndx0;
          tndx0 *= nchnls;
          out[0] += amp*fadeout*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                     tab[tndx0]));
          if(onchnls == 2){
           tndx0 += 1;
           out[1] += amp*fadeout*(tab[tndx0] + frac0*(tab[tndx0+nchnls] -
                                                      tab[tndx0]));
          }
          ndx[0] += pitch;
          count  += pitch;
        }
        /* this is the backward reader */
        if (ndx[1] > loop_end - crossfade) {
          if (etab==NULL) fadein = count/crossfade;
          else fadein = etab[(int)(elen*count/crossfade)];
          tndx1 = (int) ndx[1];
          frac1 = ndx[1] - tndx1;
          tndx1 *= nchnls;
          out[0] += amp*fadein*(tab[tndx1] + frac1*(tab[tndx1+nchnls] -
                                                    tab[tndx1]));
          if(onchnls == 2){
            tndx1 += 1;
            out[1] += amp*fadein*(tab[tndx1] + frac1*(tab[tndx1+nchnls] -
                                                      tab[tndx1]));
          }
          ndx[1] -= pitch;
        }
        else if (ndx[1] > loop_start + crossfade) {
          tndx1 = (int) ndx[1];
          frac1 = ndx[1] - tndx1;
          tndx1 *= nchnls;
          out[0] = amp*(tab[tndx1] + frac1*(tab[tndx1+nchnls] - tab[tndx1]));
         if(onchnls == 2){
            tndx1 += 1;
            out[1] += amp*(tab[tndx1] + frac1*(tab[tndx1+nchnls] - tab[tndx1]));
          }
          ndx[1] -= pitch;
          if (ndx[1] <= loop_start + crossfade) {
            ndx[0] = (double) loop_start;
            count = 0;
          }
        }
        else if (ndx[1] > loop_start) {
          if (etab==NULL) fadeout = FL(1.0) - count/crossfade;
          else fadeout = etab[(int)(elen*(1.0 - count/crossfade))];
          tndx1 = (int) ndx[1];
          frac1 = ndx[1] - tndx1;
          tndx1 *= nchnls;
          out[0] += amp*fadeout*(tab[tndx1] + frac1*(tab[tndx1+nchnls]
                                                     - tab[tndx1]));
         if(onchnls == 2){
            tndx1 += 1;
            out[1] += amp*fadeout*(tab[tndx1] + frac1*(tab[tndx1+nchnls]
                                                        - tab[tndx1]));
          }
          ndx[1] -= pitch;
          if (ndx[1] <= loop_start) {
            int loopsize;
            loop_start = (int) (*p->loop_start*sr);
            loop_end =   (int) (*p->loop_end*sr);
            p->lstart = loop_start = loop_start < 0 ? 0 : loop_start;
            p->lend = loop_end =   loop_end > len ? len :
              (loop_end < loop_start ? loop_start : loop_end);
            loopsize = loop_end - loop_start;
            if(*p->crossfade > 0.0)
              crossfade = (int) (*p->crossfade*sr);
            p->cfade = crossfade = crossfade > loopsize/2 ?
              loopsize/2-1 : crossfade;
            p->oend = *p->loop_end;
            p->ostart = *p->loop_start;
            ndx[0] = (double) loop_start;
            count = 0;
          }
        }
      }
      else {  /* normal */
        out[0] = 0;
        tndx0 = (uint32) ndx[0];
        frac0 = ndx[0] - tndx0;
        if (ndx[0] < loop_end-crossfade) {
          tndx0 *= nchnls;
          out[0] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          if(onchnls == 2){
            tndx0 += 1;
            out[1] = amp*(tab[tndx0] + frac0*(tab[tndx0+nchnls] - tab[tndx0]));
          }
          if (ijump) ndx[1] = loop_start;
        }
        else {
          tndx1 = (int) ndx[1];
          frac1 = ndx[1] - tndx1;
          if (etab==NULL) {
            fadein = count/crossfade;
            fadeout = FL(1.0) - fadein;
          }
          else {
            fadein = elen*count/crossfade;
            fadeout = etab[elen - (int)fadein];
            fadein = etab[(int)fadein];
          }
          tndx1 *= nchnls;
          tndx0 *= nchnls;
          out[0] = amp*(fadeout*(tab[tndx0] +
                                 frac0*(tab[tndx0+nchnls] - tab[tndx0]))
                        + fadein*(tab[tndx1] +
                                  frac1*(tab[tndx1+nchnls] - tab[tndx1])));
          if(onchnls == 2){
            tndx1 += 1;
            tndx0 += 1;
            out[1] = amp*(fadeout*(tab[tndx0] +
                                 frac0*(tab[tndx0+nchnls] - tab[tndx0]))
                        + fadein*(tab[tndx1] +
                                  frac1*(tab[tndx1+nchnls] - tab[tndx1])));
          }
          ndx[1]+=pitch;
          count+=pitch;
        }
        ndx[0]+=pitch;
        if (ndx[0] >= loop_end) {
          int loopsize;
          loop_start = (int) (*p->loop_start*sr);
          loop_end =   (int) (*p->loop_end*sr);
          p->lstart = loop_start = loop_start < 0 ? 0 : loop_start;
          p->lend = loop_end =   loop_end > len ? len :
            (loop_end < loop_start ? loop_start : loop_end);
          loopsize = loop_end - loop_start;
          if(*p->crossfade > 0.0)
            crossfade = (int) (*p->crossfade*sr);
          p->cfade = crossfade = crossfade > loopsize ? loopsize-1 : crossfade;
          ndx[0] = ndx[1];
          ndx[1] = (double)loop_start;
          p->oend = *p->loop_end;
          p->ostart = *p->loop_start;
          count=0;
        }
      }
      aout[0][i] = out[0];
      if(onchnls == 2) aout[1][i] = out[1];
    }

    p->count = count;
    p->cfade = crossfade;
    p->lend = loop_end;
    p->lstart = loop_start;
    p->init = init;
    return OK;
}


/*
static int flooper3_init(CSOUND *csound, flooper3 *p)
{
    int len,i,p2s,lomod;
    p->sfunc = csound->FTnp2Find(csound, p->ifn);
    if (UNLIKELY(p->sfunc==NULL)) {
      return csound->InitError(csound,Str("function table not found\n"));
    }
    if (*p->ifn2 != 0) p->efunc = csound->FTFind(csound, p->ifn2);
    else p->efunc = NULL;

    len = p->sfunc->flen;
    p->lobits = 0;
    for(i=1; i < len; i<<=1);
    p2s = i;
    for(;(i & MAXLEN)==0; p->lobits++, i<<=1);
    lomod = MAXLEN/p2s;
    p->lomask = lomod - 1;
    p->lodiv = 1.0/lomod;

    if (*p->iskip == 0){
      p->mode = (int) *p->imode;
      if (p->mode == 0 || p->mode == 2){
        if ((p->ndx[0] = *p->start*CS_ESR) < 0)
          p->ndx[0] = 0;
        if (p->ndx[0] >= p->sfunc->flen)
          p->ndx[0] = p->sfunc->flen - 1.0;
        p->count = 0;
      }
      p->init = 1;
      p->firsttime = 1;
      p->ndx[0] <<= p->lobits;

    }
    return OK;
}

static int flooper3_process(CSOUND *csound, flooper3 *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int lobits = p->lobits,si,ei;
    MYFLT *out = p->out, sr = CS_ESR;
    MYFLT amp = *(p->amp), pitch = *(p->pitch);
    MYFLT *tab = p->sfunc->ftable, cvt;
    int32 *ndx = p->ndx, lomask = p->lomask, pos;
    MYFLT frac0, frac1, *etab, lodiv = p->lodiv;
    int loop_end = p->lend, loop_start = p->lstart, mode = p->mode,
      crossfade = p->cfade, len = p->sfunc->flen, count = p->count;
    MYFLT fadein, fadeout;
    int *firsttime = &p->firsttime, elen, init = p->init;
    uint32 tndx0, tndx1;

    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (pitch < FL(0.0)) pitch = FL(0.0);
    if (*firsttime) {
      int loopsize;
      if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
      loop_start = MYFLT2LRND(*p->loop_start*sr);
      loop_end =   MYFLT2LRND (*p->loop_end*sr);
      p->lstart = loop_start = (loop_start < 0 ? 0 : loop_start);
      p->lend = loop_end =   (loop_end > len ? len :
                              (loop_end < loop_start ? loop_start : loop_end));
      loopsize = loop_end - loop_start;
      crossfade = MYFLT2LRND(*p->crossfade*sr);

      if (mode == 1) {
        ndx[0] = loop_end<<lobits;
        ndx[1] = loop_end<<lobits;
        count = crossfade<<lobits;
        p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
      }
      else if (mode == 2) {
        ndx[1] = (loop_start-1)<<lobits;
        p->cfade = crossfade = crossfade > loopsize/2 ? loopsize/2-1 : crossfade;
      }
      else {
        ndx[1] = loop_start<<lobits;
        p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
      }
      *firsttime = 0;
    }

    if (p->efunc != NULL) {
      etab = p->efunc->ftable;
      elen = p->efunc->flen;
    }
    else {
      etab = NULL;
      elen = 1;
    }
    cvt = (MYFLT )elen/p->cfade;
    si = MYFLT2LRND(pitch*(lomask));
    ei = MYFLT2LRND(pitch*(lomask));

    for (i=offset; i < nsmps; i++) {
      if (mode == 0){
        tndx0 = ndx[0]>>lobits;
        frac0 = (ndx[0] & lomask)*lodiv;
        if (tndx0 < loop_end-crossfade)
          out[i] = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
        else {
          tndx1 = ndx[1]>>lobits;
          frac1 = (ndx[1] & lomask)*lodiv;
          if (etab==NULL) {
            fadein = (count>>lobits)*cvt;
            fadeout = FL(1.0) - fadein;
          }
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadein = etab[pos];
            fadeout = etab[elen - pos];
          }
          out[i] = amp*(fadeout*(tab[tndx0] +
                                 frac0*(tab[tndx0+1] - tab[tndx0]))
                        + fadein*(tab[tndx1] +
                                  frac1*(tab[tndx1+1] - tab[tndx1])));

          ndx[1]+=si;
          count+=ei;
        }
        ndx[0]+=si;
        if (tndx0 >= loop_end) {
          int loopsize;
          loop_start = MYFLT2LRND(*p->loop_start*sr);
          loop_end =   MYFLT2LRND(*p->loop_end*sr);
          p->lstart = loop_start = (loop_start < 0 ? 0 : loop_start);
          p->lend = loop_end =   (loop_end > len ? len :
                                  (loop_end < loop_start ? loop_start : loop_end));
          loopsize = (loop_end - loop_start);
          crossfade =  MYFLT2LRND(*p->crossfade*sr);
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
          ndx[0] = ndx[1];
          ndx[1] = loop_start<<lobits;
          count=0;
          cvt = (MYFLT)elen/p->cfade;
        }
      }
      else if (mode == 1) {
        tndx0 = ndx[0]>>lobits;
        frac0 = (ndx[0] & lomask)*lodiv;
        if (tndx0 > crossfade + loop_start)
          out[i] = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
        else {
          tndx1 = ndx[1]>>lobits;
          frac1 = (ndx[1] & lomask)*lodiv;
          if (etab==NULL) {
            fadeout = (count>>lobits)*cvt;
            fadein = FL(1.0) - fadeout;
          }
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadeout = etab[pos];
            fadein = etab[elen - pos];
          }
          out[i] = amp*(fadeout*(tab[tndx0] +
                                 frac0*(tab[tndx0+1] - tab[tndx0]))
                        + fadein*(tab[tndx1] +
                                  frac1*(tab[tndx1+1] - tab[tndx1])));

          ndx[1]-=si;
          count-=ei;
        }
        ndx[0]-=si;

        if (tndx0 <= loop_start) {
          int loopsize;
          loop_start = MYFLT2LRND(*p->loop_start*sr);
          loop_end =   MYFLT2LRND(*p->loop_end*sr);
          p->lstart = loop_start = (loop_start < 0 ? 0 : loop_start);
          p->lend = loop_end =   (loop_end > len ? len :
                                  (loop_end < loop_start ? loop_start : loop_end));
          loopsize = (loop_end - loop_start);
          crossfade =  MYFLT2LRND(*p->crossfade*sr);
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
          ndx[0] = ndx[1];
          ndx[1] = loop_end<<lobits;
          count=crossfade<<lobits;
          cvt = (MYFLT)elen/p->cfade;
        }
      }
      else if (mode == 2){
        out[i] = 0;

        tndx0 = ndx[0]>>lobits;
        frac0 = (ndx[0] & lomask)*lodiv;
        if (init && tndx0 < loop_start + crossfade) {
          out[i] = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          ndx[0] += si;
        }
        else if (tndx0 < loop_start + crossfade) {
          if (etab==NULL) fadein = (count>>lobits)*cvt;
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadein = etab[pos];
          }
          out[i] += amp*fadein*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          ndx[0] += si;
          count  += ei;
        }
        else if (tndx0 < loop_end - crossfade) {
          out[i] = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          ndx[0] += si;
          init = 0;
          tndx0 = ndx[0]>>lobits;
          if (tndx0 >= loop_end - crossfade) {
            ndx[1] = loop_end<<lobits;
            count = 0;
          }
        }
        else if (tndx0 < loop_end) {
          if (etab==NULL) fadeout = FL(1.0) - (count>>lobits)*cvt;
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadeout = etab[elen - pos];
          }
          out[i] += amp*fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          ndx[0] += si;
          count  += ei;
        }


        tndx1 = ndx[1]>>lobits;
        frac1 = (ndx[1] & lomask)*lodiv;
        if (tndx1 > loop_end - crossfade) {
          if (etab==NULL) fadein = (count>>lobits)*cvt;
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadein = etab[pos];
          }
          out[i] += amp*fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
          ndx[1] -= si;
        }
        else if (tndx1 > loop_start + crossfade) {
          out[i] = amp*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
          ndx[1] -= si;
          tndx1 = ndx[1]>>lobits;
          if (tndx1 <= loop_start + crossfade) {
            ndx[0] = loop_start<<lobits;
            count = 0;
          }
        }
        else if (tndx1 > loop_start) {
          if (etab==NULL) fadeout = FL(1.0) - (count>>lobits)*cvt;
          else {
            pos = MYFLT2LRND((count>>lobits)*cvt);
            fadeout = etab[elen - pos];
          }
          out[i] += amp*fadeout*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
          ndx[1] -= si;
          tndx1 = ndx[1]>>lobits;
          if (tndx1 <= loop_start) {
            int loopsize;
            loop_start = MYFLT2LRND(*p->loop_start*sr);
            loop_end =   MYFLT2LRND(*p->loop_end*sr);
            p->lstart = loop_start = (loop_start < 0 ? 0 : loop_start);
            p->lend = loop_end =
              (loop_end > len ? len :
              (loop_end < loop_start ? loop_start : loop_end));
            loopsize = (loop_end - loop_start);
            crossfade =  MYFLT2LRND(*p->crossfade*sr);
            p->cfade = crossfade =
              crossfade > loopsize/2 ? loopsize/2-1 : crossfade;
            cvt = (MYFLT)elen/p->cfade;
          }
        }
      }
    }

    p->count = count;
    p->cfade = crossfade;
    p->lend = loop_end;
    p->lstart = loop_start;
    p->init = init;
    return OK;
}
*/

static int pvsarp_init(CSOUND *csound, pvsarp *p)
{
    int32 N = p->fin->N;

    if (p->fout->frame.auxp==NULL || p->fout->frame.size<(N+2)*sizeof(float))
      csound->AuxAlloc(csound,(N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (UNLIKELY(!(p->fout->format==PVS_AMP_FREQ) ||
                 (p->fout->format==PVS_AMP_PHASE))){
      return csound->InitError(csound,
                               Str("pvsarp: signal format must be amp-phase "
                                   "or amp-freq.\n"));
    }

    return OK;
}

static int pvsarp_process(CSOUND *csound, pvsarp *p)
{
    int32 i,j,N = p->fout->N, bins = N/2 + 1;
    float g = (float) *p->gain;
    MYFLT kdepth = (MYFLT) *(p->kdepth), cf = (MYFLT) *(p->cf);
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if (UNLIKELY(fout==NULL)) goto err1;

    if (p->lastframe < p->fin->framecount) {
      cf = cf >= 0 ? (cf < bins ? cf*bins : bins-1) : 0;
      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : FL(1.0)): FL(0.0);
      for (i=j=0;i < N+2;i+=2, j++) {
        if (j == (int) cf) fout[i] = fin[i]*g;
        else fout[i] = (float)(fin[i]*(1-kdepth));
        fout[i+1] = fin[i+1];
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsarp: not initialised\n"));
}

static int pvsvoc_init(CSOUND *csound, pvsvoc *p)
{
    int32 N = p->fin->N;

    if (p->fout->frame.auxp==NULL || p->fout->frame.size<(N+2)*sizeof(float))
      csound->AuxAlloc(csound,(N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (UNLIKELY(!(p->fout->format==PVS_AMP_FREQ) ||
                 (p->fout->format==PVS_AMP_PHASE))){
      return csound->InitError(csound,
                               Str("signal format must be amp-phase "
                                   "or amp-freq.\n"));
    }
   if (p->ceps.auxp == NULL ||
      p->ceps.size < sizeof(MYFLT) * (N+2))
    csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->ceps);
   else
     memset(p->ceps.auxp, 0, sizeof(MYFLT)*(N+2));
   if (p->fenv.auxp == NULL ||
       p->fenv.size < sizeof(MYFLT) * (N+2))
     csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fenv);
   if (p->fexc.auxp == NULL ||
       p->fexc.size < sizeof(MYFLT) * (N+2))
     csound->AuxAlloc(csound, sizeof(MYFLT) * (N + 2), &p->fexc);

   return OK;
}

void csoundComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);
void csoundInverseComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);

static int pvsvoc_process(CSOUND *csound, pvsvoc *p)
{
    int32 i,N = p->fout->N;
    float gain = (float) *p->gain;
    MYFLT kdepth = (MYFLT) *(p->kdepth);
    float *fin = (float *) p->fin->frame.auxp;
    float *ffr = (float *) p->ffr->frame.auxp;
    float *fexc = (float *) p->fexc.auxp;
    float *fout = (float *) p->fout->frame.auxp;
    int coefs = (int) *(p->kcoefs), j;
    MYFLT   *fenv = (MYFLT *) p->fenv.auxp;
    MYFLT   *ceps = (MYFLT *) p->ceps.auxp;
    float maxe=0.f, maxa=0.f;

    if (UNLIKELY(fout==NULL)) goto err1;

    if (p->lastframe < p->fin->framecount) {
      int tmp = N/2;
      tmp = tmp + tmp%2;
      for (j=0; j < 2; j++) {
        MYFLT a;
        maxe = 0.f;
        maxa = 0.f;
        for (i=0; i < N; i+=2) {
          a  = (j ? fin[i] : (fexc[i] = ffr[i]));
          maxa = maxa < a ? a : maxa;
          if (a <= 0) a = 1e-20;
          fenv[i/2] = log(a);
        }
        if (coefs < 1) coefs = 80;
        for (i=0; i < N; i+=2){
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
          fenv[i/2] = exp(ceps[i]);
          maxe = maxe < fenv[i/2] ? fenv[i/2] : maxe;
        }
        if (maxe)
          for (i=0; i<N; i+=2){
            if (j) fenv[i/2] *= maxa/maxe;
            if (fenv[i/2] && !j) {
              fenv[i/2] /= maxe;
              fexc[i] /= fenv[i/2];
            }
          }
      }

      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : FL(1.0)): FL(0.0);
      for (i=0;i < N+2;i+=2) {
        fout[i] = fenv[i/2]*(fexc[i]*kdepth + fin[i]*(FL(1.0)-kdepth))*gain;
        fout[i+1] = ffr[i+1]*(kdepth) + fin[i+1]*(FL(1.0)-kdepth);
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsvoc: not initialised\n"));
}

static int pvsmorph_init(CSOUND *csound, pvsmorph *p)
{
    int32 N = p->fin->N;

    if (p->fout->frame.auxp==NULL || p->fout->frame.size<(N+2)*sizeof(float))
      csound->AuxAlloc(csound,(N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (UNLIKELY(!(p->fout->format==PVS_AMP_FREQ) ||
                 (p->fout->format==PVS_AMP_PHASE))){
      return csound->InitError(csound,
                               Str("signal format must be amp-phase "
                                   "or amp-freq.\n"));
    }

    return OK;
}

static int pvsmorph_process(CSOUND *csound, pvsmorph *p)
{
    int32 i,N = p->fout->N;
    float frint = (float) *p->gain;
    float amint = (float) *(p->kdepth);
    float *fi1 = (float *) p->fin->frame.auxp;
    float *fi2 = (float *) p->ffr->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if (UNLIKELY(fout==NULL)) goto err1;

    if (p->lastframe < p->fin->framecount) {

      amint = amint > 0 ? (amint <= 1 ? amint : FL(1.0)): FL(0.0);
      frint = frint > 0 ? (frint <= 1 ? frint : FL(1.0)): FL(0.0);
      for(i=0;i < N+2;i+=2) {
        fout[i] = fi1[i]*(1.0-amint) + fi2[i]*(amint);
        fout[i+1] = fi1[i+1]*(1.0-frint) + fi2[i+1]*(frint);
      }
      p->fout->framecount = p->lastframe = p->fin->framecount;
    }

    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("pvsmorph: not initialised\n"));
}

static OENTRY localops[] = {
  {"sndloop", sizeof(sndloop),0, 5,
   "ak", "akkii", (SUBR)sndloop_init, NULL, (SUBR)sndloop_process},
  {"flooper", sizeof(flooper), TR, 5,
   "mm", "kkiiii", (SUBR)flooper_init, NULL, (SUBR)flooper_process},
  {"pvsarp", sizeof(pvsarp), 0,3,
   "f", "fkkk", (SUBR)pvsarp_init, (SUBR)pvsarp_process},
  {"pvsvoc", sizeof(pvsvoc), 0,3,
   "f", "ffkkO", (SUBR)pvsvoc_init, (SUBR)pvsvoc_process},
  {"flooper2", sizeof(flooper2), TR, 5,
   "mm", "kkkkkiooooO", (SUBR)flooper2_init, NULL, (SUBR)flooper2_process},
  /* {"flooper3", sizeof(flooper3), TR, 5,
     "a", "kkkkkioooo", (SUBR)flooper3_init, NULL, (SUBR)flooper3_process},*/
 {"pvsmorph", sizeof(pvsvoc), 0,3,
   "f", "ffkk", (SUBR)pvsmorph_init, (SUBR)pvsmorph_process}
};

int sndloop_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int) (sizeof(localops) / sizeof(OENTRY)));
}
