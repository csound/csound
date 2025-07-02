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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "singwave.h"
#include "moog1.h"

void OneZero_setCoeff(OneZero*, MYFLT);
MYFLT Wave_tick(MYFLT *, int32_t len, MYFLT *, MYFLT, MYFLT);

static void SingWave_setFreq(CSOUND *csound, SingWave *p, MYFLT aFreq);
static MYFLT SingWave_tick(CSOUND *csound, SingWave *p);
static void VoicForm_setVoicedUnVoiced(VOICF *p, MYFLT vGain, MYFLT nGain);

static inline void make_SubNoise(SubNoise *p, int32_t subSample)
{
    p->lastOutput = FL(0.0);
    p->howOften = p->counter = subSample-1;
}

static MYFLT SubNoise_tick(CSOUND *csound, SubNoise *p)
{
    MYFLT last;
    if (p->counter==0) {
      last = p->lastOutput = Noise_tick(csound, &p->lastOutput);
      p->counter = p->howOften;
    }
    else {
      (p->counter)--;
      last = p->lastOutput;
    }
    return last;
}

/*******************************************/
/*  Modulator Class, Perry R. Cook, 1995-96*/
/*  This Object combines random and        */
/*  periodic modulations to give a nice    */
/*  natural human modulation function.     */
/*******************************************/

#define POLE_POS  (FL(0.999))
#define RND_SCALE (FL(10.0))

static int32_t make_Modulatr(CSOUND *csound,Modulatr *p, MYFLT *i)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound,i)) != NULL))
      p->wave = ftp;
    else { /* Expect sine wave */
      return csound->InitError(csound, "%s", Str("No table for Modulatr"));
    }
    p->v_time = FL(0.0);
/*  p->v_rate = 6.0; */
/*  p->vibAmt = 0.04; */
    make_SubNoise(&p->noise, 330);/* Surely this should be scaled to esr?? */
/*  p->rndAmt = 0.005; */
    make_OnePole(&p->onepole);
    OnePole_setPole(&p->onepole, POLE_POS);
    OnePole_setGain(&p->onepole, RND_SCALE * FL(0.005) /* p->rndAmt */);
    return 0;
}

#define Modulatr_setVibFreq(p,vibFreq)  \
        (p.v_rate = vibFreq * (MYFLT)p.wave->flen*CS_ONEDSR)
#define Modulatr_setVibAmt(p,vibAmount) (p.vibAmt = vibAmount)

static MYFLT Modulatr_tick(CSOUND *csound, Modulatr *p)
{
    MYFLT lastOutput;
    lastOutput = Wave_tick(&p->v_time, p->wave->flen, p->wave->ftable,
                           p->v_rate, FL(0.0));
    lastOutput *= p->vibAmt;        /*  Compute periodic and */
    /*   random modulations  */
    lastOutput += OnePole_tick(&p->onepole, SubNoise_tick(csound, &p->noise));
    return lastOutput;
}

#if 0
static void Modulatr_print(CSOUND *csound, Modulatr *p)
{
    csound->Message(csound, "Modulatr: v_rate=%f v_time=%f vibAmt=%f\n",
                            p->v_rate, p->v_time, p->vibAmt);
}
#endif

