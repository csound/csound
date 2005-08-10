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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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
#if defined(mac_classic)
#  pragma               pack(1)
#  define       PACKED
#endif

typedef struct {
        int num                 PACKED;
        sfSample *sample        PACKED;
        BYTE sampleModes        PACKED;
        BYTE minNoteRange       PACKED;
        BYTE maxNoteRange       PACKED;
        BYTE minVelRange        PACKED;
        BYTE maxVelRange        PACKED;
        long startOffset;
        long endOffset;
        long startLoopOffset;
        long endLoopOffset;
        char overridingRootKey  PACKED;
        char coarseTune         PACKED;
        char fineTune           PACKED;
        SHORT scaleTuning       PACKED;
        SHORT initialAttenuation        PACKED;
        SHORT pan               PACKED;
} splitType;

typedef struct {
        int num                 PACKED;
        char *name              PACKED;
        BYTE splits_num         PACKED;
        splitType *split        PACKED;
} instrType;

typedef struct {
        int num                 PACKED;
        char *name              PACKED;
        BYTE splits_num         PACKED;
        splitType *split        PACKED;
        BYTE minNoteRange       PACKED;
        BYTE maxNoteRange       PACKED;
        BYTE minVelRange        PACKED;
        BYTE maxVelRange        PACKED;
        char coarseTune         PACKED;
        char fineTune           PACKED;
        SHORT scaleTuning       PACKED;
        SHORT initialAttenuation        PACKED;
        SHORT pan               PACKED;
} layerType;

typedef struct {
        char *name              PACKED;
        int num                 PACKED;
        WORD prog               PACKED;
        WORD bank               PACKED;
        int layers_num          PACKED;
        layerType *layer        PACKED;
} presetType;

typedef struct {
  BYTE  ckID[4] PACKED; /*  A chunk ID identifies the type of data within the chunk. */
  DWORD ckSize  PACKED; /*  The size of the chunk data in bytes, excluding any pad byte. */
  BYTE  *ckDATA PACKED; /*  The actual data plus a pad byte if req’d to word align. */
} CHUNK;

typedef struct {
        CHUNK main_chunk        PACKED;
        CHUNK *phdrChunk, *pbagChunk, *pmodChunk, *pgenChunk, *instChunk,
                  *ibagChunk, *imodChunk, *igenChunk, *shdrChunk, *smplChunk    PACKED;
        sfPresetHeader *phdr    PACKED;
        sfInst *inst            PACKED;
        sfSample *shdr          PACKED;
        sfPresetBag *pbag       PACKED;
        sfModList *pmod         PACKED;
        sfGenList *pgen         PACKED;
        sfInstBag *ibag         PACKED;
        sfInstModList *imod     PACKED;
        sfInstGenList *igen     PACKED;
} CHUNKS;

typedef struct {
        char name[256]          PACKED;
        int presets_num         PACKED;
        presetType *preset      PACKED;
        int instrs_num          PACKED;
        instrType *instr        PACKED;
        SHORT *sampleData       PACKED;
        CHUNKS chunk            PACKED;
} SFBANK;

#ifdef          MSVC
#  pragma       pack(pop, before)
#endif
#define _SF_H
#endif
