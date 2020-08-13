/*
    bus.h:

    Copyright (C) 2004 John ffitch
        (C) 2005, 2006 Istvan Varga
        (C) 2006 Victor Lazzarini

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

/*                                                      BUS.H           */

#ifndef CSOUND_BUS_H
#define CSOUND_BUS_H

#include "pstream.h"
#include "arrays.h"
#include "csound_standard_types.h"

#define MAX_CHAN_NAME 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    OPDS    h;
    MYFLT   *r, *a;
} CHNVAL;

typedef struct {
    OPDS    h;
    PVSDAT   *r;
    MYFLT    *a,*N, *overlap, *winsize, *wintype, *format;
    PVSDAT   init;
} FCHAN;

typedef struct {
    OPDS    h;
    MYFLT   *ans;
    MYFLT   *keyDown;
    int32_t evtbuf;
} KSENSE;

typedef struct channelEntry_s {
    struct channelEntry_s *nxt;
    controlChannelHints_t hints;
    MYFLT       *data;
    spin_lock_t lock;               /* Multi-thread protection */
    int32_t     type;
    int32_t     datasize;  /* size of allocated chn data */
    char        name[1];
} CHNENTRY;

typedef struct {
    OPDS        h;
    MYFLT       *arg;
    STRINGDAT   *iname;
    MYFLT       *fp;
    spin_lock_t *lock;
    int32_t     pos;
    char        chname[MAX_CHAN_NAME+1];
} CHNGET;

typedef struct {
    OPDS        h;
    ARRAYDAT    *arrayDat;
    STRINGDAT   *iname;
    MYFLT       *fp;
    spin_lock_t *lock;
    int32_t     pos;
    int32_t     arraySize;
    MYFLT**     channelPtrs;
    STRINGDAT   *channels;
    char        chname[MAX_CHAN_NAME+1];
} CHNGETARRAY;

typedef struct {
    OPDS    h;
    STRINGDAT   *iname[MAX_CHAN_NAME+1];
    MYFLT   *fp[MAX_CHAN_NAME+1];
    spin_lock_t *lock[MAX_CHAN_NAME+1];
} CHNCLEAR;

typedef struct {
    OPDS    h;
    STRINGDAT   *iname;
    MYFLT   *imode;
    MYFLT   *itype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
    MYFLT   *ix;
    MYFLT   *iy;
    MYFLT   *iwidth;
    MYFLT   *iheight;
    STRINGDAT *Sattributes;
    spin_lock_t      *lock;
} CHN_OPCODE_K;

typedef struct {
    OPDS    h;
    STRINGDAT   *iname;
    MYFLT   *imode;
    spin_lock_t   *lock;
} CHN_OPCODE;

typedef struct {
    OPDS    h;
    MYFLT   *arg;
    STRINGDAT   *iname;
    MYFLT   *imode;
    MYFLT   *itype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
} CHNEXPORT_OPCODE;

typedef struct {
    OPDS    h;
    MYFLT   *itype;
    MYFLT   *imode;
    MYFLT   *ictltype;
    MYFLT   *idflt;
    MYFLT   *imin;
    MYFLT   *imax;
    STRINGDAT   *iname;
} CHNPARAMS_OPCODE;

typedef struct {
    OPDS    h;
    MYFLT   *value, *valID;
    AUXCH   channelName;
    const CS_TYPE *channelType;
    void *channelptr;
} INVAL;

typedef struct {
    OPDS    h;
    MYFLT   *valID, *value;
    AUXCH   channelName;
    const CS_TYPE *channelType;
    void *channelptr;
} OUTVAL;

int32_t     chano_opcode_perf_k(CSOUND *, CHNVAL *);
int32_t     chano_opcode_perf_a(CSOUND *, CHNVAL *);
int32_t     chani_opcode_perf_k(CSOUND *, CHNVAL *);
int32_t     chani_opcode_perf_a(CSOUND *, CHNVAL *);
int32_t     pvsin_init(CSOUND *, FCHAN *);
int32_t     pvsin_perf(CSOUND *, FCHAN *);
int32_t     pvsout_init(CSOUND *, FCHAN *);
int32_t     pvsout_perf(CSOUND *, FCHAN *);