static int32_t make_SingWave(CSOUND *csound, SingWave *p, MYFLT *ifn, MYFLT *ivfn)
{
    FUNC        *ftp;

    if (LIKELY((ftp = csound->FTFind(csound,ifn)) != NULL)) p->wave = ftp;
    else {
      return csound->InitError(csound, "%s", Str("No table for Singwave"));
    }
    p->mytime = FL(0.0);
    p->rate = FL(1.0);
    p->sweepRate = FL(0.001);
    if (UNLIKELY(make_Modulatr(csound, &p->modulator, ivfn))) return NOTOK;
    Modulatr_setVibFreq(p->modulator, FL(6.0));
    Modulatr_setVibAmt(p->modulator, FL(0.04));
    make_Envelope(&p->envelope);
    //    printf("Singwave envelope=%p\n", &p->envelope);
    /* Envelope_setTarget(&p->envelope, FL(1.0)); */
    /* Envelope_setRate(csound, &p->envelope, FL(0.1)); */
    make_Envelope(&p->pitchEnvelope);
    //    printf("Singwave pitchenvelope=%p\n", &p->pitchEnvelope);
    SingWave_setFreq(csound, p, FL(75.0));
    Envelope_setRate(csound, &p->pitchEnvelope, FL(1.0));
/*  SingWave_print(csound, p); */
    SingWave_tick(csound, p);
    SingWave_tick(csound, p);
    Envelope_setRate(csound, &p->pitchEnvelope, p->sweepRate * p->rate);
/*  Envelope_print(&p->pitchEnvelope); */
    return OK;
}

static void SingWave_setFreq(CSOUND *csound, SingWave *p, MYFLT aFreq)
{
    MYFLT temp = p->rate;

    p->rate = (MYFLT)p->wave->flen * aFreq * CS_ONEDSR;
    temp -= p->rate;
    temp = FABS(temp);
    Envelope_setTarget(&p->pitchEnvelope, p->rate);
    Envelope_setRate(csound, &p->pitchEnvelope, p->sweepRate * temp);
    //    Envelope_print(csound, &p->pitchEnvelope);
}

#define SingWave_setVibFreq(p, vibFreq) \
            Modulatr_setVibFreq(p.modulator, vibFreq)

#define SingWave_setVibAmt(p, vibAmount) \
            Modulatr_setVibAmt(p.modulator, vibAmount)

static MYFLT SingWave_tick(CSOUND *csound, SingWave *p)
{
    MYFLT lastOutput;
    int32  temp, temp1;
    MYFLT alpha, temp_rate;
    MYFLT mytime = p->mytime;

    temp_rate = Envelope_tick(&p->pitchEnvelope);
    //    printf("SingWave_tick: %f\n", temp_rate);
    mytime += temp_rate;                      /*  Update current time     */
    mytime += temp_rate*Modulatr_tick(csound,&p->modulator); /* Add vibratos */
    //printf("             : %f %d\n", mytime, p->wave->flen);
    while (mytime >= (MYFLT)p->wave->flen) {  /*  Check for end of sound  */
      mytime -= p->wave->flen;                /*  loop back to beginning  */
    }
    while (mytime < FL(0.0)) {                /*  Check for end of sound  */
      mytime += p->wave->flen;                /*  loop back to beginning  */
    }

    temp = (int32) mytime;             /*  Integer part of time address    */
    alpha = mytime - (MYFLT) temp;    /*  fractional part of time address */

    temp1 = temp + 1;
    if (temp1==(int32_t)p->wave->flen) temp1 = temp; /* Wrap!! */

    lastOutput = alpha * p->wave->ftable[temp1];         /*  Do linear  */
    //    printf("             : (%d %d) %f %f ", temp, temp1, alpha, lastOutput);
    lastOutput += (FL(1.0)-alpha) * p->wave->ftable[temp];   /* interpolation */
    //    printf("%f ", lastOutput);
    lastOutput *= Envelope_tick(&p->envelope);
//    printf("%f\n", lastOutput);
    p->mytime = mytime;
    return lastOutput;
}

#if 0
static void SingWave_print(CSOUND *csound, SingWave *p)
{
    csound->Message(csound, Str("SingWave: rate=%f sweepRate=%f mytime=%f\n"),
                            p->rate, p->sweepRate, p->mytime);
    Modulatr_print(csound, &p->modulator);
    //    Envelope_print(csound, &p->envelope);
    //    Envelope_print(csound, &p->pitchEnvelope);
}
#endif

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

