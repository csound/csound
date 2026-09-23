/*  Copyright (C) 2002-2004 Gabriel Maldonado

  The gab library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The gab library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the gab library; if not, write to the Free Software
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

  Ported to csound5 by:Andres Cabrera andres@geminiflux.com
*/

#ifndef GAB_H
#define GAB_H

#include "../stdopcod.h"

typedef struct {
    OPDS    h;
    cs_float   *ar, *asig, *kcf, *kbw, *ord, *iscl, *istor;
    int32_t scale, loop;
    cs_float   c1, c2, c3, *yt1, *yt2, cosf, prvcf, prvbw;
    AUXCH   aux;
} KRESONX;

typedef struct {
    OPDS    h;
    cs_float   *rslt, *xndx, *xfn, *ixmode;
    cs_float   *table;
    cs_float   xbmul;
    int32_t xmode;
    int32_t tablen;
} FASTAB;

typedef struct {
    OPDS    h;
    cs_float   *r, *ndx;
    cs_float   **tb_ptr;
} FASTB;

typedef struct {
    OPDS    h;
    cs_float   *ifn;
} TB_INIT;

/* ====================== */
/* opcodes from Jens Groh */
/* ====================== */
typedef struct {        /* for nlalp opcode */
    OPDS    h;          /* header */
    cs_float   *aresult;   /* resulting signal */
    cs_float   *ainsig;    /* input signal */
    cs_float   *klfact;    /* linear factor */
    cs_float   *knfact;    /* nonlinear factor */
    cs_float   *istor;     /* initial storage disposition */
    cs_double  m0;         /* energy storage */
    cs_double  m1;         /* energy storage */
} NLALP;

/* end opcodes from Jens Groh */

typedef struct {
    OPDS    h;
    cs_float   *sr, *kamp, *kcps, *ifn, *ifreqtbl, *iamptbl, *icnt, *iphs, *interp;
    FUNC    *ftp;
    FUNC    *freqtp;
    FUNC    *amptp;
    int32_t count;
    int32_t inerr;
    AUXCH   lphs;
    AUXCH   pamp;
  int32_t floatph;
} ADSYNT2;

typedef struct {
    OPDS    h;
    cs_float   *retval;
} EXITNOW;

typedef struct {
    OPDS    h;
    cs_float   *ktrig_start, *ktrig_stop, *numtics, *kfn, *inargs[VARGMAX];
    int32_t recording, numins;
    cs_double currtic;
    int64_t ndx, tablen;
    cs_float   *table;
} TABREC;

typedef struct {
    OPDS    h;
    cs_float   *ktrig, *numtics, *kfn, *outargs[VARGMAX];
    int32_t playing, numouts;
    cs_double currtic;
    int64_t ndx, tablen;
    cs_float   *table, old_fn;
} TABPLAY;

typedef struct {
    OPDS    h;
    cs_float   *ktrig, *inargs[VARGMAX];
    int32_t numargs;            /* Reordered for caching */
    int32_t cnt;
    cs_float   old_inargs[VARGMAX];
} ISCHANGED;

typedef struct {
    OPDS     h;
    cs_float    *ktrig;
    ARRAYDAT *chk;
    size_t   size;
    int32_t  cnt;
    AUXCH    old_chk;
} ISACHANGED;

typedef struct {
    OPDS    h;
    cs_float   *commandLine;
} CSSYSTEM;

typedef struct {
    OPDS    h;
    cs_float   *kout, *asig, *ktrig, *imaxflag;
    cs_float   max;
    uint64_t    counter;
} P_MAXIMUM;

/* From fractals.h */
typedef struct {
    OPDS    h;
    cs_float   *kr, *koutrig,  *ktrig, *kx, *ky, *kmaxIter;
    cs_float   oldx, oldy, oldMaxIter;
    int32_t oldCount;
} MANDEL;

#endif

