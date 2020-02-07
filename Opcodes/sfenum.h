/*
    sfenum.h:

    Copyright (C) 1995-1998 Creative Technology Ltd. / E-mu Systems, Inc.

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

/* //////////////////////////////////////////////////////////////////////////
//                                                                         //
//   SFENUM.H                                                              //
//                                                                         //
//   Description : Header file for acccesing the SoundFont Edit Engine     //
//                                                                         //
//   Copyright (c) Creative Technology Ltd. / E-mu Systems, Inc.           //
//                 1995-1998. All rights reserved.                         //
//                                                                         //
//   Revision:     1.00                                                    //
//                                                                         //
////////////////////////////////////////////////////////////////////////// */

#ifndef _SFENUM_H
#define _SFENUM_H

typedef enum
{
  /* Oscillator */
  startAddrsOffset,       /*sample start address -4 (0 to 0xffffff)   0 */
  endAddrsOffset,
  startloopAddrsOffset,   /*loop start address -4 (0 to 0xffffff) */
  endloopAddrsOffset,     /*loop end address -3 (0 to 0xffffff) */

  /* Pitch */
  startAddrsCoarseOffset, /*CHANGED FOR SF2 */
  modLfoToPitch,          /*main fm: lfo1-> pitch                     5 */
  vibLfoToPitch,          /*aux fm:  lfo2-> pitch */
  modEnvToPitch,          /*pitch env: env1(aux)-> pitch */

  /* Filter */
  initialFilterFc,        /*initial filter cutoff */
  initialFilterQ,         /*filter Q */
  modLfoToFilterFc,       /*filter modulation: lfo1 -> filter cutoff  10 */
  modEnvToFilterFc,       /*filter env: env1(aux)-> filter cutoff */

  /* Amplifier */
  endAddrsCoarseOffset,   /*CHANGED FOR SF2 */
  modLfoToVolume,         /*tremolo: lfo1-> volume */
    unused1,

  /* Effects */
  chorusEffectsSend,      /*chorus                                    15 */
  reverbEffectsSend,      /*reverb */
    pan,
    unused2,
    unused3,
  unused4,                /*                                          20 */

  /* Main lfo1 */
  delayModLFO,            /*delay 0x8000-n*(725us) */
  freqModLFO,             /*frequency */

  /* Aux lfo2 */
  delayVibLFO,            /*delay 0x8000-n*(725us) */
  freqVibLFO,             /*frequency */

  /* Env1(aux/value) */
  delayModEnv,            /*delay 0x8000 - n(725us)                   25 */
  attackModEnv,           /*attack */
  holdModEnv,             /*hold */
  decayModEnv,            /*decay */
  sustainModEnv,          /*sustain */
  releaseModEnv,          /*release                                   30 */
    keynumToModEnvHold,
    keynumToModEnvDecay,

  /* Env2(ampl/vol) */
  delayVolEnv,            /*delay 0x8000 - n(725us) */
  attackVolEnv,           /*attack */
  holdVolEnv,             /*hold                                      35 */
  decayVolEnv,            /*decay */
  sustainVolEnv,          /*sustain */
  releaseVolEnv,          /*release */
    keynumToVolEnvHold,
  keynumToVolEnvDecay,    /*                                          40 */

  /* Preset */
    instrument,
    reserved1,
    keyRange,
    velRange,
  startloopAddrCoarseOffset, /*CHANGED FOR SF2                       45 */
    keynum,
    velocity,
  initialAttenuation,            /*CHANGED FOR SF2 */
    reserved2,
  endloopAddrsCoarseOffset,   /*CHANGED FOR SF2                       50 */
    coarseTune,
    fineTune,
    sampleID,
  sampleModes,                /*CHANGED FOR SF2 */
  reserved3,                  /*                                      55 */
    scaleTuning,
    exclusiveClass,
    overridingRootKey,
    unused5,
  endOper                     /*                                      60 */
} SFGenerator;

typedef enum
{
    /* Start of MIDI modulation operators */
    cc1_Mod,
    cc7_Vol,
    cc10_Pan,
    cc64_Sustain,
    cc91_Reverb,
    cc93_Chorus,

    ccPitchBend,
    ccIndirectModX,
    ccIndirectModY,

    endMod
} SFModulator;

#ifdef never
#define monoSample      0x0001
#define rightSample     0x0002
#define leftSample      0x0004
#define linkedSample    0x0008

#define ROMSample       0x8000          /* 32768 */
#define ROMMonoSample   0x8001          /* 32769 */
#define ROMRightSample  0x8002          /* 32770 */
#define ROMLeftSample   0x8004          /* 32772 */
#define ROMLinkedSample 0x8008          /* 32776 */
#endif

enum scaleTuning
{
    equalTemp,
    fiftyCents
};

enum SFSampleField          /*used by Sample Read Module */
{
    NAME_FIELD = 1,
    START_FIELD,
    END_FIELD,
    START_LOOP_FIELD,
    END_LOOP_FIELD,
    SMPL_RATE_FIELD,
    ORG_KEY_FIELD,
    CORRECTION_FIELD,
    SMPL_LINK_FIELD,
    SMPL_TYPE_FIELD
};

enum SFInfoChunkField       /*used by Bank Read Module */
{
    IFIL_FIELD = 1,
    IROM_FIELD,
    IVER_FIELD,
    ISNG_FIELD,
    INAM_FIELD,
    IPRD_FIELD,
    IENG_FIELD,
    ISFT_FIELD,
    ICRD_FIELD,
    ICMT_FIELD,
    ICOP_FIELD
};

#endif /*_SFENUM_H */
