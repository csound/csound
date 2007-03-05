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

#include "csdl.h"
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
#define BPEROOVERLOG2 69.24936196f    
#define FACTORTOBINS FL(4/0.0145453)      
#define BINGUARD 10                     
#define PARTIALDEVIANCE FL(0.023)          
#define DBSCAL 3.333       
#define DBOFFSET FL(-92.3)
#define MINBIN 3
#define MINAMPS 40
#define MAXAMPS 50


#define THRSH FL(10.)   


static MYFLT partialonset[] =
{
0,
48,
76.0782000346154967102,
96,
111.45254855459339269887,
124.07820003461549671089,
134.75303625876499715823,
144,
152.15640006923099342109,
159.45254855459339269887,
166.05271769459026829915,
172.07820003461549671088,
177.62110647077242370064,
182.75303625876499715892,
187.53074858920888940907,
192,
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
  int i, j, k, hop = p->hopsize, n = 2*hop, npeak, 
    logn = -1, count, tmp;
  MYFLT totalpower, totalloudness, totaldb;
  MYFLT maxbin,  *histogram = spectmp + BINGUARD;
  MYFLT hzperbin = p->sr / (2.0f * n);
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
  while(tmp){
    tmp >>= 1;
    logn++;
  }
  maxbin = BINPEROCT * (logn-2);

  for (i = 0, k = 0; i < hop; i++, k += 2){
    spec[k] = sig[i] * sinus[k];
    spec[k+1] = sig[i] * sinus[k+1];
  }

  csound->ComplexFFT(csound, spec, hop);

  for (i = 0, k = 2*FLTLEN; i < hop; i+=2, k += 4){
    spectmp[k] = spec[i];
    spectmp[k+1] = spec[i+1];
  }
  for (i = n - 2, k = 2*FLTLEN+2; i >= 0; i-=2, k += 4){
    spectmp[k] = spec[i]; 
    spectmp[k+1] = -spec[i+1];
  }
  for (i = (2*FLTLEN), k = (2*FLTLEN-2);i<FLTLEN*2; i+=2, k-=2){
    spectmp[k] = spectmp[i];  
    spectmp[k+1] = -spectmp[i+1];
  }
  for (i = (2*FLTLEN+n-2), k =(2*FLTLEN+n); i>=0; i-=2, k+=2){
    spectmp[k] = spectmp[i];
    spectmp[k+1] = -spectmp[k+1];
  }

  for (i = j = 0, k = 2*FLTLEN; i < halfhop; i++, j+=8, k+=2)
    {
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

      spec[j] = 0.7071f * (re + im);
      spec[j+1] = 0.7071f * (im - re);
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

      spec[j] = 0.7071f * (re + im);
      spec[j+1] = 0.7071f * (im - re);
      spec[j+4] = prev[k] - spectmp[k+1];
      spec[j+5] = prev[k+1] + spectmp[k];
        
    }
    

 for (i = 0; i < n + 4*FLTLEN; i++) prev[i] = spectmp[i];

  for (i = 0; i < MINBIN; i++) spec[4*i + 2] = spec[4*i + 3] = 0;
  
  for (i = 4*MINBIN, totalpower = 0; i < (n-2)*4; i += 4)
    {
      MYFLT re = spec[i] - 0.5f * (spec[i-8] + spec[i+8]);
      MYFLT im = spec[i+1] - 0.5f * (spec[i-7] + spec[i+9]);
      spec[i+3] = (totalpower += (spec[i+2] = re * re + im * im));
    }
 
  if (totalpower > 1e-9f)
    {
      totaldb = DBSCAL  * log(totalpower/n);
      totalloudness = sqrt(sqrt(totalpower));
      if (totaldb < 0) totaldb = 0;
    }
  else totaldb = totalloudness = 0;
  
  p->dbs[count] = totaldb + DBOFFSET;

  if (totaldb >= p->amplo) {

    npeak = 0;
  
    for (i = 4*MINBIN;i < (4*(n-2)) && npeak < numpks; i+=4)
      {
        MYFLT height = spec[i+2], h1 = spec[i-2], h2 = spec[i+6];
        MYFLT totalfreq, peakfr, tmpfr1, tmpfr2, m, var, stdev;
        
        if (height < h1 || height < h2 ||
            h1 < 0.00001f*totalpower || 
            h2 < 0.00001f*totalpower) continue;

        peakfr= ((spec[i-8] - spec[i+8]) * (2.0f * spec[i] - spec[i+8] - spec[i-8]) +
                (spec[i-7] - spec[i+9]) * (2.0f * spec[i+1] - spec[i+9] - spec[i-7])) /
	  (2.0f * height);
        tmpfr1=    ((spec[i-12] - spec[i+4]) * (2.0f * spec[i-4] - spec[i+4] - spec[i-12]) +
                (spec[i-11] - spec[i+5]) * (2.0f * spec[i-3] - spec[i+5] - spec[i-11])) /
	  (2.0f * h1) - 1;
        tmpfr2=    ((spec[i-4] - spec[i+12]) * (2.0f * spec[i+4] - spec[i+12] - spec[i-4]) +
                (spec[i-3] - spec[i+13]) * (2.0f * spec[i+5] - spec[i+13] - spec[i-3])) /
	  (2.0f * h2) + 1;


        m = 0.333333f * (peakfr + tmpfr1 + tmpfr2);
        var = 0.5f * ((peakfr-m)*(peakfr-m) + (tmpfr1-m)*(tmpfr1-m) + (tmpfr2-m)*(tmpfr2-m));

        totalfreq = (i>>2) + m;
        if (var * totalpower > THRSH * height 
                      || var < 1e-30) continue;
	  
        stdev = sqrt(var);
        if (totalfreq < 4) totalfreq = 4;
        	 
      
        peaklist[npeak].pwidth = stdev;
        peaklist[npeak].ppow = height;
        peaklist[npeak].ploudness = sqrt(sqrt(height));
        peaklist[npeak].pfreq = totalfreq;
        npeak++;
       
      }
    
    if (npeak > numpks) npeak = numpks; 
    for (i = 0; i < maxbin; i++) histogram[i] = 0;
    for (i = 0; i < npeak; i++)
      {
        MYFLT pit = BPEROOVERLOG2 * log(peaklist[i].pfreq) - 96.0f;
        MYFLT binbandwidth = FACTORTOBINS * peaklist[i].pwidth/peaklist[i].pfreq;
        MYFLT putbandwidth = (binbandwidth < 2 ? 2 : binbandwidth);
        MYFLT weightbandwidth = (binbandwidth < 1.0f ? 1.0f : binbandwidth);
        MYFLT weightamp = 4. * peaklist[i].ploudness / totalloudness;
        for (j = 0; j < NPARTIALONSET; j++)
	  {
            MYFLT bin = pit - partialonset[j];
            if (bin < maxbin)
	      {
                MYFLT para, pphase, score = 30.0f * weightamp /
		  ((j+p->npartial) * weightbandwidth);
                int firstbin = bin + 0.5f - 0.5f * putbandwidth;
                int lastbin = bin + 0.5f + 0.5f * putbandwidth;
                int ibw = lastbin - firstbin;
                if (firstbin < -BINGUARD) break;
                para = 1.0f / (putbandwidth * putbandwidth);
                for (k = 0, pphase = firstbin-bin; k <= ibw;
		     k++,pphase += 1.0f)
                    histogram[k+firstbin] += score * (1.0f - para * pphase * pphase);
                   
	      }
	  }
      }


            for (best = 0, indx = -1, j=0; j < maxbin; j++)
	      if (histogram[j] > best)
		indx = j,  best = histogram[j];
	   
        histpeak.hvalue = best;
        histpeak.hindex = indx;
       
        putfreq = exp((1.0f / BPEROOVERLOG2) *
			     (histpeak.hindex + 96.0f));
        for (j = 0; j < npeak; j++)
	  {
            MYFLT fpnum = peaklist[j].pfreq/putfreq;
            int pnum = fpnum + 0.5f;
            MYFLT fipnum = pnum;
            MYFLT deviation;
            if (pnum > 16 || pnum < 1) continue;
            deviation = 1.0f - fpnum/fipnum;
            if (deviation > -PARTIALDEVIANCE && deviation < PARTIALDEVIANCE)
	      {
                MYFLT stdev, weight;
                npartials++;
                if (pnum < 8) nbelow8++;
                cumpow += peaklist[j].ppow;
                cumstrength += sqrt(sqrt(peaklist[j].ppow));
                stdev = (peaklist[j].pwidth > MINBW ?
			 peaklist[j].pwidth : MINBW);
                weight = 1.0f / ((stdev*fipnum) * (stdev*fipnum));
                freqden += weight;
                freqnum += weight * peaklist[j].pfreq/fipnum;          

	      }

	  }
        if ((nbelow8 < 4 || npartials < 7) && cumpow < 0.01f * totalpower)
	  histpeak.hvalue = 0;
        else
	  {
            MYFLT pitchpow = (cumstrength * cumstrength) *
	      (cumstrength * cumstrength);
            MYFLT freqinbins = freqnum/freqden;
            if (freqinbins < MINFREQINBINS)
	      histpeak.hvalue = 0;
            else
	      {
                p->cps = histpeak.hpitch = hzperbin * freqnum/freqden;
                histpeak.hloud = (DBSCAL) * log(pitchpow/n);
	      }
	  }

  }

}

