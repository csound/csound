/*  
    singwave.c:

    Copyright (C) 1996, 1997 Perry Cook, John ffitch

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

/*******************************************/
/*  "Singing" Looped Soundfile Class,      */
/*  by Perry R. Cook, 1995-96              */
/*  This Object contains all that's needed */
/*  to make a pitched musical sound, like  */
/*  a simple voice or violin.  In general, */
/*  it will not be used alone (because of  */
/*  of munchinification effects from pitch */
/*  shifting.  It will be used as an       */
/*  excitation source for other instruments*/
/*******************************************/

#include "csdl.h"
#include "singwave.h"

void OneZero_setCoeff(OneZero*, MYFLT);
MYFLT Wave_tick(MYFLT *, int len, MYFLT *, MYFLT, MYFLT);

void make_SubNoise(SubNoise *p, int subSample)
{
    p->lastOutput = FL(0.0);
    p->howOften = subSample-1;
    p->counter = subSample-1;
}

MYFLT SubNoise_tick(SubNoise *p)
{
    if (p->counter==0) {
        p->lastOutput = Noise_tick(&p->lastOutput);
        p->counter = p->howOften;
    }
    else (p->counter)--;
    return p->lastOutput;
}

/*******************************************/
/*  Modulator Class, Perry R. Cook, 1995-96*/
/*  This Object combines random and        */
/*  periodic modulations to give a nice    */
/*  natural human modulation function.     */
/*******************************************/

#define POLE_POS  (FL(0.999))
#define RND_SCALE (FL(10.0))

int make_Modulatr(ENVIRON *csound,Modulatr *p, MYFLT *i)
{
    FUNC        *ftp;

    if ((ftp = csound->ftfind_(i)) != NULL)      p->wave = ftp;
    else {
      return csound->initerror_(csound->getstring_(X_382,"No table for Modulatr")); /* Expect sine wave */
    }
    p->v_time = FL(0.0);
/*     p->v_rate = 6.0; */
/*     p->vibAmt = 0.04; */
    make_SubNoise(&p->noise, 330);/* Surely this should be scaled to esr?? */
/*     p->rndAmt = 0.005; */
    make_OnePole(&p->onepole);
    OnePole_setPole(&p->onepole, POLE_POS);
    OnePole_setGain(&p->onepole, RND_SCALE * FL(0.005) /* p->rndAmt */);
    return 0;
}

#define Modulatr_setVibFreq(p,vibFreq)  (p.v_rate = vibFreq * (MYFLT)p.wave->flen/csound->esr_)
#define Modulatr_setVibAmt(p,vibAmount) (p.vibAmt = vibAmount)

MYFLT Modulatr_tick(Modulatr *p)
{
    MYFLT lastOutput;
    lastOutput = Wave_tick(&p->v_time, p->wave->flen, p->wave->ftable,
                    p->v_rate, FL(0.0));
/* printf("Wave_tick: (%f,%f) %f\n", p->v_time, p->v_rate, lastOutput); */
    lastOutput *= p->vibAmt;        /*  Compute periodic and */
    lastOutput += OnePole_tick(&p->onepole, SubNoise_tick(&p->noise));  /*   random modulations  */
/* printf("Modulatr_tick: %f\n", lastOutput); */
    return lastOutput;
}

static void Modulatr_print(Modulatr *p)
{
    printf("Modulatr: v_rate=%f v_time=%f vibAmt=%f\n",
           p->v_rate, p->v_time, p->vibAmt);
}

static int make_SingWave(ENVIRON *csound, SingWave *p, MYFLT *ifn, MYFLT *ivfn)
{
    FUNC        *ftp;

    if ((ftp = csound->ftfind_(ifn)) != NULL) p->wave = ftp;
    else {
      csound->perferror_(csound->getstring_(X_383,"No table for Singwave"));
      return NOTOK;
    }
    p->mytime = FL(0.0);
    p->rate = FL(1.0);
    p->sweepRate = FL(0.001);
    if (make_Modulatr(csound, &p->modulator, ivfn)) return NOTOK;
    Modulatr_setVibFreq(p->modulator, FL(6.0));
    Modulatr_setVibAmt(p->modulator, FL(0.04));
    make_Envelope(&p->envelope);
    make_Envelope(&p->pitchEnvelope);
    SingWave_setFreq(csound, p, FL(75.0));
    Envelope_setRate(csound, &p->pitchEnvelope, FL(1.0));
/*     SingWave_print(csound, p); */
    SingWave_tick(p);
    SingWave_tick(p);
    Envelope_setRate(csound, &p->pitchEnvelope, p->sweepRate * p->rate);
/*     Envelope_print(&p->pitchEnvelope); */
    return OK;
}

