/*
    sf.h:

    Copyright (C) 2000 Gabriel Maldonado, John ffitch

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
 
#if !defined(_SF_H)
#include "sftype.h"
#ifdef          __GNUC__
#  ifndef       PACKED
#    define     PACKED  __attribute__((packed))
#  endif        /* PACKED */
#else
#  define       PACKED
#endif
#ifdef          MSVC
#  pragma       pack(push, before, 1)
#endif

struct _splitType {
        int32_t num;
        sfSample *sample;
        BYTE sampleModes;
        BYTE minNoteRange;
        BYTE maxNoteRange;
        BYTE minVelRange;
        BYTE maxVelRange;
        int64_t startOffset;
        int64_t endOffset;
        int64_t startLoopOffset;
        int64_t endLoopOffset;
        /*char*/ SBYTE overridingRootKey;
        /*char*/ SBYTE coarseTune;
        /*char*/ SBYTE fineTune;
        SHORT scaleTuning;
        SHORT initialAttenuation;
        SHORT pan;
        MYFLT attack;
        MYFLT decay;
        MYFLT sustain;
        MYFLT release;
} PACKED;
typedef struct _splitType splitType;

struct _instrType {
        int32_t num;
        char *name;
        BYTE splits_num;
        splitType *split;
} PACKED;
typedef struct _instrType instrType;

struct _layerType {
        int32_t num;
        char *name;
        BYTE splits_num;
        splitType *split;
        BYTE minNoteRange;
        BYTE maxNoteRange;
        BYTE minVelRange;
        BYTE maxVelRange;
        SBYTE coarseTune;
        SBYTE fineTune;
        SHORT scaleTuning;
        SHORT initialAttenuation;
        SHORT pan;
} PACKED;
typedef struct _layerType layerType;

struct _presetType {
        char *name;
        int32_t num;
        WORD prog;
        WORD bank;
        int32_t layers_num;
        layerType *layer;
} PACKED;
typedef struct _presetType presetType;

struct _CHUNK {
  BYTE  ckID[4]; /* A chunk ID identifies the type of data within the chunk. */
  DWORD ckSize;  /* The size of the chunk data in bytes, excluding any pad byte. */
  BYTE  *ckDATA; /* The actual data plus a pad byte if req’d to word align. */
} PACKED;
typedef struct _CHUNK CHUNK;

struct _CHUNKS {
        CHUNK main_chunk;
        CHUNK *phdrChunk, *pbagChunk, *pmodChunk, *pgenChunk, *instChunk,
              *ibagChunk, *imodChunk, *igenChunk, *shdrChunk, *smplChunk;
        sfPresetHeader *phdr;
        sfInst *inst;
        sfSample *shdr;
        sfPresetBag *pbag;
        sfModList *pmod;
        sfGenList *pgen;
        sfInstBag *ibag;
        sfInstModList *imod;
        sfInstGenList *igen;
} PACKED;
typedef struct _CHUNKS CHUNKS;

struct _SFBANK {
        char name[256];
        int32_t presets_num;
        presetType *preset;
        int32_t instrs_num;
        instrType *instr;
        SHORT *sampleData;
        CHUNKS chunk;
} PACKED;
typedef struct _SFBANK SFBANK;

#ifdef          MSVC
#  pragma       pack(pop, before)
#endif
#define _SF_H
#endif
