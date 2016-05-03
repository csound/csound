/*  pvscent.c:
    Calculation of spectral centroid as Beauchamp

    (c) John ffitch, 2005
    (c) Alan OCinneide, 2005
    (c) V Lazzarin, 2012

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

#include "pvs_ops.h"
#include "pstream.h"

typedef struct {
  OPDS    h;
  MYFLT   *ans;
  PVSDAT  *fin;
  uint32  lastframe;
  MYFLT   old;
} PVSCENT;

static int pvscentset(CSOUND *csound, PVSCENT *p)
{
    *p->ans = FL(0.0);
    p->lastframe = 0;
    if (UNLIKELY(!(p->fin->format==PVS_AMP_FREQ) ||
                 (p->fin->format==PVS_AMP_PHASE)))
      return csound->InitError(csound,
                               Str("pvscent: format must be amp-phase"
                                   " or amp-freq.\n"));
    return OK;
}

static int pvscent(CSOUND *csound, PVSCENT *p)
{
    int32 i,N = p->fin->N;
    MYFLT c = FL(0.0);
    MYFLT d = FL(0.0);
    MYFLT j, binsize = CS_ESR/(MYFLT)N;
    if (p->fin->sliding) {
      CMPLX *fin = (CMPLX*) p->fin->frame.auxp;
      int NB = p->fin->NB;
      for (i=0, j=FL(0.5)*binsize; i<NB; i++, j += binsize) {
        c += fin[i].re*j;
        d += fin[i].re;
      }
    }
    else {
      float *fin = (float *) p->fin->frame.auxp;
      if (p->lastframe < p->fin->framecount) {
        //printf("N=%d binsize=%f\n", N, binsize);
        for (i=0,j=FL(0.5)*binsize; i<N+2; i+=2, j += binsize) {
          c += fin[i]*j;         /* This ignores phase */
          d += fin[i];
          //printf("%d (%f) sig=%f c=%f d=%f\n", i,j,fin[i],c,d);
        }
        p->lastframe = p->fin->framecount;
      }
    }
    *p->ans = (d==FL(0.0) ? FL(0.0) : c/d);
    return OK;
}