static void VoicForm_setPhoneme(CSOUND *csound, VOICF *p, int32_t i, MYFLT sc)
{
    if (i>16) i = i%16;
    VoicForm_setFormantAll(p, 0,sc*phonParams[i][0][0], phonParams[i][0][1],
                           (MYFLT)pow(10.0,phonParams[i][0][2] / FL(20.0)));
    VoicForm_setFormantAll(p, 1,sc*phonParams[i][0][0], phonParams[i][1][1],
                           (MYFLT)pow(10.0,phonParams[i][1][2] / FL(20.0)));
    VoicForm_setFormantAll(p, 2,sc*phonParams[i][0][0], phonParams[i][2][1],
                           (MYFLT)pow(10.0,phonParams[i][2][2] / FL(20.0)));
    VoicForm_setFormantAll(p, 3,sc*phonParams[i][0][0], phonParams[i][3][1],
                           (MYFLT)pow(10.0,phonParams[i][3][2] / FL(20.0)));
     /* VoicForm_setFormantAll(p, 1,sc*phonParams[i][1][0], */
    /*                        phonParams[i][1][1], FL(1.0)); */
    /* VoicForm_setFormantAll(p, 2,sc*phonParams[i][2][0], */
    /*                        phonParams[i][2][1], FL(1.0)); */
    /* VoicForm_setFormantAll(p, 3,sc*phonParams[i][3][0], */
    /*                        phonParams[i][3][1], FL(1.0)); */
    VoicForm_setVoicedUnVoiced(p,phonGains[i][0], phonGains[i][1]);
    csound->Message(csound,
                    Str("Found Formant: %s (number %i)\n"), phonemes[i], i);
}

static void VoicForm_setVoicedUnVoiced(VOICF *p, MYFLT vGain, MYFLT nGain)
{
    Envelope_setTarget(&(p->voiced.envelope), vGain);
    Envelope_setTarget(&p->noiseEnv, nGain);
}

#if 0
static void VoicForm_quiet(VOICF *p)
{
    Envelope_keyOff(&(p->voiced.envelope));
    Envelope_setTarget(&p->noiseEnv, FL(0.0));
}

static void VoicForm_noteOff(VOICF *p)
{
    Envelope_keyOff(&p->voiced.envelope);
}

static void voicprint(CSOUND *csound, VOICF *p)
{
    SingWave_print(csound, &p->voiced);
    OneZero_print(csound, &p->onezero);
    OnePole_print(csound, &p->onepole);
}
#endif

static void make_FormSwep(FormSwep *p)
{
    p->poleCoeffs[0] = p->poleCoeffs[1] = FL(0.0);
    p->gain          = FL(1.0);
    p->freq          = p->reson         = FL(0.0);
    p->currentGain   = FL(1.0);
    p->currentFreq   = p->currentReson  = FL(0.0);
    p->targetGain    = FL(1.0);
    p->targetFreq    = p->targetReson   = FL(0.0);
    p->deltaGain     = FL(0.0);
    p->deltaFreq     = p->deltaReson    = FL(0.0);
    p->sweepState    = FL(0.0);
    p->sweepRate     = FL(0.002);
    p->dirty         = 0;
    p->outputs[0]    = p->outputs[1] = FL(0.0);
}