void SingWave_setFreq(ENVIRON *csound, SingWave *p, MYFLT aFreq)
{
    MYFLT temp = p->rate;

    p->rate = (MYFLT)p->wave->flen * aFreq * csound->onedsr_;
/*     printf("SingWave: rate set to %f\n", p->rate); */
    temp -= p->rate;
    if (temp<0) temp = - temp;
    Envelope_setTarget(&p->pitchEnvelope, p->rate);
    Envelope_setRate(csound, &p->pitchEnvelope, p->sweepRate * temp);
}

#define SingWave_setVibFreq(p, vibFreq) Modulatr_setVibFreq(p.modulator, vibFreq)

#define SingWave_setVibAmt(p, vibAmount)        (Modulatr_setVibAmt(p.modulator, vibAmount)

/* #define SingWave_setSweepRate(p, swpRate)    (p->sweepRate = swpRate) */

/* #define SingWave_setGainRate(p, gainRate)    Envelope_setRate(&(p.envelope), gainRate) */


MYFLT SingWave_tick(SingWave *p)
{
    MYFLT lastOutput;
    long  temp, temp1;
    MYFLT alpha, temp_rate;
    MYFLT mytime = p->mytime;

    temp_rate = Envelope_tick(&p->pitchEnvelope);
    mytime += temp_rate;                /*  Update current time            */
    mytime += temp_rate * Modulatr_tick(&p->modulator); /*  Add vibratos   */

    while (mytime >= (MYFLT)p->wave->flen) {   /*  Check for end of sound  */
        mytime -= p->wave->flen;               /*  loop back to beginning  */
    }
    while (mytime < 0.0)  {                    /*  Check for end of sound  */
        mytime += p->wave->flen;               /*  loop back to beginning  */
    }

    temp = (long) mytime;             /*  Integer part of time address    */
    alpha = mytime - (MYFLT) temp;    /*  fractional part of time address */

    temp1 = temp + 1;
    if (temp1==p->wave->flen) temp1 = temp; /* Wrap!! */
    lastOutput = alpha * p->wave->ftable[temp1];         /*  Do linear  */
    lastOutput += (FL(1.0)-alpha) * p->wave->ftable[temp];   /* interpolation */
    lastOutput *= Envelope_tick(&p->envelope);

    p->mytime = mytime;
/* printf("SingWave_tick: %f\n", lastOutput); */
    return lastOutput;
}

void SingWave_print(ENVIRON *csound, SingWave *p)
{
    printf(csound->getstring_(X_463,"SingWave: rate=%f sweepRate=%f mytime=%f\n"),
           p->rate, p->sweepRate, p->mytime);
    Modulatr_print(&p->modulator);
    Envelope_print(csound, &p->envelope);
    Envelope_print(csound, &p->pitchEnvelope);
}

/*******************************************/
/*  4 Formant Synthesis Instrument         */
/*  by Perry R. Cook, 1995-96              */
/*  This instrument contains an excitation */
/*  singing wavetable (looping wave with   */
/*  random and periodic vibrato, smoothing */
/*  on frequency, etc.), excitation noise, */
/*  and four sweepable complex resonances. */
/*                                         */
/*  Measured Formant data (from me) is     */
/*  included, and enough data is there to  */
/*  support either parallel or cascade     */
/*  synthesis.  In the floating point case */
/*  cascade synthesis is the most natural  */
/*  so that's what you'll find here.       */
/*                                         */
/*  For right now, there's a simple command*/
/*  line score interface consisting of 3   */
/*  letter symbols for the phonemes, =xx   */
/*  sets the pitch to x, + and - add and   */
/*  subtract a half step, and ... makes it */
/*  keep doing what it's doing for longer. */
/*******************************************/

char phonemes[32][4] =
    {"eee","ihh","ehh","aaa",
     "ahh","aww","ohh","uhh",
     "uuu","ooo","rrr","lll",
     "mmm","nnn","nng","ngg",
     "fff","sss","thh","shh",
     "xxx","hee","hoo","hah",
     "bbb","ddd","jjj","ggg",
     "vvv","zzz","thz","zhh"};

/* #define VoicForm_setFreq(p, frequency)       \ */
/*     SingWave_setFreq(csound, (p).voiced, frequency) */

/* #define VoicForm_setPitchSweepRate(p, rate)  \ */
/*     SingWave_setSweepRate((p).voiced, rate) */

/* #define VoicForm_speak(p)    SingWave_noteOn(p->voiced) */

#define VoicForm_setFormantAll(p,w,f,r,g) \
        FormSwep_setTargets(& p->filters[w],f,r,g)