static int pvsscent(CSOUND *csound, PVSCENT *p)
{
    MYFLT *a = p->ans;
    if (p->fin->sliding) {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      int32 i,N = p->fin->N;

      MYFLT c = FL(0.0);
      MYFLT d = FL(0.0);
      MYFLT j, binsize = CS_ESR/(MYFLT)N;
      int NB = p->fin->NB;
      if (UNLIKELY(offset)) memset(a, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&a[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++) {
        CMPLX *fin = (CMPLX*) p->fin->frame.auxp + n*NB;
        for (i=0,j=FL(0.5)*binsize; i<N+2; i+=2, j += binsize) {
          c += j*fin[i].re;         /* This ignores phase */
          d += fin[i].re;
        }
        a[n] = (d==FL(0.0) ? FL(0.0) : c/d);
      }
    }
    else {
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      MYFLT old = p->old;
      int32 i,N = p->fin->N;
      MYFLT c = FL(0.0);
      MYFLT d = FL(0.0);
      MYFLT j, binsize = CS_ESR/(MYFLT)N;
      float *fin = (float *) p->fin->frame.auxp;
      nsmps -= early;
      for (n=offset; n<nsmps; n++) {
        if (p->lastframe < p->fin->framecount) {
          for (i=0,j=FL(0.5)*binsize; i<N+2; i+=2, j += binsize) {
            c += fin[i]*j;         /* This ignores phase */
            d += fin[i];
          }
          old = a[n] = (d==FL(0.0) ? FL(0.0) : c/d);
          p->lastframe = p->fin->framecount;
        }
        else {
          a[n] = old;
        }
      }
      p->old = old;
    }
    return OK;
}

typedef struct _cent {
  OPDS    h;
  MYFLT   *ans;
  MYFLT  *asig, *ktrig, *ifftsize;
  uint32_t fsize, count;
  MYFLT old;
  void *setup;
  AUXCH frame, windowed, win;
} CENT;

static int cent_i(CSOUND *csound, CENT *p)
{
    int fftsize = *p->ifftsize;
    p->count = 0;
    p->fsize = 1;
    while(fftsize >>= 1) p->fsize <<= 1;
    if (p->fsize < *p->ifftsize) {
      p->fsize <<= 1;
      csound->Warning(csound,
                      Str("centroid requested fftsize = %.0f, actual = %d\n"),
                      *p->ifftsize, p->fsize);
    }
    if (p->frame.auxp == NULL || p->frame.size < p->fsize*sizeof(MYFLT))
      csound->AuxAlloc(csound, p->fsize*sizeof(MYFLT), &p->frame);
    if (p->windowed.auxp == NULL || p->windowed.size < p->fsize*sizeof(MYFLT))
      csound->AuxAlloc(csound, p->fsize*sizeof(MYFLT), &p->windowed);
    if (p->win.auxp == NULL || p->win.size < p->fsize*sizeof(MYFLT)) {
      unsigned int i;
      MYFLT *win;
      csound->AuxAlloc(csound, p->fsize*sizeof(MYFLT), &p->win);
      win = (MYFLT *) p->win.auxp;
    for (i=0; i < p->fsize; i++)
      win[i] = 0.5 - 0.5*cos(i*TWOPI/p->fsize);
    }
    p->old = 0;
    memset(p->frame.auxp, 0, p->fsize*sizeof(MYFLT));
    memset(p->windowed.auxp, 0, p->fsize*sizeof(MYFLT));
    p->setup = csound->RealFFT2Setup(csound,p->fsize,FFT_FWD); 
    return OK;
}


static int cent_k(CSOUND *csound, CENT *p)
{
    unsigned int n = p->count, k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT *frame = (MYFLT *) p->frame.auxp, *asig = p->asig;

    uint32_t fsize = (uint32_t)p->fsize;
    if (UNLIKELY(early)) nsmps -= early;
    for (i=offset; i < nsmps; i++){
      frame[n] = asig[i];
      if (n == fsize-1) n=0;
      else n++;
    }

    if (*p->ktrig) {
      MYFLT c = FL(0.0);
      MYFLT d = FL(0.0);
      MYFLT *windowed = (MYFLT *) p->windowed.auxp;
      MYFLT *win = (MYFLT *) p->win.auxp;
      MYFLT mag, cf, binsize = CS_ESR/(MYFLT)fsize;
      for (i=0,k=n; i < fsize; i++){
        windowed[i] = frame[k]*win[i];
        if (k == fsize-1) k=0;
        else k++;
      }
      csound->RealFFT2(csound, p->setup, windowed);
      cf=FL(0.5)*binsize;
      mag = windowed[0];
      c += mag*cf;
      d += mag;
      cf += binsize;
      for (i=2; i < fsize; i+=2, cf += binsize) {
        mag = sqrt(windowed[i]*windowed[i] + windowed[i+1]*windowed[i+1]);
        c += mag*cf;
        d += mag;
      }
      p->old = *p->ans = (d==FL(0.0) ? FL(0.0) : c/d);
    } else *p->ans = p->old;
    p->count = n;
    return OK;
}




/* PVSPITCH opcode by Ala OCinneide */

typedef struct _pvspitch
{
  /* OPDS data structure */
  OPDS    h;

  /* Output */
  MYFLT   *kfreq;
  MYFLT   *kamp;

  /* Inputs */
  PVSDAT  *fin;
  MYFLT   *ithreshold;

  /* Internal arrays */
  AUXCH peakfreq;
  AUXCH inharmonic;

  uint32 lastframe;

} PVSPITCH;


#if !defined(FALSE)
#define FALSE (0)
#endif
#if !defined(TRUE)
#define TRUE (!FALSE)
#endif

#define RoundNum(Number)  (int)MYFLT2LRND(Number)

/* Should one use remainder or drem ?? */
#define Remainder(Numerator, Denominator)               \
  Numerator/Denominator - (int) (Numerator/Denominator)


int pvspitch_init(CSOUND *csound, PVSPITCH *p)
{
    /* Initialise frame count to zero. */
    unsigned int size;
    p->lastframe = 0;

    if (UNLIKELY(p->fin->sliding))
      return csound->InitError(csound, Str("SDFT case not implemented yet"));
    size = sizeof(MYFLT)*(p->fin->N+2);
    if (p->peakfreq.auxp == NULL || p->peakfreq.size < size)
      csound->AuxAlloc(csound, size, &p->peakfreq);
    if (p->inharmonic.auxp == NULL || p->inharmonic.size < size)
      csound->AuxAlloc(csound, size, &p->inharmonic);
    if (UNLIKELY(p->fin->format!=PVS_AMP_FREQ)) {
      return csound->InitError(csound,
                               Str("PV Frames must be in AMP_FREQ format!\n"));
    }

    return OK;
}

int pvspitch_process(CSOUND *csound, PVSPITCH *p)
{
    /* Initialised inputs */
    float *Frame            = (float *) p->fin->frame.auxp;
    MYFLT *PeakFreq         = (MYFLT *) p->peakfreq.auxp;
    MYFLT *inharmonic       = (MYFLT *) p->inharmonic.auxp;
    MYFLT Threshold         = (MYFLT) *p->ithreshold;
    int fftsize             = (int) p->fin->N;
    int numBins             = fftsize/2 + 1;

    MYFLT f0Cand, Frac, Freq = FL(0.0);
    int i, j,  P1, P2, maxPartial;
    MYFLT lowHearThreshold  = FL(20.0);
    MYFLT Amp               = FL(0.0);
    int Partial             = 0;
    int     numPeaks        = 0;
    int maxAdj              = 3;
    int Adj                 = FALSE;
    int PrevNotAdj          = FALSE;

    /* Un-normalise the threshold value */
    Threshold *= csound->e0dbfs;

    /* If a new frame is ready... */
    if (p->lastframe < p->fin->framecount) {
      /* Finds the peaks in the frame. */
      for (i=1; i<(numBins-1) && numPeaks<numBins/2; i++) {
        /* A peak is defined as being above the threshold and */
        /* greater than both its neighbours... */
        if (Frame[2*i] > Threshold &&
            Frame[2*i] > Frame[2*(i-1)] &&
            Frame[2*i] > Frame[2*(i+1)]) {
          PeakFreq[numPeaks]=Frame[2*i+1];
          numPeaks++;
          i++; /* Impossible to have two peaks in a row, skip over the next. */
        }

        Amp += Frame[2*i];
      }

      Amp += Frame[0];
      Amp += Frame[2*numBins];
      Amp *= FL(0.5);

      if (UNLIKELY(numPeaks==0)) {
        /* If no peaks found return 0. */
        Partial = 0;
      }
      else {
        /* Threshold of hearing is 20 Hz, so no need to look beyond
           there for the fundamental. */
        maxPartial = (int) (PeakFreq[0]/lowHearThreshold);

        /* Calculates the inharmonicity for each fundamental candidate */
        for (i=0; i<maxPartial && i < numBins/2; i++) {
          inharmonic[i] = FL(0.0);
          f0Cand = PeakFreq[0]/(i+1);

          for (j=1; j<numPeaks; j++) {
            Frac = Remainder(PeakFreq[j], f0Cand);
            if (Frac > FL(0.5)) Frac = FL(1.0) - Frac;
            Frac /= PeakFreq[j];

            inharmonic[i]+=Frac;
          }

          /* Test for the adjacency of partials... */
          for (j=0; j<numPeaks-1; j++) {
            P1 = RoundNum(PeakFreq[j]/f0Cand);
            P2 = RoundNum(PeakFreq[j+1]/f0Cand);

            if (P2-P1<maxAdj && P2-P1!=0) {
              Adj = TRUE;
              break;
            }
            else Adj = FALSE;
          }

          /* Search for the fundamental with the least inharmonicity */
          if (i==0 ||
              (i>0 && inharmonic[i]<inharmonic[Partial-1]) ||
              (i>0 && PrevNotAdj && Adj)) {
            /* The best candidate so far... */
            if (Adj) {
              Partial = i+1;
              PrevNotAdj = FALSE;
            }
            else if (i==0) {
              Partial = i+1;
              PrevNotAdj = TRUE;
            }
            else PrevNotAdj = TRUE;

          }
        }
      }

      /* Output the appropriate frequency values. */
      if (LIKELY(Partial!=0)) {
        f0Cand = PeakFreq[0]/Partial;
        /* Average frequency between partials */
        for (i=0; i<numPeaks; i++) {
          Freq += PeakFreq[i] / RoundNum(PeakFreq[i]/f0Cand);
        }
        Freq /= numPeaks;
        *p->kfreq = Freq;
      }
      else {
        *p->kfreq = FL(0.0);
      }

      *p->kamp  = Amp;

      /* Update the frame count */
      p->lastframe = p->fin->framecount;
    }

    return OK;
}

static OENTRY localops[] = {
  { "pvscent", sizeof(PVSCENT), 0, 3, "s", "f",
                             (SUBR)pvscentset, (SUBR)pvscent, (SUBR)pvsscent },
  { "centroid", sizeof(CENT), 0, 3, "k", "aki", (SUBR)cent_i, (SUBR)cent_k, NULL},
  { "pvspitch", sizeof(PVSPITCH), 0, 3, "kk", "fk",
                            (SUBR)pvspitch_init, (SUBR)pvspitch_process, NULL}
};

int pvscent_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int) (sizeof(localops) / sizeof(OENTRY)));
}
