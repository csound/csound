/*
    bbcut.c:

    Copyright (C) 2001 Nick Collins

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
#include "stdopcod.h"
#include "interlocks.h"
#include "bbcut.h"
#include <math.h>

/* my auxilliary functions */

static inline int32_t roundoffint(MYFLT x)
{
    if (x > 0)
      return((int32_t)(x + 0.500001)); /* in case of a close rounding
                                      error when x= 0.5 +- integer */
    else
      return((int32_t)(x - 0.5));
}

static int32_t random_number(CSOUND *csound, int32_t a, int32_t b)
{
    MYFLT x;
    x = (MYFLT) (csound->Rand31(csound->RandSeed1(csound)) - 1) / FL(2147483645.0);
    return roundoffint((MYFLT) a + x * (MYFLT) (b - a));
}

static MYFLT myfltrandom(CSOUND *csound, MYFLT a, MYFLT b)
{
    MYFLT x;
    x = (MYFLT) (csound->Rand31(csound->RandSeed1(csound)) - 1) / FL(2147483645.0);
    return (a + x * (b - a));
}

static int32_t BBCutMonoInit(CSOUND *csound, BBCUTMONO *p)
{
    /* call seed random at time now? */
    /* later for efficiency- lookup table for grain envelope */
    /* int32_t i; */
    /* MYFLT t; */

    /* allocate space for a 256 point quarter sine/ exponential wavetable  */
/*     if (p->envbuffer.auxp == NULL) { */
/*       csound->AuxAlloc(csound, 256*sizeof(MYFLT),&p->envbuffer); */

/*       for (i=0;i<256;++i) { */
/*         t= (PI*0.5*(MYFLT)i)/255.0; */
/*         ((MYFLT*) (p->envbuffer.auxp))[i]=t; */
/*       } */
/*     } */
    size_t M;                      /* A temporary */

    /* have no knowledge of source length */
    p->numbarsnow  = 0;
    p->unitsdone   = 0;
    p->totalunits  = 0;
    p->unitblock   = 0;
    p->repeats     = 0;
    p->repeatsdone = 0;
    p->stutteron   = 0;

    /* allocate space- need no more than a half bar at current
       tempo and barlength */
    M = ((size_t)(CS_ESR*(*p->barlength)/(*p->bps)))*sizeof(MYFLT);
    if (p->repeatbuffer.auxp == NULL || p->repeatbuffer.size<M) {
      csound->AuxAlloc(csound, M, &p->repeatbuffer);
    }

    p->repeatsampdone = 0;

    p->Subdiv       = roundoffint(*p->subdiv);
    p->Phrasebars   = roundoffint(*p->phrasebars);
    p->Numrepeats   = roundoffint(*p->numrepeats);

    p->Stutterspeed = roundoffint(*p->stutterspeed);

    /* samp per unit= samp per bar/ subdiv */
    /* = samp per beat * beats per bar /subdiv */
    /* =(samp per sec / beats per sec)* (beats per bar/subdiv)  */
    p->samplesperunit = roundoffint(((MYFLT)CS_ESR*(FL(1.0)/(*p->bps)))*
                                    (*p->barlength/(MYFLT)p->Subdiv));

    /* enveloping */
    p->Envelopingon = roundoffint(*p->envelopingon);
    p->envsize = (p->Envelopingon) ? 64 : 0;

    return OK;
}

/* rounding errors will accumulate slowly with respect to bps,  */
/* true tempo is determined by samplesperunit (which has been rounded off) */
/* only make floating point corrections for stutters with stutterspeed>1 */

