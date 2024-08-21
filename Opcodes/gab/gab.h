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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

  Ported to csound5 by:Andres Cabrera andres@geminiflux.com
*/

#ifndef GAB_H
#define GAB_H

#include "../stdopcod.h"

typedef struct {
    OPDS    h;
    MYFLT   *ar, *asig, *kcf, *kbw, *ord, *iscl, *istor;
    int32_t scale, loop;
    MYFLT   c1, c2, c3, *yt1, *yt2, cosf, prvcf, prvbw;
    AUXCH   aux;
} KRESONX;

typedef struct {
    OPDS    h;
    MYFLT   *rslt, *xndx, *xfn, *ixmode;
    MYFLT   *table;
    MYFLT   xbmul;
    int32_t xmode;
    int32_t tablen;
} FASTAB;

typedef struct {
    OPDS    h;
    MYFLT   *r, *ndx;
    MYFLT   **tb_ptr;
} FASTB;

typedef struct {
    OPDS    h;
    MYFLT   *ifn;
} TB_INIT;

/* ====================== */
/* opcodes from Jens Groh */
/* ====================== */
typedef struct {        /* for nlalp opcode */
    OPDS    h;          /* header */
    MYFLT   *aresult;   /* resulting signal */
    MYFLT   *ainsig;    /* input signal */
    MYFLT   *klfact;    /* linear factor */
    MYFLT   *knfact;    /* nonlinear factor */
    MYFLT   *istor;     /* initial storage disposition */
    double  m0;         /* energy storage */
    double  m1;         /* energy storage */
} NLALP;

/* end opcodes from Jens Groh */

typedef struct {
    OPDS    h;
    MYFLT   *sr, *kamp, *kcps, *ifn, *ifreqtbl, *iamptbl, *icnt, *iphs;
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
    MYFLT   *retval;
} EXITNOW;

typedef struct {
    OPDS    h;
    MYFLT   *ktrig_start, *ktrig_stop, *numtics, *kfn, *inargs[VARGMAX];
    int32_t recording, numins;
    int64_t currtic, ndx, tablen;
    MYFLT   *table, old_fn;
} TABREC;

typedef struct {
    OPDS    h;
    MYFLT   *ktrig, *numtics, *kfn, *outargs[VARGMAX];
    int32_t playing, numouts;
    int64_t currtic, ndx, tablen;
    MYFLT   *table, old_fn;
} TABPLAY;

typedef struct {
    OPDS    h;
    MYFLT   *ktrig, *inargs[VARGMAX];
    int32_t numargs;            /* Reordered for caching */
    int32_t cnt;
    MYFLT   old_inargs[VARGMAX];
} ISCHANGED;

typedef struct {
    OPDS     h;
    MYFLT    *ktrig;
    ARRAYDAT *chk;
    int32_t  size;
    int32_t  cnt;
    AUXCH    old_chk;
} ISACHANGED;

typedef struct {
    OPDS    h;
    MYFLT   *commandLine;
} CSSYSTEM;

typedef struct {
    OPDS    h;
    MYFLT   *kout, *asig, *ktrig, *imaxflag;
    MYFLT   max;
    int32_t     counter;
} P_MAXIMUM;

/* From fractals.h */
typedef struct {
    OPDS    h;
    MYFLT   *kr, *koutrig,  *ktrig, *kx, *ky, *kmaxIter;
    MYFLT   oldx, oldy;
    int32_t oldCount;
} MANDEL;

#endif