void VoicForm_setPhoneme(VOICF *p, int i, MYFLT sc)
{
    if (i>16) i = i%16;
    VoicForm_setFormantAll(p, 0,sc*phonParams[i][0][0], phonParams[i][0][1],
                           (MYFLT)pow(10.0,phonParams[i][0][2] / FL(20.0)));
    VoicForm_setFormantAll(p, 1,sc*phonParams[i][1][0], phonParams[i][1][1],FL(1.0));
    VoicForm_setFormantAll(p, 2,sc*phonParams[i][2][0], phonParams[i][2][1],FL(1.0));
    VoicForm_setFormantAll(p, 3,sc*phonParams[i][3][0], phonParams[i][3][1],FL(1.0));
    VoicForm_setVoicedUnVoiced(p,phonGains[i][0], phonGains[i][1]);
    printf(Str(X_282,"Found Formant: %s (number %i)\n"),phonemes[i],i);
}

void VoicForm_setVoicedUnVoiced(VOICF *p, MYFLT vGain, MYFLT nGain)
{
    Envelope_setTarget(&(p->voiced.envelope), vGain);
    Envelope_setTarget(&p->noiseEnv, nGain);
}

void VoicForm_quiet(VOICF *p)
{
    Envelope_keyOff(&(p->voiced.envelope));
    Envelope_setTarget(&p->noiseEnv, FL(0.0));
}

void VoicForm_noteOff(VOICF *p)
{
    Envelope_keyOff(&p->voiced.envelope);
}

void voicprint(ENVIRON *csound, VOICF *p)
{
    SingWave_print(csound, &p->voiced);
    OneZero_print(&p->onezero);
    OnePole_print(&p->onepole);
}

static int step = 0;

int voicformset(VOICF *p)
{
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    ENVIRON *csound = p->h.insdshead->csound;

    if (make_SingWave(csound, &p->voiced, p->ifn, p->ivfn)==NOTOK) return NOTOK;
    Envelope_setRate(csound, &(p->voiced.envelope), FL(0.001));
    Envelope_setTarget(&(p->voiced.envelope), FL(0.0));

    make_Noise(p->noise);

    make_FormSwep(&p->filters[0]);
    make_FormSwep(&p->filters[1]);
    make_FormSwep(&p->filters[2]);
    make_FormSwep(&p->filters[3]);
    FormSwep_setSweepRate(p->filters[0], FL(0.001));
    FormSwep_setSweepRate(p->filters[1], FL(0.001));
    FormSwep_setSweepRate(p->filters[2], FL(0.001));
    FormSwep_setSweepRate(p->filters[3], FL(0.001));

    make_OneZero(&p->onezero);
/*     OneZero_print(&p->onezero); */
    OneZero_setCoeff(&p->onezero, - FL(0.9));
/*     OneZero_print(&p->onezero); */
    make_OnePole(&p->onepole);
    OnePole_setPole(&p->onepole, FL(0.9));

    make_Envelope(&p->noiseEnv);
    Envelope_setRate(csound, &p->noiseEnv, FL(0.001));
    Envelope_setTarget(&p->noiseEnv, FL(0.0));

    p->oldform = *p->formant;
    p->ph = (int)(FL(0.5)+ *p->phoneme);
    VoicForm_setPhoneme(p, p->ph, p->oldform);
                                /* Clear */
    /*     OnePole_clear(&p->onepole); */ /* Included in make */
    FormSwep_clear(p->filters[0]);
    FormSwep_clear(p->filters[1]);
    FormSwep_clear(p->filters[2]);
    FormSwep_clear(p->filters[3]);
    Envelope_setTarget(&(p->voiced.envelope), amp);
/*     printf("Setting onepole to %f (%f)\n", 0.95 - (amp * 0.2)/128.0, amp); */
    OnePole_setPole(&p->onepole, FL(0.95) - (amp * FL(0.2))/FL(128.0));
    p->basef = *p->frequency;
    SingWave_setFreq(csound, &p->voiced, p->basef);
/*     voicprint(csound, p); */
/*     printf("**********************************\n"); */
    step = 1;
    return OK;
}