static int32_t BBCutMono(CSOUND *csound, BBCUTMONO *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t oddmax,unitproj;
    int32_t unitb,unitl,unitd;      /* temp for integer unitblock calculations */
    MYFLT envmult,out;          /* intermedaites for enveloping grains */

    if (UNLIKELY(offset)) memset(p->aout, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset;i<nsmps;i++) {
      if (UNLIKELY((p->unitsdone+FL(0.000001))>=p->totalunits)) {
        /* a new phrase of cuts */
        p->numbarsnow  = random_number(csound, 1, p->Phrasebars);
        p->totalunits  = p->numbarsnow*p->Subdiv;

        p->unitsdone   = 0;
        p->unitsleft   = (MYFLT)p->totalunits;    /* must reset here */
        p->repeats     = 0;
        p->repeatsdone = 0;
        p->stutteron   = 0;
      }

      if (p->repeatsdone>=p->repeats) {
        /* a new subphrase- a cut + some repeats of it */
        p->repeatsdone = 0;

        /* STUTTER- only within half a bar of the end */
        if (UNLIKELY((*p->stutterchance > myfltrandom(csound, FL(0.0), FL(1.0))) &&
                     (p->unitsleft<(p->Subdiv/2)))) {
          /* stutterspeed must be an integer greater than zero */

          p->repeats = roundoffint(p->unitsleft*p->Stutterspeed);
          p->unitblock = FL(1.0)/(p->Stutterspeed);
          p->stutteron = 1;
        }
        else {  /* NO STUTTER */
          /* finding set of valid odd numbers for syncopated cuts */
          /* integer division */
          oddmax = p->Subdiv/2;
          if ((oddmax&1)==0)
            oddmax =(oddmax-2)/2;
          else
            oddmax = (oddmax-1)/2;

          unitb = random_number(csound, 0, oddmax);
          unitb = (2*unitb)+1;

          unitl = roundoffint(p->unitsleft);

          while (unitb> unitl)
            unitb = unitb-2;

          unitd = roundoffint(p->unitsdone);

          /* for debug testing           */
          /* if (unitblock<=0, {"this should never happen".postln}); */
          /* p->repeats is the total number of repeats, including
             initial statement */
          /* usually 1 or 2  */
          p->repeats = random_number(csound, 1, p->Numrepeats + 1);
          /* take right arg as p->Numrepeats+1 if make param more sensible */
          unitproj = (p->repeats*unitb)+ unitd;
          /* should be same logic as old algorithm for numrepeats=2 */
          while (unitproj> p->totalunits) {
            p->repeats = p->repeats-1;

            if (p->repeats<=1) {
              p->repeats = 1;
              unitb = unitl;    /* at this point is an integer */
            }
            unitproj =(p->repeats*unitb)+ unitd;
          }

          /* convert integer to float */
          p->unitblock = (MYFLT) unitb;
        }       /* end of stutter/no stutter */

        /* determine offset - this part is very different for the csound
           version */
        /* there is no random access possible any more, only have now
           (and past) */

        /* we must determine how long in samples a repeat is */
        /* must persist between calls to this function */

        p->repeatlengthsamp = roundoffint(p->unitblock*p->samplesperunit);

        p->repeatsampdone = 0;

        /* determine envelope size - default is always 0 if enveloping off,
           128 if on; */

        /* envsize must be at most a quarter of repeatsamplelength */
        if ((p->Envelopingon) ==1) {
          if (p->repeatlengthsamp<256) {
            p->envsize = p->repeatlengthsamp/4;
          }
        }

      }

      /* AUDIO OUT */
      if (p->repeatsdone==0) {
        out = p->ain[i];        /* pass in directly to out */

        /* ENVELOPING */
        envmult = FL(1.0);

        /* envelope in */
        if (p->repeatsampdone<p->envsize) {
          /* used sinusoid- prefer exponential */
        /* envmult= sin(PI*0.5*(((MYFLT)(p->repeatsampdone))/(MYFLT)p->envsize)); */
          envmult = (EXP((p->repeatsampdone)/(p->envsize))-FL(1.0))/
            FL(1.7182818284590);
        }

        /* envelope out if necessary */
        if (p->repeatsampdone>=(p->repeatlengthsamp-p->envsize)) {
          MYFLT xx = p->envsize; /* JPff patch 2019 Apr 28 */
          if (xx==0.0) xx = 00.1;
          /* envmult = sin(PI*0.5*
             (((MYFLT)(p->repeatlengthsamp-p->repeatsampdone))/
             (MYFLT)p->envsize)); */
          envmult = (EXP(((p->repeatlengthsamp-p->repeatsampdone))/
                         (xx))-FL(1.0))/
            FL(1.7182818284590);
        }

        out *= envmult;
        /* /ENVELOPING DONE */

        p->aout[i] = out;

        if (p->repeats>1) {     /* if recording a repeat */
          ((MYFLT*)(p->repeatbuffer.auxp))[p->repeatsampdone] = out;
        }
      }
      else {    /* reading repeatbuffer for repeats */
        p->aout[i] = ((MYFLT*)(p->repeatbuffer.auxp))[p->repeatsampdone];
      }

      /* per sample accounting */
      ++(p->repeatsampdone);

      /* if finished a cut, do accounting  */
      if (UNLIKELY(p->repeatsampdone>=p->repeatlengthsamp)) {
        ++(p->repeatsdone);
        p->repeatsampdone = 0;

        p->unitsdone = p->unitsdone + p->unitblock;

        p->unitsleft = p->totalunits - p->unitsdone;

        /* to account for floating point errors in unitblock when  */
        /* stuttering with stutterspeed > 1 */
        if (UNLIKELY(p->stutteron && (p->repeatsdone==(p->repeats-1)))) {
          p->unitblock = p->unitsleft;
        }
      }
    }

    return OK;
}