int32_t voicformset(CSOUND *csound, VOICF *p)
{
    MYFLT amp = (*p->amp)*AMP_RSCALE; /* Normalise */
    int32_t i;

    if (UNLIKELY(make_SingWave(csound, &p->voiced, p->ifn, p->ivfn)!=OK))
      return NOTOK;
    Envelope_setRate(csound, &(p->voiced.envelope), FL(0.001));
    Envelope_setTarget(&(p->voiced.envelope), FL(0.0));
    p->voiced.h = p->h;
    make_Noise(p->noise);

    for (i=0; i<4; i++) {
      make_FormSwep(&p->filters[i]);
      FormSwep_setSweepRate(p->filters[i], FL(0.001));
    }

    make_OneZero(&p->onezero);
    OneZero_setCoeff(&p->onezero, - FL(0.9));
    make_OnePole(&p->onepole);
    OnePole_setPole(&p->onepole, FL(0.9));

    make_Envelope(&p->noiseEnv);
    Envelope_setRate(csound, &p->noiseEnv, FL(0.001));
    Envelope_setTarget(&p->noiseEnv, FL(0.0));

    p->oldform = *p->formant;
    p->ph = (int32_t)(FL(0.5)+ *p->phoneme);
    VoicForm_setPhoneme(csound, p, p->ph, p->oldform);
                                /* Clear */
/*  OnePole_clear(&p->onepole); */ /* Included in make */
    FormSwep_clear(p->filters[0]);
    FormSwep_clear(p->filters[1]);
    FormSwep_clear(p->filters[2]);
    FormSwep_clear(p->filters[3]);
    {
      MYFLT temp, freq = *p->frequency;
      if ((freq * FL(22.0)) > CS_ESR)      {
        csound->Warning(csound, "%s", Str("This note is too high!!\n"));
        freq = CS_ESR / FL(22.0);
      }
      p->basef = freq;
      temp = FABS(FL(1500.0) - freq) + FL(200.0);
      p->lastGain = FL(10000.0) / temp / temp;
      SingWave_setFreq(csound, &p->voiced, freq);
    }

    Envelope_setTarget(&(p->voiced.envelope), amp);
    OnePole_setPole(&p->onepole, FL(0.95) - (amp * FL(0.2))/FL(128.0));
/*  voicprint(csound, p); */
    return OK;
}

int32_t voicform(CSOUND *csound, VOICF *p)
{
    MYFLT *ar = p->ar;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (p->basef != *p->frequency) {
      p->basef = *p->frequency;
      SingWave_setFreq(csound, &p->voiced, p->basef);
    }
/*  OnePole_setPole(&p->onepole, 0.95 - (amp * 0.1)); */
/*  Envelope_setTarget(&(p->voiced.envelope), amp); */
/*  Envelope_setTarget(&p->noiseEnv, 0.95 - (amp * 0.1)); */
    SingWave_setVibFreq(p->voiced, *p->vibf);
    Modulatr_setVibAmt(p->voiced.modulator, *p->vibAmt);
                                /* Set phoneme */
    if (p->oldform != *p->formant || p->ph != (int32_t)(0.5+*p->phoneme)) {
      p->oldform = *p->formant;
      p->ph = (int32_t)(0.5 + *p->phoneme);
      csound->Warning(csound, Str("Setting Phoneme: %d %f\n"),
                              p->ph, p->oldform);
      VoicForm_setPhoneme(csound, p, (int32_t) *p->phoneme, p->oldform);
    }
/*  voicprint(csound, p); */

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT temp;
      MYFLT lastOutput;
      temp   = OnePole_tick(&p->onepole,
                            OneZero_tick(&p->onezero,
                                         SingWave_tick(csound, &p->voiced)));
      //      printf("%d: temp=%f ", n, temp);
      temp  += Envelope_tick(&p->noiseEnv) * Noise_tick(csound, &p->noise);
      //      printf("%f\n", temp);
      lastOutput  = FormSwep_tick((OPDS *) p, &p->filters[0], temp);
      //      printf("%d: output=%f ", lastOutput);
      lastOutput  = FormSwep_tick((OPDS *) p, &p->filters[1], lastOutput);
      //      printf("%f ", lastOutput);
      lastOutput  = FormSwep_tick((OPDS *) p, &p->filters[2], lastOutput);
      //      printf("%f ", lastOutput);
      lastOutput  = FormSwep_tick((OPDS *) p, &p->filters[3], lastOutput);
      //      printf("%f ", lastOutput);
      lastOutput *= p->lastGain;
      //      printf("%f ", lastOutput);
      //      printf("->%f\n", lastOutput* AMP_SCALE);
      ar[n] = lastOutput * FL(0.22) * AMP_SCALE * *p->amp;
    }

    return OK;
}

