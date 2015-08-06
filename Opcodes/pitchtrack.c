/*
  pitchtrack.c

  kcps, kamp  ptrack asig, ihopsize [, ipeaks]

  (c) Victor Lazzarini, 2007

  based on M Puckette's pitch tracking algorithm.

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
#include <math.h>

#define MINFREQINBINS 5
#define MAXHIST 3
#define MAXWINSIZ 8192
#define MINWINSIZ 128
#define DEFAULTWINSIZ 1024
#define NPREV 20
#define MAXPEAKNOS 100
#define DEFAULTPEAKNOS 20
#define MINBW FL(0.03)
#define BINPEROCT 48
#define BPEROOVERLOG2 69.24936196
#define FACTORTOBINS FL(4/0.0145453)
#define BINGUARD 10
#define PARTIALDEVIANCE FL(0.023)
#define DBSCAL 3.333
#define DBOFFSET FL(-92.3)
#define MINBIN 3
#define MINAMPS 40
#define MAXAMPS 50


#define THRSH FL(10.)


static const MYFLT partialonset[] =
{
    FL(0.0),
    FL(48.0),
    FL(76.0782000346154967102),
    FL(96.0),
    FL(111.45254855459339269887),
    FL(124.07820003461549671089),
    FL(134.75303625876499715823),
    FL(144.0),
    FL(152.15640006923099342109),
    FL(159.45254855459339269887),
    FL(166.05271769459026829915),
    FL(172.07820003461549671088),
    FL(177.62110647077242370064),
    FL(182.75303625876499715892),
    FL(187.53074858920888940907),
    FL(192.0),
};

#define NPARTIALONSET ((int)(sizeof(partialonset)/sizeof(MYFLT)))


#define COEF1 ((MYFLT)(.5 * 1.227054))
#define COEF2 ((MYFLT)(.5 * -0.302385))
#define COEF3 ((MYFLT)(.5 * 0.095326))
#define COEF4 ((MYFLT)(.5 * -0.022748))
#define COEF5 ((MYFLT)(.5 * 0.002533))
#define FLTLEN 5


typedef struct peak
{
  MYFLT pfreq;
  MYFLT pwidth;
  MYFLT ppow;
  MYFLT ploudness;
} PEAK;

typedef struct histopeak
{
  MYFLT hpitch;
  MYFLT hvalue;
  MYFLT hloud;
  int hindex;
  int hused;
} HISTOPEAK;


typedef struct pitchtrack
{
  OPDS  h;
  MYFLT *freq, *amp;
  MYFLT *asig,*size,*peak;
  AUXCH signal, prev, sin, spec1, spec2, peakarray;
  int numpks;
  int cnt;
  int histcnt;
  int hopsize;
  MYFLT sr;
  MYFLT cps;
  MYFLT dbs[NPREV];
  MYFLT amplo;
  MYFLT amphi;
  MYFLT npartial;
  MYFLT dbfs;
  MYFLT prevf;
} PITCHTRACK;

void ptrack(CSOUND *csound,PITCHTRACK *p)
{
  MYFLT *spec = (MYFLT *)p->spec1.auxp;
  MYFLT *spectmp = (MYFLT *)p->spec2.auxp;
  MYFLT *sig = (MYFLT *)p->signal.auxp;
  MYFLT *sinus  = (MYFLT *)p->sin.auxp;
  MYFLT *prev  = (MYFLT *)p->prev.auxp;
  PEAK  *peaklist = (PEAK *)p->peakarray.auxp;
  HISTOPEAK histpeak;
  int i, j, k, hop = p->hopsize, n = 2*hop, npeak, logn = -1, count, tmp;
  MYFLT totalpower, totalloudness, totaldb;
  MYFLT maxbin,  *histogram = spectmp + BINGUARD;
  MYFLT hzperbin = p->sr / (n + n);
  int numpks = p->numpks;
  int indx, halfhop = hop>>1;
  MYFLT best;
  MYFLT cumpow = 0, cumstrength = 0, freqnum = 0, freqden = 0;
  int npartials = 0,  nbelow8 = 0;
  MYFLT putfreq;

  count = p->histcnt + 1;
  if (count == NPREV) count = 0;
  p->histcnt = count;

  tmp = n;
  while (tmp) {
    tmp >>= 1;
    logn++;
  }
  maxbin = BINPEROCT * (logn-2);

  for (i = 0, k = 0; i < hop; i++, k += 2) {
    spec[k]   = sig[i] * sinus[k];
    spec[k+1] = sig[i] * sinus[k+1];
  }

  csound->ComplexFFT(csound, spec, hop);

  for (i = 0, k = 2*FLTLEN; i < hop; i+=2, k += 4) {
    spectmp[k]   = spec[i];
    spectmp[k+1] = spec[i+1];
  }
  for (i = n - 2, k = 2*FLTLEN+2; i >= 0; i-=2, k += 4) {
    spectmp[k]   = spec[i];
    spectmp[k+1] = -spec[i+1];
  }
  for (i = (2*FLTLEN), k = (2*FLTLEN-2);i<FLTLEN*4; i+=2, k-=2) {
    spectmp[k]   = spectmp[i];
    spectmp[k+1] = -spectmp[i+1];
  }
  for (i = (2*FLTLEN+n-2), k =(2*FLTLEN+n); i>=0; i-=2, k+=2) {
    spectmp[k]   = spectmp[i];
    spectmp[k+1] = -spectmp[k+1];
  }

  for (i = j = 0, k = 2*FLTLEN; i < halfhop; i++, j+=8, k+=2) {
    MYFLT re,  im;

    re= COEF1 * ( prev[k-2] - prev[k+1]  + spectmp[k-2] - prev[k+1]) +
      COEF2 * ( prev[k-3] - prev[k+2]  + spectmp[k-3]  - spectmp[ 2]) +
      COEF3 * (-prev[k-6] +prev[k+5]  -spectmp[k-6] +spectmp[k+5]) +
      COEF4 * (-prev[k-7] +prev[k+6]  -spectmp[k-7] +spectmp[k+6]) +
      COEF5 * ( prev[k-10] -prev[k+9]  +spectmp[k-10] -spectmp[k+9]);

    im= COEF1 * ( prev[k-1] +prev[k]  +spectmp[k-1] +spectmp[k]) +
      COEF2 * (-prev[k-4] -prev[k+3]  -spectmp[k-4] -spectmp[k+3]) +
      COEF3 * (-prev[k-5] -prev[k+4]  -spectmp[k-5] -spectmp[k+4]) +
      COEF4 * ( prev[k-8] +prev[k+7]  +spectmp[k-8] +spectmp[k+7]) +
      COEF5 * ( prev[k-9] +prev[k+8]  +spectmp[k-9] +spectmp[k+8]);

    spec[j]   = FL(0.707106781186547524400844362104849) * (re + im);
    spec[j+1] = FL(0.707106781186547524400844362104849) * (im - re);
    spec[j+4] = prev[k] + spectmp[k+1];
    spec[j+5] = prev[k+1] - spectmp[k];

    j += 8;
    k += 2;
    re= COEF1 * ( prev[k-2] -prev[k+1]  -spectmp[k-2] +spectmp[k+1]) +
      COEF2 * ( prev[k-3] -prev[k+2]  -spectmp[k-3] +spectmp[k+2]) +
      COEF3 * (-prev[k-6] +prev[k+5]  +spectmp[k-6] -spectmp[k+5]) +
      COEF4 * (-prev[k-7] +prev[k+6]  +spectmp[k-7] -spectmp[k+6]) +
      COEF5 * ( prev[k-10] -prev[k+9]  -spectmp[k-10] +spectmp[k+9]);

    im= COEF1 * ( prev[k-1] +prev[k]  -spectmp[k-1] -spectmp[k]) +
      COEF2 * (-prev[k-4] -prev[k+3]  +spectmp[k-4] +spectmp[k+3]) +
      COEF3 * (-prev[k-5] -prev[k+4]  +spectmp[k-5] +spectmp[k+4]) +
      COEF4 * ( prev[k-8] +prev[k+7]  -spectmp[k-8] -spectmp[k+7]) +
      COEF5 * ( prev[k-9] +prev[k+8]  -spectmp[k-9] -spectmp[k+8]);

    spec[j]   = FL(0.707106781186547524400844362104849) * (re + im);
    spec[j+1] = FL(0.707106781186547524400844362104849) * (im - re);
    spec[j+4] = prev[k] - spectmp[k+1];
    spec[j+5] = prev[k+1] + spectmp[k];

  }


  for (i = 0; i < n + 4*FLTLEN; i++) prev[i] = spectmp[i];

  for (i = 0; i < MINBIN; i++) spec[4*i + 2] = spec[4*i + 3] = FL(0.0);

  for (i = 4*MINBIN, totalpower = 0; i < (n-2)*4; i += 4) {
    MYFLT re = spec[i] - FL(0.5) * (spec[i-8] + spec[i+8]);
    MYFLT im = spec[i+1] - FL(0.5) * (spec[i-7] + spec[i+9]);
    spec[i+3] = (totalpower += (spec[i+2] = re * re + im * im));
  }

  if (totalpower > FL(1.0e-9)) {
    totaldb = FL(DBSCAL) * LOG(totalpower/n);
    totalloudness = SQRT(SQRT(totalpower));
    if (totaldb < 0) totaldb = 0;
  }
  else totaldb = totalloudness = FL(0.0);

  p->dbs[count] = totaldb + DBOFFSET;

  if (totaldb >= p->amplo) {

    npeak = 0;

    for (i = 4*MINBIN;i < (4*(n-2)) && npeak < numpks; i+=4) {
      MYFLT height = spec[i+2], h1 = spec[i-2], h2 = spec[i+6];
      MYFLT totalfreq, peakfr, tmpfr1, tmpfr2, m, var, stdev;

      if (height < h1 || height < h2 ||
          h1 < FL(0.00001)*totalpower ||
          h2 < FL(0.00001)*totalpower) continue;

      peakfr= ((spec[i-8] - spec[i+8]) * (FL(2.0) * spec[i] -
                                          spec[i+8] - spec[i-8]) +
               (spec[i-7] - spec[i+9]) * (FL(2.0) * spec[i+1] -
                                          spec[i+9] - spec[i-7])) /
        (height + height);
      tmpfr1=  ((spec[i-12] - spec[i+4]) *
                (FL(2.0) * spec[i-4] - spec[i+4] - spec[i-12]) +
                (spec[i-11] - spec[i+5]) * (FL(2.0) * spec[i-3] -
                                            spec[i+5] - spec[i-11])) /
        (FL(2.0) * h1) - 1;
      tmpfr2= ((spec[i-4] - spec[i+12]) * (FL(2.0) * spec[i+4] -
                                           spec[i+12] - spec[i-4]) +
               (spec[i-3] - spec[i+13]) * (FL(2.0) * spec[i+5] -
                                           spec[i+13] - spec[i-3])) /
        (FL(2.0) * h2) + 1;


      m = FL(0.333333333333) * (peakfr + tmpfr1 + tmpfr2);
      var = FL(0.5) * ((peakfr-m)*(peakfr-m) +
                       (tmpfr1-m)*(tmpfr1-m) + (tmpfr2-m)*(tmpfr2-m));

      totalfreq = (i>>2) + m;
      if (var * totalpower > THRSH * height
          || var < FL(1.0e-30)) continue;

      stdev = (MYFLT)sqrt((double)var);
      if (totalfreq < 4) totalfreq = 4;


      peaklist[npeak].pwidth = stdev;
      peaklist[npeak].ppow = height;
      peaklist[npeak].ploudness = SQRT(SQRT(height));
      peaklist[npeak].pfreq = totalfreq;
      npeak++;

    }

    if (npeak > numpks) npeak = numpks;
    for (i = 0; i < maxbin; i++) histogram[i] = 0;
    for (i = 0; i < npeak; i++) {
      MYFLT pit = (MYFLT)(BPEROOVERLOG2 * LOG(peaklist[i].pfreq) - 96.0);
      MYFLT binbandwidth = FACTORTOBINS * peaklist[i].pwidth/peaklist[i].pfreq;
      MYFLT putbandwidth = (binbandwidth < FL(2.0) ? FL(2.0) : binbandwidth);
      MYFLT weightbandwidth = (binbandwidth < FL(1.0) ? FL(1.0) : binbandwidth);
      MYFLT weightamp = FL(4.0) * peaklist[i].ploudness / totalloudness;
      for (j = 0; j < NPARTIALONSET; j++) {
        MYFLT bin = pit - partialonset[j];
        if (bin < maxbin) {
          MYFLT para, pphase, score = FL(30.0) * weightamp /
            ((j+p->npartial) * weightbandwidth);
          int firstbin = bin + FL(0.5) - FL(0.5) * putbandwidth;
          int lastbin = bin + FL(0.5) + FL(0.5) * putbandwidth;
          int ibw = lastbin - firstbin;
          if (firstbin < -BINGUARD) break;
          para = FL(1.0) / (putbandwidth * putbandwidth);
          for (k = 0, pphase = firstbin-bin; k <= ibw;
               k++,pphase += FL(1.0))
            histogram[k+firstbin] += score * (FL(1.0) - para * pphase * pphase);

        }
      }
    }


    for (best = 0, indx = -1, j=0; j < maxbin; j++)
      if (histogram[j] > best)
        indx = j,  best = histogram[j];

    histpeak.hvalue = best;
    histpeak.hindex = indx;

    putfreq = EXP((FL(1.0) / BPEROOVERLOG2) *
                  (histpeak.hindex + FL(96.0)));
    for (j = 0; j < npeak; j++) {
      MYFLT fpnum = peaklist[j].pfreq/putfreq;
      int pnum = (int)(fpnum + FL(0.5));
      MYFLT fipnum = pnum;
      MYFLT deviation;
      if (pnum > 16 || pnum < 1) continue;
      deviation = FL(1.0) - fpnum/fipnum;
      if (deviation > -PARTIALDEVIANCE && deviation < PARTIALDEVIANCE) {
        MYFLT stdev, weight;
        npartials++;
        if (pnum < 8) nbelow8++;
        cumpow += peaklist[j].ppow;
        cumstrength += SQRT(SQRT(peaklist[j].ppow));
        stdev = (peaklist[j].pwidth > MINBW ?
                 peaklist[j].pwidth : MINBW);
        weight = FL(1.0) / ((stdev*fipnum) * (stdev*fipnum));
        freqden += weight;
        freqnum += weight * peaklist[j].pfreq/fipnum;
      }
    }
    if ((nbelow8 < 4 || npartials < 7) && cumpow < FL(0.01) * totalpower)
      histpeak.hvalue = 0;
    else {
      double pitchpow = (cumstrength * cumstrength);
      MYFLT freqinbins = freqnum/freqden;
      pitchpow = pitchpow * pitchpow;
      if (freqinbins < MINFREQINBINS)
        histpeak.hvalue = 0;
      else {
        p->cps = histpeak.hpitch = hzperbin * freqnum/freqden;
        histpeak.hloud = FL(DBSCAL) * LOG(pitchpow/n);
      }
    }

  }
}

int pitchtrackinit(CSOUND *csound, PITCHTRACK  *p)
{

  int i, winsize = *p->size*2, powtwo, tmp;
  MYFLT *tmpb;

  if (UNLIKELY(winsize < MINWINSIZ || winsize > MAXWINSIZ)) {
    csound->Warning(csound, Str("ptrack: FFT size out of range; using %d\n"),
                    winsize = DEFAULTWINSIZ);
  }

  tmp = winsize;
  powtwo = -1;

  while (tmp) {
    tmp >>= 1;
    powtwo++;
  }

  if (UNLIKELY(winsize != (1 << powtwo))) {
    csound->Warning(csound, Str("ptrack: FFT size not a power of 2; using %d\n"),
                    winsize = (1 << powtwo));
  }
  p->hopsize = *p->size;
  if (!p->signal.auxp || p->signal.size < p->hopsize*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, p->hopsize*sizeof(MYFLT), &p->signal);
  }
  if (!p->prev.auxp || p->prev.size < (p->hopsize*2 + 4*FLTLEN)*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, (p->hopsize*2 + 4*FLTLEN)*sizeof(MYFLT), &p->prev);
  }
  if (!p->sin.auxp || p->sin.size < (p->hopsize*2)*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, (p->hopsize*2)*sizeof(MYFLT), &p->sin);
  }

  if (!p->spec2.auxp || p->spec2.size < (winsize*4 + 4*FLTLEN)*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, (winsize*4 + 4*FLTLEN)*sizeof(MYFLT), &p->spec2);
  }

  if (!p->spec1.auxp || p->spec1.size < (winsize*4)*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, (winsize*4)*sizeof(MYFLT), &p->spec1);
  }

  for (i = 0, tmpb = (MYFLT *)p->signal.auxp; i < p->hopsize; i++)
    tmpb[i] = FL(0.0);
  for (i = 0, tmpb = (MYFLT *)p->prev.auxp; i < winsize + 4 * FLTLEN; i++)
    tmpb[i] = FL(0.0);
  for (i = 0, tmpb = (MYFLT *)p->sin.auxp; i < p->hopsize; i++)
    tmpb[2*i] =   (MYFLT) cos((PI*i)/(winsize)),
      tmpb[2*i+1] = -(MYFLT)sin((PI*i)/(winsize));

  p->cnt = 0;
  if (*p->peak == 0 || *p->peak > MAXPEAKNOS)
    p->numpks = DEFAULTPEAKNOS;
  else
    p->numpks = *p->peak;

  if (!p->peakarray.auxp || p->peakarray.size < (p->numpks+1)*sizeof(PEAK)) {
    csound->AuxAlloc(csound, (p->numpks+1)*sizeof(PEAK), &p->peakarray);
  }

  p->cnt = 0;
  p->histcnt = 0;
  p->sr = CS_ESR;
  for (i = 0; i < NPREV; i++) p->dbs[i] = FL(-144.0);
  p->amplo = MINAMPS;
  p->amphi = MAXAMPS;
  p->npartial = 7;
  p->dbfs = FL(32768.0)/csound->e0dbfs;
  p->prevf = p->cps = 100.0;
  return (OK);
}

int pitchtrackprocess(CSOUND *csound, PITCHTRACK *p)
{
  MYFLT *sig = p->asig; int i;
  MYFLT *buf = (MYFLT *)p->signal.auxp;
  int pos = p->cnt, h = p->hopsize;
  MYFLT scale = p->dbfs;
  int ksmps = CS_KSMPS;

  for (i=0; i<ksmps; i++,pos++) {
    if (pos == h) {
      ptrack(csound,p);
      pos = 0;
    }
    buf[pos] = sig[i]*scale;
  }
  //if (p->cps)
  *p->freq = p->cps;
  //else *p->freq = p->prevf;
  //p->prevf = *p->freq;
  *p->amp =  p->dbs[p->histcnt];
  p->cnt = pos;

  return OK;
}

typedef struct _pitchaf{
  OPDS h;
  MYFLT *kpitch;
  MYFLT *asig, *kfmin, *kfmax, *iflow;
  AUXCH buff1, buff2, cor;
  int lag;
  MYFLT pitch;
  int len,size;
} PITCHAF;

int pitchafset(CSOUND *csound, PITCHAF *p){
  int siz = (int)(CS_ESR/ (*p->iflow));
  if (p->buff1.auxp == NULL || p->buff1.size < siz*sizeof(MYFLT))
    csound->AuxAlloc(csound, siz*sizeof(MYFLT), &p->buff1);
  else
    memset(p->buff1.auxp, 0, p->buff1.size);
  if (p->buff2.auxp == NULL ||p-> buff2.size < siz*sizeof(MYFLT))
    csound->AuxAlloc(csound, siz*sizeof(MYFLT), &p->buff2);
  else
    memset(p->buff2.auxp, 0, p->buff2.size);
  if (p->cor.auxp == NULL || p->cor.size < siz*sizeof(MYFLT))
    csound->AuxAlloc(csound, siz*sizeof(MYFLT), &p->cor);
  else
    memset(p->cor.auxp, 0, p->cor.size);
  p->lag = 0;
  p->pitch = FL(0.0);
  p->len = siz;
  p->size = siz;
  return OK;
}

int pitchafproc(CSOUND *csound, PITCHAF *p)
{

  int lag = p->lag,n, i, j, imax = 0, len = p->len,
    ksmps = CS_KSMPS;
  MYFLT *buff1 = (MYFLT *)p->buff1.auxp;
  MYFLT *buff2 = (MYFLT *)p->buff2.auxp;
  MYFLT *cor = (MYFLT *)p->cor.auxp;
  MYFLT *s = p->asig, pitch;
  //MYFLT ifmax = *p->kfmax;

  for (n=0; n < ksmps; n++) {
    for (i=0,j=lag; i < len; i++) {
      cor[lag] += buff1[i]*buff2[j];
      j = j != len ? j+1 : 0;
    }
    buff2[lag++] = s[n];

    if (lag == len) {
      float max = 0.0f;
      for (i=0; i < len; i++) {
        if (cor[i] > max) {
          max = cor[i];
          if (i) imax = i;
        }
        buff1[i] = buff2[i];
        cor[i] = FL(0.0);
      }
      len = CS_ESR/(*p->kfmin);
      if (len > p->size) len = p->size;
      lag  =  0;
    }
  }
  p->lag = lag;
  p->len = len;
  if (imax) {
    pitch = CS_ESR/imax;
    if (pitch <= *p->kfmax) p->pitch = pitch;
  }
  *p->kpitch = p->pitch;

  return OK;
}
/* PLL Pitch tracker (Zoelzer et al)
   V Lazzarini, 2012
*/

