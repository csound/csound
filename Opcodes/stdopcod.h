/*
    stdopcod.h:

    Copyright (c) 205 Istvan Varga

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

#ifndef CSOUND_STDOPCOD_H
#define CSOUND_STDOPCOD_H

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif


#include "interlocks.h"

/* file structure for fout opcodes */

struct fileinTag {
    void     *file;        /* Used in audio cases */
    FILE        *raw;         /* Only used if text file */
    void        *fd;          /* file handle returned by CSOUND::FileOpen */
    char        *name;        /* short name */
    int32_t         do_scale;     /* non-zero if 0dBFS scaling should be applied */
    uint32      refCount;   /* reference count, | 0x80000000 if close reqd */
};

typedef struct VCO2_TABLE_ARRAY_  VCO2_TABLE_ARRAY;
typedef struct _atsbufread        ATSBUFREAD;

typedef struct STDOPCOD_GLOBALS_ {
    CSOUND      *csound;
    /* fout.c */
    struct fileinTag  *file_opened;
    int32_t         file_max;
    int32_t         file_num;
    int64_t        fout_kreset;
   /* MYFLT       *buf;
      int32_t         buf_size; */ /* VL - now using per instance buffer */
    /* oscbnk.c */
    uint32      oscbnk_seed;
    int32       rnd31i_seed;
    int32_t         denorm_seed;
    int32_t         vco2_nr_table_arrays;
    VCO2_TABLE_ARRAY  **vco2_tables;
    /* ugnorman.c */
    ATSBUFREAD  *atsbufreadaddr;
    int32_t         swapped_warning;
    /* locsig.c */
    void        *locsigaddr;
    /* space.c */
    void        *spaceaddr;
    /* gab/gab.c */
    MYFLT       *tb_ptrs[16];       /* Left here while the rest is implemented */
    MYFLT       *tb[16];       /* gab: updated */
    int32_t         tb_ixmode[16]; /* gab: added */
    int32       tb_size[16];   /* gab: added */
  //OPARMS  oparms;
} STDOPCOD_GLOBALS;

extern int32_t ambicode_init_(CSOUND *);
extern int32_t bbcut_init_(CSOUND *);
extern int32_t biquad_init_(CSOUND *);
extern int32_t butter_init_(CSOUND *);
extern int32_t clfilt_init_(CSOUND *);
extern int32_t cross2_init_(CSOUND *);
extern int32_t dam_init_(CSOUND *);
extern int32_t dcblockr_init_(CSOUND *);
extern int32_t filter_init_(CSOUND *);
extern int32_t flanger_init_(CSOUND *);
extern int32_t follow_init_(CSOUND *);
extern int32_t fout_init_(CSOUND *);
extern int32_t freeverb_init_(CSOUND *);
extern int32_t ftconv_init_(CSOUND *);
extern int32_t ftgen_init_(CSOUND *);
extern int32_t gab_gab_init_(CSOUND *);
extern int32_t gab_vectorial_init_(CSOUND *);
extern int32_t grain_init_(CSOUND *);
extern int32_t locsig_init_(CSOUND *);
extern int32_t lowpassr_init_(CSOUND *);
extern int32_t metro_init_(CSOUND *);
extern int32_t newfils_init_(CSOUND *);
extern int32_t nlfilt_init_(CSOUND *);
extern int32_t oscbnk_init_(CSOUND *);
extern int32_t pluck_init_(CSOUND *);
extern int32_t repluck_init_(CSOUND *);
extern int32_t reverbsc_init_(CSOUND *);
extern int32_t seqtime_init_(CSOUND *);
extern int32_t sndloop_init_(CSOUND *);
extern int32_t sndwarp_init_(CSOUND *);
extern int32_t space_init_(CSOUND *);
extern int32_t spat3d_init_(CSOUND *);
extern int32_t syncgrain_init_(CSOUND *);
extern int32_t ugens7_init_(CSOUND *);
extern int32_t ugens9_init_(CSOUND *);
extern int32_t ugensa_init_(CSOUND *);
extern int32_t uggab_init_(CSOUND *);
extern int32_t ugmoss_init_(CSOUND *);
extern int32_t ugnorman_init_(CSOUND *);
extern int32_t ugsc_init_(CSOUND *);
extern int32_t
wave_terrain_init_(CSOUND *);
extern int32_t
wter2_init_(CSOUND *);


#endif  /* CSOUND_STDOPCOD_H */