int32_t     sensekey_perf(CSOUND *, KSENSE *);

//Rory 2020
int32_t     chnget_array_opcode_init_i(CSOUND *, CHNGETARRAY *);
int32_t     chnget_array_opcode_init(CSOUND *, CHNGETARRAY *);
int32_t     chnget_array_opcode_perf_k(CSOUND *, CHNGETARRAY *);
int32_t     chnget_array_opcode_perf_a(CSOUND *, CHNGETARRAY *);
int32_t     chnget_array_opcode_perf_S(CSOUND* csound, CHNGETARRAY* p);
int32_t     chnset_array_opcode_init_i(CSOUND *, CHNGETARRAY *);
int32_t     chnset_array_opcode_init(CSOUND *, CHNGETARRAY *);
int32_t     chnset_array_opcode_perf_k(CSOUND *csound, CHNGETARRAY *p);
int32_t     chnset_array_opcode_perf_a(CSOUND *csound, CHNGETARRAY *p);
int32_t     chnset_array_opcode_perf_S(CSOUND *csound, CHNGETARRAY *p);

int32_t     notinit_opcode_stub(CSOUND *, void *);
int32_t     chnget_opcode_init_i(CSOUND *, CHNGET *);
int32_t     chnget_opcode_init_k(CSOUND *, CHNGET *);
int32_t     chnget_opcode_init_a(CSOUND *, CHNGET *);
int32_t     chnget_opcode_init_S(CSOUND *, CHNGET *);
int32_t     chnget_opcode_perf_S(CSOUND *, CHNGET *);
int32_t     chnset_opcode_init_i(CSOUND *, CHNGET *);
int32_t     chnset_opcode_init_k(CSOUND *, CHNGET *);
int32_t     chnset_opcode_init_a(CSOUND *, CHNGET *);
int32_t     chnset_opcode_init_S(CSOUND *, CHNGET *);
int32_t     chnset_opcode_perf_S(CSOUND *, CHNGET *);
int32_t     chnmix_opcode_init(CSOUND *, CHNGET *);
int32_t     chnclear_opcode_init(CSOUND *, CHNCLEAR *);
int32_t     chn_k_opcode_init(CSOUND *, CHN_OPCODE_K *);
int32_t     chn_k_opcode_init_S(CSOUND *, CHN_OPCODE_K *);
int32_t     chn_a_opcode_init(CSOUND *, CHN_OPCODE *);
int32_t     chn_S_opcode_init(CSOUND *, CHN_OPCODE *);
int32_t     chnexport_opcode_init(CSOUND *, CHNEXPORT_OPCODE *);
int32_t     chnparams_opcode_init(CSOUND *, CHNPARAMS_OPCODE *);

int32_t kinval(CSOUND *csound, INVAL *p);
int32_t kinvalS(CSOUND *csound, INVAL *p);
int32_t invalset(CSOUND *csound, INVAL *p);
int32_t invalset_string(CSOUND *csound, INVAL *p);
int32_t invalset_string_S(CSOUND *csound, INVAL *p);
int32_t invalset_S(CSOUND *csound, INVAL *p);
int32_t invalsetgo(CSOUND *csound, INVAL *p);
int32_t invalsetSgo(CSOUND *csound, INVAL *p);
int32_t koutval(CSOUND *csound, OUTVAL *p);
int32_t koutvalS(CSOUND *csound, OUTVAL *p);
int32_t outvalset(CSOUND *csound, OUTVAL *p);
int32_t outvalset_string(CSOUND *csound, OUTVAL *p);
int32_t outvalset_string_S(CSOUND *csound, OUTVAL *p);
int32_t outvalset_S(CSOUND *csound, OUTVAL *p);
int32_t outvalsetgo(CSOUND *csound, OUTVAL *p);
int32_t outvalsetSgo(CSOUND *csound, OUTVAL *p);
#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_BUS_H */