int voicform(VOICF *p)
{
    MYFLT *ar = p->ar;
    long nsmps = ksmps;
    MYFLT temp;
    MYFLT lastOutput;
    ENVIRON *csound = p->h.insdshead->csound;

    if (p->basef != *p->frequency) {
      p->basef = *p->frequency;
      SingWave_setFreq(csound, &p->voiced, p->basef);
    }
/*     OnePole_setPole(&p->onepole, 0.95 - (amp * 0.1)); */
/*     Envelope_setTarget(&(p->voiced.envelope), amp); */
/*     Envelope_setTarget(&p->noiseEnv,   0.95 - (amp * 0.1)); */
    SingWave_setVibFreq(p->voiced, *p->vibf);
/*     printf("Vibf=%f (%f %f)\n", */
/*            *p->vibf, (MYFLT)p->voiced.modulator.wave->flen, */
/*            p->voiced.modulator.v_rate); */
    Modulatr_setVibAmt(p->voiced.modulator, *p->vibAmt);
                                /* Set phoneme */
    if (p->oldform != *p->formant || p->ph != (int)(0.5+*p->phoneme)) {
      p->oldform = *p->formant;
      p->ph = (int)(0.5 + *p->phoneme);
      printf(Str(X_461,"Setting Phoneme: %f %d\n"), p->ph, p->oldform);
      VoicForm_setPhoneme(p,(int)*p->phoneme, p->oldform);
    }
/*     voicprint(csound, p); */

    do {
/*       printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*              p->filters[0].poleCoeffs[0], p->filters[0].poleCoeffs[1], */
/*              p->filters[0].freq, p->filters[0].reson); */
/*       printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*              p->filters[0].dirty, p->filters[0].targetFreq, */
/*              p->filters[0].targetReson); */
/*       printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*              p->filters[0].targetGain, p->filters[0].currentFreq, */
/*              p->filters[0].currentReson); */
/*       printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*              p->filters[0].currentGain, p->filters[0].deltaFreq, */
/*              p->filters[0].deltaReson); */
/*       printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*              p->filters[0].deltaGain, p->filters[0].sweepState, */
/*              p->filters[0].sweepRate); */
/*       printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*              p->filters[1].poleCoeffs[0], p->filters[1].poleCoeffs[1], */
/*              p->filters[1].freq, p->filters[1].reson); */
/*       printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*              p->filters[1].dirty, p->filters[1].targetFreq, */
/*              p->filters[1].targetReson); */
/*       printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*              p->filters[1].targetGain, p->filters[1].currentFreq, */
/*              p->filters[1].currentReson); */
/*       printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*              p->filters[1].currentGain, p->filters[1].deltaFreq, */
/*              p->filters[1].deltaReson); */
/*       printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*              p->filters[1].deltaGain, p->filters[1].sweepState, */
/*              p->filters[1].sweepRate); */
/*       printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*              p->filters[2].poleCoeffs[0], p->filters[2].poleCoeffs[1], */
/*              p->filters[2].freq, p->filters[2].reson); */
/*       printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*              p->filters[2].dirty, p->filters[2].targetFreq, */
/*              p->filters[2].targetReson); */
/*       printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*              p->filters[2].targetGain, p->filters[2].currentFreq, */
/*              p->filters[2].currentReson); */
/*       printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*              p->filters[2].currentGain, p->filters[2].deltaFreq, */
/*              p->filters[2].deltaReson); */
/*       printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*              p->filters[2].deltaGain, p->filters[2].sweepState, */
/*              p->filters[2].sweepRate); */
/*       printf("FormSwep: poleCoeffs=%f, %f\tfreq=%.2f\treson=%f\n", */
/*              p->filters[3].poleCoeffs[0], p->filters[3].poleCoeffs[1], */
/*              p->filters[3].freq, p->filters[3].reson); */
/*       printf("        : dirty=%d\ttargetFreq=%f\ttargetReson=%f\n", */
/*              p->filters[3].dirty, p->filters[3].targetFreq, */
/*              p->filters[3].targetReson); */
/*       printf("        : targetGain=%f\tcurrentFreq=%f\tcurrentReson=%f\n", */
/*              p->filters[3].targetGain, p->filters[3].currentFreq, */
/*              p->filters[3].currentReson); */
/*       printf("        : currentGain=%f\tdeltaFreq=%.2f\tdeltaReson=%f\n", */
/*              p->filters[3].currentGain, p->filters[3].deltaFreq, */
/*              p->filters[3].deltaReson); */
/*       printf("        : deltaGain=%f\tsweepState=%f\tsweepRate=%f\n", */
/*              p->filters[3].deltaGain, p->filters[3].sweepState, */
/*              p->filters[3].sweepRate); */
      temp   = OnePole_tick(&p->onepole,
                            OneZero_tick(&p->onezero,
                                         SingWave_tick(&p->voiced)));
/*       printf("VF: %f(%f) ", temp, p->onezero.lastOutput); */
      temp  += Envelope_tick(&p->noiseEnv) * Noise_tick(&p->noise);
/*       printf("E:%f ", temp); */
      lastOutput  = FormSwep_tick(csound, &p->filters[0], temp);
/*       printf("F0:%f ", lastOutput); */
      lastOutput  = FormSwep_tick(csound, &p->filters[1], lastOutput);
/*       printf("F1:%f ", lastOutput); */
      lastOutput  = FormSwep_tick(csound, &p->filters[2], lastOutput);
/*       printf("F2:%f ", lastOutput); */
      lastOutput  = FormSwep_tick(csound, &p->filters[3], lastOutput);
/*       printf("F3:%f\n", lastOutput); */
      lastOutput *= FL(0.07) * FL(1.5);        /* JPff rescaled */
/* printf("Voice_tick: %f\t", lastOutput); */
      *ar++ = lastOutput * AMP_SCALE;
    } while (--nsmps);

    return OK;
}