#define ROOT2 (1.4142135623730950488)
enum {LP1=0, LP2, HP};

typedef struct biquad_ {
  double a0, a1, a2, b1, b2;
  double del1, del2;
} BIQUAD;

typedef struct plltrack_
{
  OPDS  h;
  MYFLT *freq, *lock;
  MYFLT *asig,*kd,*klpf,*klpfQ,*klf,*khf,*kthresh;
  BIQUAD   fils[6];
  double  ace, xce;
  double cos_x, sin_x, x1, x2;
  MYFLT klpf_o, klpfQ_o, klf_o,khf_o;

} PLLTRACK;

void update_coefs(CSOUND *csound, double fr, double Q, BIQUAD *biquad, int TYPE)
{
    double k, ksq, div, ksqQ;

    switch(TYPE){
    case LP2:
      k = tan(fr*csound->pidsr);
      ksq = k*k;
      ksqQ = ksq*Q;
      div = ksqQ+k+Q;
      biquad->b1 = (2*Q*(ksq-1.))/div;
      biquad->b2 = (ksqQ-k+Q)/div;
      biquad->a0 = ksqQ/div;
      biquad->a1 = 2*biquad->a0;
      biquad->a2 = biquad->a0;
      break;

    case LP1:
      k = 1.0/tan(csound->pidsr*fr);
      ksq = k*k;
      biquad->a0 = 1.0 / ( 1.0 + ROOT2 * k + ksq);
      biquad->a1 = 2.0*biquad->a0;
      biquad->a2 = biquad->a0;
      biquad->b1 = 2.0 * (1.0 - ksq) * biquad->a0;
      biquad->b2 = ( 1.0 - ROOT2 * k + ksq) * biquad->a0;
      break;

    case HP:
      k = tan(csound->pidsr*fr);
      ksq = k*k;
      biquad->a0 = 1.0 / ( 1.0 + ROOT2 * k + ksq);
      biquad->a1 = -2.*biquad->a0;
      biquad->a2 = biquad->a0;
      biquad->b1 = 2.0 * (ksq - 1.0) * biquad->a0;
      biquad->b2 = ( 1.0 - ROOT2 * k + ksq) * biquad->a0;
      break;
    }

}


