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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*                                                      BUS.H           */

#ifndef CSOUND_BUS_H
#define CSOUND_BUS_H

#include "pstream.h"
#include "csound_standard_types.h"

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
    int     evtbuf;
} KSENSE;

typedef struct channelEntry_s {
    struct channelEntry_s *nxt;
    controlChannelHints_t hints;
    MYFLT   *data;
#ifndef MACOSX
#if defined(HAVE_PTHREAD_SPIN_LOCK)
    pthread_spinlock_t *lock;
    pthread_spinlock_t theLock;
#else
    int     lock;
#endif
#else
    int     lock;               /* Multi-thread protection */
#endif
    int     type;
    int     datasize;  /* size of allocated chn data */
    char    name[1];
} CHNENTRY;

typedef struct {
    OPDS    h;
    MYFLT   *arg;
    STRINGDAT   *iname;
    MYFLT   *fp;
    int     *lock;
    int      pos;
} CHNGET;

typedef struct {
    OPDS    h;
    STRINGDAT   *iname;
    MYFLT   *fp;
    int     *lock;
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
    int     *lock;
} CHN_OPCODE_K;

typedef struct {
    OPDS    h;
    STRINGDAT   *iname;
    MYFLT   *imode;
    int     *lock;
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

int     chano_opcode_perf_k(CSOUND *, CHNVAL *);
int     chano_opcode_perf_a(CSOUND *, CHNVAL *);
int     chani_opcode_perf_k(CSOUND *, CHNVAL *);
int     chani_opcode_perf_a(CSOUND *, CHNVAL *);
int     pvsin_init(CSOUND *, FCHAN *);
int     pvsin_perf(CSOUND *, FCHAN *);
int     pvsout_init(CSOUND *, FCHAN *);
int     pvsout_perf(CSOUND *, FCHAN *);

int     sensekey_perf(CSOUND *, KSENSE *);

int     notinit_opcode_stub(CSOUND *, void *);
int     chnget_opcode_init_i(CSOUND *, CHNGET *);
int     chnget_opcode_init_k(CSOUND *, CHNGET *);
int     chnget_opcode_init_a(CSOUND *, CHNGET *);
int     chnget_opcode_init_S(CSOUND *, CHNGET *);
int     chnget_opcode_perf_S(CSOUND *, CHNGET *);
int     chnset_opcode_init_i(CSOUND *, CHNGET *);
int     chnset_opcode_init_k(CSOUND *, CHNGET *);
int     chnset_opcode_init_a(CSOUND *, CHNGET *);
int     chnset_opcode_init_S(CSOUND *, CHNGET *);
int     chnset_opcode_perf_S(CSOUND *, CHNGET *);
int     chnmix_opcode_init(CSOUND *, CHNGET *);
int     chnclear_opcode_init(CSOUND *, CHNCLEAR *);
int     chn_k_opcode_init(CSOUND *, CHN_OPCODE_K *);
int     chn_a_opcode_init(CSOUND *, CHN_OPCODE *);
int     chn_S_opcode_init(CSOUND *, CHN_OPCODE *);
int     chnexport_opcode_init(CSOUND *, CHNEXPORT_OPCODE *);
int     chnparams_opcode_init(CSOUND *, CHNPARAMS_OPCODE *);

int kinval(CSOUND *csound, INVAL *p);
int kinvalS(CSOUND *csound, INVAL *p);
int invalset(CSOUND *csound, INVAL *p);
int invalset_string(CSOUND *csound, INVAL *p);
int invalset_string_S(CSOUND *csound, INVAL *p);
int invalset_S(CSOUND *csound, INVAL *p);
int koutval(CSOUND *csound, OUTVAL *p);
int koutvalS(CSOUND *csound, OUTVAL *p);
int outvalset(CSOUND *csound, OUTVAL *p);
int outvalset_string(CSOUND *csound, OUTVAL *p);
int outvalset_string_S(CSOUND *csound, OUTVAL *p);
int outvalset_S(CSOUND *csound, OUTVAL *p);
#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_BUS_H */
