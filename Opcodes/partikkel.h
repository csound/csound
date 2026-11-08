/*
Partikkel - a granular synthesis module for Csound 5
Copyright (C) 2006-2009 Øyvind Brandtsegg, Torgeir Strand Henriksen,
Thom Johansen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

typedef struct {
    FUNC *table;
    cs_double phase, delta;
    cs_double sweepoffset, sweepdecay;
    cs_float gain;
} WAVEDATA;

typedef struct {
    uint32_t start, stop;
    cs_double envphase, envinc;
    cs_double envattacklen, envdecaystart;
    cs_double env2amount;
    cs_float fmamp;
    FUNC *fmenvtab;
    uint32_t harmonics;
    cs_float falloff, falloff_pow_N;
    cs_float gain1, gain2;
    uint32_t chan1, chan2;
    WAVEDATA wav[5];
} GRAIN;

/* which of the wav[] entries above correspond to the trainlet generator */
#define WAV_TRAINLET 4

/* support structs for the grain pool routines */
typedef struct NODE {
    GRAIN grain;
    struct NODE *next;
} NODE;

typedef struct {
    NODE *grainlist;
    char *mempool;
    uint32_t free_nodes;
} GRAINPOOL;

struct PARTIKKEL;

typedef struct PARTIKKEL_GLOBALS_ENTRY {
    cs_float id;
    cs_float *synctab;
    struct PARTIKKEL *partikkel;
    struct PARTIKKEL_GLOBALS_ENTRY *next;
} PARTIKKEL_GLOBALS_ENTRY;

typedef struct {
    /* default tables. name describes table contents, 'z' is 0.0, 'o' is 1.0
     * and 'h' is 0.5 */
    FUNC *ooo_tab;
    FUNC *zzz_tab;
    FUNC *zzo_tab;
    FUNC *zzhhhhz_tab;
  //char *tablestorage;
    PARTIKKEL_GLOBALS_ENTRY *rootentry;
} PARTIKKEL_GLOBALS;

typedef struct PARTIKKEL {
    OPDS h;
    /* output arrays */
    cs_float *output1, *output2, *output3, *output4;
    cs_float *output5, *output6, *output7, *output8;

    /* opcode parameters */
    cs_float *grainfreq;
    cs_float *distribution;
    cs_float *dist;
    cs_float *sync;
    cs_float *env2_amount;
    cs_float *env2;
    cs_float *env_attack;
    cs_float *env_decay;
    cs_float *sustain_amount;
    cs_float *a_d_ratio;
    cs_float *duration;
    cs_float *amplitude;
    cs_float *gainmasks;
    cs_float *wavfreq;
    cs_float *freqsweepshape;
    cs_float *wavfreq_startmuls;
    cs_float *wavfreq_endmuls;
    cs_float *fm;
    cs_float *fm_indices;
    cs_float *fm_env;
    cs_float *cosine;
    cs_float *trainletfreq;
    cs_float *harmonics;
    cs_float *falloff;
    cs_float *channelmasks;
    cs_float *randommask;
    cs_float *waveform1, *waveform2, *waveform3, *waveform4;
    cs_float *waveamps;
    cs_float *samplepos1, *samplepos2, *samplepos3, *samplepos4;
    cs_float *wavekey1, *wavekey2, *wavekey3, *wavekey4;
    cs_float *max_grains;
    cs_float *opcodeid;
    cs_float *pantable;

    /* internal variables */
    PARTIKKEL_GLOBALS *globals;
    PARTIKKEL_GLOBALS_ENTRY *globals_entry;
    GRAINPOOL gpool;
    NODE *grainroot;
    int32_t out_of_voices_warning;
    uint32_t num_outputs;
    int32_t grainfreq_arate;
    int32_t synced;
    AUXCH aux, aux2;
    CsoundRandMTState randstate;
    FUNC *wavetabs[4];
    FUNC *costab;
    uint32_t cosineshift;
    cs_float zscale;
    FUNC *disttab;
    uint32_t distindex;
    uint32_t disttabshift;
    FUNC *env2_tab, *env_attack_tab, *env_decay_tab;
    FUNC *fmenvtab;
    FUNC *gainmasktab;
    uint32_t gainmaskindex;
    FUNC *wavfreqstarttab, *wavfreqendtab;
    uint32_t wavfreqstartindex, wavfreqendindex;
    FUNC *fmamptab;
    uint32_t fmampindex;
    FUNC *channelmasktab;
    uint32_t channelmaskindex;
    FUNC *wavgaintab;
    uint32_t wavgainindex;
    cs_double grainphase, graininc;
    FUNC *pantab;
    int32_t floatph;
} PARTIKKEL;

typedef struct {
    OPDS h;
    /* output arrays */
    cs_float *syncout;
    cs_float *schedphaseout;

    /* opcode parameters */
    cs_float *opcodeid;

    /* internal variables */
    int32_t
    output_schedphase;
    PARTIKKEL_GLOBALS_ENTRY *ge;
} PARTIKKEL_SYNC;

typedef struct {
    OPDS h;
    /* output */
    cs_float *valout;

    /* inputs */
    cs_float *index;
    cs_float *opcodeid;
    PARTIKKEL_GLOBALS_ENTRY *ge;
} PARTIKKEL_GET;

typedef struct {
    OPDS h;
    /* inputs */
    cs_float *index;
    cs_float *value;
    cs_float *opcodeid;
    PARTIKKEL_GLOBALS_ENTRY *ge;
} PARTIKKEL_SET;