int plltrack_set(CSOUND *csound, PLLTRACK *p)
{
    int i;
    p->x1 = p->cos_x = p->sin_x = 0.0;
    p->x2 = 1.0;
    p->klpf_o = p->klpfQ_o = p->klf_o = p->khf_o = 0.0;
    update_coefs(csound,10.0, 0.0, &p->fils[4], LP1);
    p->ace = p->xce = 0.0;
    for (i=0; i < 6; i++)
      p->fils[i].del1 = p->fils[i].del2 = 0.0;

    return OK;
}

int plltrack_perf(CSOUND *csound, PLLTRACK *p)
{
    int ksmps, i, k;
    MYFLT _0dbfs;
    double a0[6], a1[6], a2[6], b1[6], b2[6];
    double *mem1[6], *mem2[6];
    double *ace, *xce;
    double *cos_x, *sin_x, *x1, *x2;
    double scal,esr;
    BIQUAD *biquad = p->fils;
    MYFLT *asig=p->asig,kd=*p->kd,klpf,klpfQ,klf,khf,kthresh;
    MYFLT *freq=p->freq, *lock =p->lock, itmp = asig[0];
    int itest = 0;

    _0dbfs = csound->e0dbfs;
    ksmps = CS_KSMPS;
    esr = CS_ESR;
    scal = 2.0*csound->pidsr;

    /* check for muted input & bypass */
    for(i=0; i < ksmps; i++){
      if(asig[i] != 0.0 && asig[i] != itmp) {
        itest = 1;
        break;
      }
      itmp = asig[i];
    }
    if(!itest) return OK;


    if (*p->klpf == 0) klpf = 20.0;
    else klpf = *p->klpf;

    if (*p->klpfQ == 0) klpfQ =  1./3.;
    else klpfQ = *p->klpfQ;

    if (*p->klf == 0) klf = 20.0;
    else klf = *p->klf;

    if (*p->khf == 0) khf = 1500.0;
    else khf = *p->khf;

    if (*p->kthresh == 0.0) kthresh= 0.001;
    else kthresh = *p->kthresh;



    if (p->khf_o != khf) {
      update_coefs(csound, khf, 0.0, &biquad[0], LP1);
      update_coefs(csound, khf, 0.0, &biquad[1], LP1);
      update_coefs(csound, khf, 0.0, &biquad[2], LP1);
      p->khf_o = khf;
    }

    if (p->klf_o != klf) {
      update_coefs(csound, klf, 0.0, &biquad[3], HP);
      p->klf_o = klf;
    }

    if (p->klpf_o != klpf || p->klpfQ_o != klpfQ ) {
      update_coefs(csound, klpf, klpfQ, &biquad[5], LP2);
      p->klpf_o = klpf; p->klpfQ_o = klpfQ;
    }



    for(k=0; k < 6; k++) {
      a0[k] = biquad[k].a0;
      a1[k] = biquad[k].a1;
      a2[k] = biquad[k].a2;
      b1[k] = biquad[k].b1;
      b2[k] = biquad[k].b2;
      mem1[k] = &(biquad[k].del1);
      mem2[k] = &(biquad[k].del2);
    }

    cos_x = &p->cos_x;
    sin_x = &p->sin_x;
    x1 = &p->x1;
    x2 = &p->x2;
    xce = &p->xce;
    ace = &p->ace;

    for (i=0; i < ksmps; i++){
      double input = (double) (asig[i]/_0dbfs), env;
      double w, y, icef = 0.99, fosc, xd, c, s, oc;

      /* input stage filters */
      for (k=0; k < 4 ; k++){
        w =  input - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
        y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
        *(mem2[k]) = *(mem1[k]);
        *(mem1[k]) = w;
        input = y;
      }

      /* envelope extraction */
      w =  FABS(input) - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
      y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
      *(mem2[k]) = *(mem1[k]);
      *(mem1[k]) = w;
      env = y;
      k++;

      /* constant envelope */
      if (env > kthresh)
        input /= env;
      else input = 0.0;

      /*post-ce filter */
      *ace = (1.-icef)*(input + *xce)/2. + *ace*icef;
      *xce = input;

      /* PLL */
      xd =  *cos_x * (*ace) * kd * esr;
      w =  xd - *(mem1[k])*b1[k] - *(mem2[k])*b2[k];
      y  = w*a0[k] + *(mem1[k])*a1[k] + *(mem2[k])*a2[k];
      *(mem2[k]) = *(mem1[k]);
      *(mem1[k]) = w;
      freq[i] = FABS(2*y);
      lock[i] = *ace * (*sin_x);
      fosc = y + xd;

      /* quadrature osc */
      *sin_x = *x1;
      *cos_x = *x2;
      oc = fosc*scal;
      c = COS(oc);  s = SIN(oc);
      *x1 = *sin_x*c + *cos_x*s;
      *x2 = -*sin_x*s + *cos_x*c;

    }
    return OK;
}


#define S(x)    sizeof(x)

static OENTRY pitchtrack_localops[] = {
  {"ptrack", S(PITCHTRACK), 0, 5, "kk", "aio",
   (SUBR)pitchtrackinit, NULL, (SUBR)pitchtrackprocess},
  {"pitchac", S(PITCHTRACK), 0, 5, "k", "akki",
   (SUBR)pitchafset, NULL, (SUBR)pitchafproc},
  {"plltrack", S(PLLTRACK), 0, 5, "aa", "akOOOOO",
   (SUBR)plltrack_set, NULL, (SUBR)plltrack_perf}

};

LINKAGE_BUILTIN(pitchtrack_localops)