/* Stereo versions */
/* This following code is excatly the same as above, but */
/* uses BBCUTSTEREO- changes are doubling of buffer size
   for interleaved stereo save and code in audio output part
   to cope with that, variables out1,out2 for 2 channels */

static int32_t BBCutStereoInit(CSOUND *csound, BBCUTSTEREO * p)
{
    /* call seed random at time now? */

    /* later for efficiency- lookup table for grain envelope */
    /* int32_t i; */
    /* MYFLT t; */

       /* allocate space for a 256 point quarter sine/ exponential wavetable  */
/*     if (p->envbuffer.auxp == NULL) { */
/*       csound->AuxAlloc(csound, ((int32_t)(256*sizeof(MYFLT),&p->envbuffer); */

/*                 for (i=0;i<256;++i) */
/*       { */
/*         t= (PI*0.5*(MYFLT)i)/255.0; */
/*         ((MYFLT*) (p->envbuffer.auxp))[i]=t; */
/*       } */
/*                 } */

    size_t M;                      /* temporary */

    /* have no knowledge of source length */

    p->numbarsnow = 0;
    p->unitsdone = 0;
    p->totalunits = 0;
    p->unitblock = 0;
    p->repeats = 0;
    p->repeatsdone = 0;
    p->stutteron = 0;

    /* allocate space- need no more than a half bar at current tempo
       and barlength */
    M = 2*((size_t)(CS_ESR*(*p->barlength)/(*p->bps)))*sizeof(MYFLT);
    if (p->repeatbuffer.auxp == NULL || p->repeatbuffer.size<M) {
      /* multiply by 2 for stereo buffer */
      csound->AuxAlloc(csound, M, &p->repeatbuffer);
    }

    p->repeatsampdone = 0;
    p->Subdiv = roundoffint(*p->subdiv);
    p->Phrasebars = roundoffint(*p->phrasebars);
    p->Numrepeats = roundoffint(*p->numrepeats);

    p->Stutterspeed = roundoffint(*p->stutterspeed);

    /* samp per unit= samp per bar/ subdiv */
    /* = samp per beat * beats per bar /subdiv */
    /* =(samp per sec / beats per sec)* (beats per bar/subdiv)  */
    p->samplesperunit = roundoffint(((MYFLT)CS_ESR/
                                     (*p->bps))*(*p->barlength/
                                                 (MYFLT)p->Subdiv));

    /* enveloping */
    p->Envelopingon = roundoffint(*p->envelopingon);
    p->envsize = (p->Envelopingon) ? 64 : 0;

    return OK;
}