int pitchtrackinit(CSOUND *csound, PITCHTRACK  *p)
{
 
    int i, winsize = *p->size*2, powtwo, tmp;
    MYFLT *tmpb;

    if (winsize < MINWINSIZ || winsize > MAXWINSIZ)
    {
      csound->Message(csound, "ptrack: FFT size out of range; using %d",
            winsize = DEFAULTWINSIZ);
    }
    
    tmp = winsize;
    powtwo = -1;
   
    while (tmp){   
        tmp >>= 1;
        powtwo++;
    }

    if (winsize != (1 << powtwo))
    {
      csound->Message(csound, "ptrack: FFT size not a power of 2; using %d", 
            winsize = (1 << powtwo));
    }
    p->hopsize = *p->size;
    if(!p->signal.auxp && p->signal.size < p->hopsize*sizeof(MYFLT)){
      csound->AuxAlloc(csound, p->hopsize*sizeof(MYFLT), &p->signal);
    }
    if(!p->prev.auxp && p->prev.size < (p->hopsize*2 + 4*FLTLEN)*sizeof(MYFLT)){
      csound->AuxAlloc(csound, (p->hopsize*2 + 4*FLTLEN)*sizeof(MYFLT), &p->prev);
    }
    if(!p->sin.auxp && p->sin.size < (p->hopsize*2)*sizeof(MYFLT)){
      csound->AuxAlloc(csound, (p->hopsize*2)*sizeof(MYFLT), &p->sin);
    }   
    
    if(!p->spec2.auxp && p->spec2.size < (winsize*4 + 4*FLTLEN)*sizeof(MYFLT)){
      csound->AuxAlloc(csound, (winsize*4 + 4*FLTLEN)*sizeof(MYFLT), &p->spec2);
    }

    if(!p->spec1.auxp && p->spec1.size < (winsize*4)*sizeof(MYFLT)){
      csound->AuxAlloc(csound, (winsize*4)*sizeof(MYFLT), &p->spec1);
    }
 
    

    for (i = 0, tmpb = (MYFLT *)p->signal.auxp; i < p->hopsize; i++)
        tmpb[i] = 0;
    for (i = 0, tmpb = (MYFLT *)p->prev.auxp; i < winsize + 4 * FLTLEN; i++)
        tmpb[i] = 0;
    for (i = 0, tmpb = (MYFLT *)p->sin.auxp; i < p->hopsize; i++)
        tmpb[2*i] =    cos((3.14159*i)/(winsize)),
        tmpb[2*i+1] = -sin((3.14159*i)/(winsize));
 
    p->cnt = 0;
    if(*p->peak == 0 || *p->peak > MAXPEAKNOS)
     p->numpks = DEFAULTPEAKNOS;
    else
     p->numpks = *p->peak;

    if(!p->peakarray.auxp && p->peakarray.size < (p->numpks+1)*sizeof(PEAK)){
      csound->AuxAlloc(csound, (p->numpks+1)*sizeof(PEAK), &p->peakarray);
    }

    p->cnt = 0;
    p->histcnt = 0;
    p->sr = csound->GetSr(csound); 
    for (i = 0; i < NPREV; i++) p->dbs[i] = 0;
    p->amplo = MINAMPS;
    p->amphi = MAXAMPS;
    p->npartial = 7;
    p->dbfs = 32768.0/csound->e0dbfs;
    
    return (OK);
}

int pitchtrackprocess(CSOUND *csound, PITCHTRACK *p){
  MYFLT *sig = p->asig; int i;
  MYFLT *buf = (MYFLT *)p->signal.auxp;
  int pos = p->cnt, h = p->hopsize;
  MYFLT scale = p->dbfs;
  int ksmps = csound->GetKsmps(csound);

  for(i=0;i<ksmps;i++, pos++){
   if(pos == h){
     ptrack(csound,p);
     pos = 0;  
   }
   buf[pos] = sig[i]*scale;
  }
  *p->freq = p->cps;
  *p->amp =  p->dbs[p->histcnt];
  p->cnt = pos;

  return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"ptrack", S(PITCHTRACK), 5, "kk", "aio", (SUBR)pitchtrackinit, NULL, (SUBR)pitchtrackprocess}
};

LINKAGE