/* rounding errors will accumulate slowly with respect to bps,  */
/* true tempo is determined by samplesperunit (which has been rounded off) */
/* only make floating point corrections for stutters with stutterspeed>1 */
static int32_t BBCutStereo(CSOUND *csound, BBCUTSTEREO *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32_t oddmax,unitproj;
    int32_t unitb,unitl,unitd;      /* temp for integer unitblock calculations */
    MYFLT envmult,out1,out2;/* intermediates for enveloping grains */

    if (UNLIKELY(offset)) {
      memset(p->aout1, '\0', offset*sizeof(MYFLT));
      memset(p->aout2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->aout1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&p->aout2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (i=offset;i<nsmps;i++) {
      /* a new phrase of cuts */
      if (UNLIKELY((p->unitsdone+FL(0.000001))>=p->totalunits)) {
        p->numbarsnow  = random_number(csound, 1, p->Phrasebars);
        p->totalunits  = p->numbarsnow*p->Subdiv;

        p->unitsdone   = 0;
        p->unitsleft   = (MYFLT)p->totalunits;    /* must reset here */
        p->repeats     = 0;
        p->repeatsdone = 0;
        p->stutteron   = 0;
      }

      if (UNLIKELY(p->repeatsdone>=p->repeats)) {
        /* a new subphrase- a cut + some repeats of it */
        p->repeatsdone = 0;

        /* STUTTER- only within half a bar of the end */
        if (UNLIKELY((*p->stutterchance > myfltrandom(csound, FL(0.0), FL(1.0))) &&
                     (p->unitsleft<(p->Subdiv/2)))) {
          /* stutterspeed must be an integer greater than zero */

          p->repeats   = roundoffint(p->unitsleft*p->Stutterspeed);
          p->unitblock = FL(1.0)/p->Stutterspeed;
          p->stutteron = 1;
        }
        else {  /* NO STUTTER */
          /* finding set of valid odd numbers for syncopated cuts */
          /* integer division */
          oddmax = p->Subdiv/2;
          if ((oddmax & 1)==0)
            oddmax = (oddmax-2)/2;
          else
            oddmax = (oddmax-1)/2;

          unitb = random_number(csound, 0, oddmax);
          unitb = (2*unitb)+1;

          unitl = roundoffint(p->unitsleft);

          while (unitb> unitl)
            unitb = unitb-2;

          unitd = roundoffint(p->unitsdone);

          /* for debug testing           */
          /* if (unitblock<=0, {"this should never happen".postln}); */

          /* p->repeats is the total number of repeats, including
             initial statement */
          /* usually 1 or 2  */
          p->repeats = random_number(csound, 1, p->Numrepeats + 1);
          /* take right arg as p->Numrepeats+1 if make param more sensible */

          unitproj = (p->repeats*unitb)+ unitd;

          /* should be same logic as old algorithm for numrepeats=2 */
          while (unitproj> p->totalunits) {
            p->repeats = p->repeats-1;

            if (p->repeats<=1) {
              p->repeats = 1;
              unitb = unitl;    /* at this point is an integer */
            }

            unitproj = (p->repeats*unitb)+ unitd;
          }

          /* convert integer to float */
          p->unitblock = (MYFLT) unitb;

        }       /* end of stutter/no stutter */

        /* determine offset- this part is very different for the
           csound version */
        /* there is no random access possible any more, only have now
           (and past) */

        /* we must determine how long in samples a repeat is */
        /* must persist between calls to this function */

        p->repeatlengthsamp = roundoffint(p->unitblock*p->samplesperunit);

        p->repeatsampdone = 0;

        /* determine envelope size- default is always 0 if enveloping off,
           128 if on; */

        /* envsize must be at most a quarter of repeatsamplelength */
        if (p->Envelopingon ==1) {
          if (p->repeatlengthsamp<256) {
            p->envsize = p->repeatlengthsamp/4;
          }
        }
      }

      /* AUDIO OUT- some changes for buffer access */
      if (p->repeatsdone == 0) {

        out1 = p->ain1[i];      /* pass in directly to out */
        out2 = p->ain2[i];

        /* ENVELOPING */
        envmult = FL(1.0);

        /* envelope in */
        if (p->repeatsampdone<p->envsize) {
          /* used sinusoid- prefer exponential */
          /* envmult = sin(PI*0.5*(((MYFLT)(p->repeatsampdone))/
             (MYFLT)p->envsize)); */
          envmult = (EXP((p->repeatsampdone)/
                                (p->envsize))-
                     FL(1.0))/FL(1.7182818284590);
        }

        /* envelope out if necessary */
        if (p->repeatsampdone>=(p->repeatlengthsamp-p->envsize)) {
   /* envmult = sin(PI*0.5*(((MYFLT)(p->repeatlengthsamp-p->repeatsampdone))/
      (MYFLT)p->envsize)); */
          MYFLT xx = p->envsize;
          if (xx==FL(0.0)) xx = 0.001; /* JPff patch 2019 Apr 28 */
          envmult = (EXP(((p->repeatlengthsamp-
                                          p->repeatsampdone))/
                                (xx))-FL(1.0))/
            FL(1.7182818284590);
        }

        out1 *= envmult;
        out2 *= envmult;
        /* /ENVELOPING DONE */

        p->aout1[i] = out1;
        p->aout2[i] = out2;

        if (p->repeats>1) {     /* if recording a repeat */
          /* STEREO INTERLEAVED */
          ((MYFLT*)(p->repeatbuffer.auxp))[2*p->repeatsampdone] = out1;
          ((MYFLT*)(p->repeatbuffer.auxp))[2*p->repeatsampdone+1] = out2;
        }

      }
      else {    /* reading repeatbuffer for repeats */
        p->aout1[i] = ((MYFLT*)(p->repeatbuffer.auxp))[2*p->repeatsampdone];
        p->aout2[i] = ((MYFLT*)(p->repeatbuffer.auxp))[2*p->repeatsampdone+1];
      }

      /* per sample accounting */
      ++(p->repeatsampdone);

      /* if finished a cut, do accounting  */
      if (p->repeatsampdone>=p->repeatlengthsamp) {
        ++(p->repeatsdone);
        p->repeatsampdone = 0;

        p->unitsdone = p->unitsdone+ p->unitblock;

        p->unitsleft = p->totalunits- p->unitsdone;

        /* to account for floating point errors in unitblock when  */
        /* stuttering with stutterspeed > 1 */
        if (p->stutteron && (p->repeatsdone== (p->repeats-1))) {
          p->unitblock = p->unitsleft;
        }
      }
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "bbcutm",S(BBCUTMONO), 0,  "a","aiiiiipop",
                                 (SUBR)BBCutMonoInit, (SUBR)BBCutMono  },
  { "bbcuts",S(BBCUTSTEREO), 0, "aa","aaiiiiipop",
                               (SUBR)BBCutStereoInit, (SUBR)BBCutStereo}
};

int32_t bbcut_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

