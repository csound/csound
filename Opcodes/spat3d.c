/*
    spat3d.c:

    Copyright (C) 2001 Istvan Varga

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

/* ----- spat3d, spat3di, and spat3dt -- written by Istvan Varga, 2001 ----- */

#include "stdopcod.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define CSOUND_SPAT3D_C 1

#include "spat3d.h"

/* ----------------------- initialisation functions ------------------------ */

/* initialise FIR filter for downsampling */

static int32_t    spat3d_init_window(CSOUND *csound, SPAT3D *p)
{
    int32_t i, j, o;
    double  d, w;

    o = p->oversamp << 5;           /* window size = 32 * oversample */

    i = ((o + 1) * (sizeof(int32_t) + sizeof(MYFLT)));      /* allocate */
    if ((p->fltr.auxp == NULL) || (p->fltr.size < (uint32_t)i)) /* space */
      csound->AuxAlloc(csound, i, &(p->fltr));
    p->sample = (int32_t *) p->fltr.auxp;             /* sample number */
    p->window = (MYFLT *) (p->sample + o + 1);        /* window value  */

    for (i = -(o >> 1), j = 0; i < (o >> 1); i++) {
      if (i == 0) {
        d = 1.0;
      }
      else {
        w = cos(PI * (d = (double) i) / (double) o);
        d *= PI / (double) (p->oversamp);
        d = w * w * sin(d) / d;
      }
      if (fabs(d) > 0.00000001) {       /* skip zero samples */
        p->window[j] = (MYFLT) d;       /* window value  */
        p->sample[j++] = i;             /* sample number */
      }
    }
    p->sample[j] = -10000;              /* end of window */
    return OK;
}

/* initialise parameric equalizer (code taken from pareq opcode) */

static int32_t spat3d_init_eq(SPAT3D *p, SPAT3D_WALL *wstruct, MYFLT *ftable)
{
    int32_t eqmode;
    double  omega, k, kk, vk, vkk, vkdq, sq, a0, a1, a2, b0, b1, b2;

    /* EQ code taken from biquad.c */

    eqmode = (int32_t) ((double) ftable[3] + 0.5);              /* mode      */
    omega = (double) ftable[0] * (double) CS_TPIDSR;       /* frequency */
    sq = sqrt(2.0 * (double) ftable[1]);                        /* level     */

    k = tan((eqmode > 1 ? (PI - omega) : omega) * 0.5); kk = k * k;
    vk = (double) ftable[1] * k; vkk = (double) ftable[1] * kk;
    vkdq = vk / (double) ftable[2];                             /* Q         */

    if (eqmode >= 1) {
      b0 = 1.0 + sq * k + vkk;
      b1 = 2.0 * (vkk - 1.0);
      b2 = 1.0 - sq * k + vkk;
    }
    else {
      b0 = 1.0 + vkdq + kk;
      b1 = 2.0 * (kk - 1.0);
      b2 = 1.0 - vkdq + kk;
    }
    a0 = 1.0 + (k / (double) ftable[2]) + kk;
    a1 = 2.0 * (kk - 1.0);
    a2 = 1.0 - (k / (double) ftable[2]) + kk;
    if (eqmode > 1) {
      a1 = -a1;
      b1 = -b1;
    }
    a0 = 1.0 / a0;
    wstruct->a1 = (MYFLT) (a0 * a1); wstruct->a2 = (MYFLT) (a0 * a2);
    wstruct->b0 = (MYFLT) (a0 * b0);
    wstruct->b1 = (MYFLT) (a0 * b1); wstruct->b2 = (MYFLT) (a0 * b2);
    return OK;
}

/* initialise wall structures */

static SPAT3D_WALL*
spat3d_init_wall(SPAT3D *p,             /* opcode struct                    */
                 int32_t    wallno,     /* wall number                      */
                 int32_t    dep,        /* recursion depth                  */
                 int32_t   *wmax,       /* wall structure number            */
                 MYFLT  X, MYFLT Y, MYFLT Z) /* coordinates (spat3di/spat3dt) */
{
    int32_t             i;
    SPAT3D_WALL     *ws;
    MYFLT           *ft, a, d, w, x, y, z;
    double          d0, d1;

    /* update random seed */

    p->rseed = (15625L * p->rseed + 1L) & 0xFFFFL;

    /* select wall structure and output buffer */

    ws = (SPAT3D_WALL *) p->ws.auxp + (*wmax)++;
    ws->yn = (MYFLT *) p->y.auxp + (p->bs * dep++);

    /* ftable */

    if ((wallno == 0) || (p->ftable == NULL)) {
      ft = NULL;
    }
    else {
      ft = p->ftable + (wallno << 3) - 2;
    }

    /* initialise wall structure */

    for (i = 0; i < 6; i++) ws->nextRefl[i] = NULL;
    ws->xnm1 = ws->xnm2 = ws->ynm1 = ws->ynm2 = FL(0.0);
    ws->a1 = ws->b1 = ws->a2 = ws->b2 = FL(0.0);
    ws->b0 = FL(1.0);
    ws->Xc = ws->W0 = ws->X0 = ws->Y0 = ws->Z0 = FL(0.0);
    ws->D0 = ws->D1 = 0.0;          /* D0 and D1 are doubles */
    ws->init = 1; ws->cnum = -1;

    /* read wall parameters from ftable */

    if (ft != NULL) {
      spat3d_init_eq(p, ws, ft + 4);     /* EQ                */
      a = -ft[3];                             /* scale             */
      ws->b0 *= a; ws->b1 *= a; ws->b2 *= a;  /* apply scale to EQ */
      ws->cnum = (6 - wallno) >> 1;           /* select wall       */
      w = ft[1];                              /* wall distance     */
      x = ft[2];                              /* randomize         */
      x *= ((MYFLT) p->rseed - FL(32767.5)) / FL(32767.5);
      w *= (x + FL(1.0)) * (wallno & 1 ? FL(2.0) : FL(-2.0));
      ws->Xc = w;                             /* coord. offset     */
    }

    /* convert coordinates (spat3di, and spat3dt) */

    if (p->o_num != 1) {
      switch (ws->cnum) {                     /* new coordinates   */
      /* FALLTHRU */ case 0: X = ws->Xc - X; break;
      /* FALLTHRU */ case 1: Y = ws->Xc - Y; break;
      /* FALLTHRU */ case 2: Z = ws->Xc - Z; break;
      }
      if (p->zout < 4) {                      /* convert coord.    */
        d = SPAT3D_XYZ2DIST(X, Y, Z);         /* distance  */
        d0 = d1 = (double) SPAT3D_DIST2DEL(d);  /* delay     */
        a = SPAT3D_DIST2AMP(d);                /* amp.      */
        d = FL(1.0) / (d > p->mdist ? d : p->mdist);
        w = x = y = z = FL(0.0);
        switch (p->zout) {
        case 3: z =  Z * d; w += z*z; z *= a;   /* Z */
          /* FALLTHRU */
        case 2: x =  Y * d; w += x*x; x *= a;   /* X */
        /* FALLTHRU */
        case 1: y = -X * d; w += y*y; y *= a;   /* Y */
        }
        w = a - FL(0.293) * w * a;                      /* W */
      }
      else {
        x = X - p->mdist * FL(0.5);             /* right channel */
        d = SPAT3D_XYZ2DIST(x, Y, Z);           /* distance  */
        d1 = (double) SPAT3D_DIST2DEL(d);       /* delay     */
        a = SPAT3D_DIST2AMP(d);                 /* amp.      */
        z = (MYFLT) sqrt(1.0 + (double) (x / (d + FL(0.0001))));
        z *= a; y = a - z;                      /* Rh, Rl    */
        x += p->mdist;                          /* left channel */
        d = SPAT3D_XYZ2DIST(x, Y, Z);           /* distance  */
        d0 = (double) SPAT3D_DIST2DEL(d);       /* delay     */
        a = SPAT3D_DIST2AMP(d);                 /* amp.      */
        x = (MYFLT) sqrt(1.0 - (double) (x / (d + FL(0.0001))));
        x *= a; w = a - x;                      /* Lh, Ll    */
      }
      if (dep <= p->mindep) {
        w = x = y = z = FL(0.0); d0 = d1 = 0.0;
      }
      /* extend delay buffer */
      if ((MYFLT) d0 > p->mdel) p->mdel = (MYFLT) d0;
      if ((MYFLT) d1 > p->mdel) p->mdel = (MYFLT) d1;
      ws->D0 = d0 * (double) CS_ESR + 0.5;
      ws->D1 = d1 * (double) CS_ESR + 0.5;
      ws->W0 = w; ws->X0 = x; ws->Y0 = y; ws->Z0 = z;
    }

    /* return if there are no more reflections */

    if ((dep > p->maxdep) || (p->ftable == NULL)) return ws;

    /* next reflections */

    for (i = 1; i <= 6; i++)
      if ((p->ftable[(i << 3) - 2] > FL(0.5))
          && ((i > wallno) || ((i == (wallno - 1)) && (i & 1))))
        ws->nextRefl[i - 1] = spat3d_init_wall(p, i, dep, wmax, X, Y, Z);

    return ws;      /* return pointer to new wall structure */
}

/* allocate space for delay buffers */

static int32_t spat3d_init_delay(CSOUND *csound, SPAT3D *p)
{
    int32_t    i, j;

    i = ((int32_t) (p->mdel * CS_ESR) + (int32_t) CS_KSMPS + 34L)
        * (int32_t) p->oversamp;
    p->mdel_s = i;
    if (p->o_num == 1) i += 4;      /* extra samples for spat3d */
    j = i * (int32_t) sizeof(MYFLT) * (int32_t) (p->zout > 3 ? 4 : p->zout + 1);
    if ((p->del.auxp == NULL) || (p->del.size < (uint32_t)j)) /* allocate */
      csound->AuxAlloc(csound, j, &(p->del));               /* space    */
    p->Wb = (MYFLT *) p->del.auxp;                  /* W */
    if (p->zout > 0) p->Yb = p->Wb + i;             /* Y */
    if (p->zout > 1) p->Xb = p->Yb + i;             /* X */
    if (p->zout > 2) p->Zb = p->Xb + i;             /* Z */
    for (j = 0; j < i; j++) {
      switch (p->zout) {              /* clear buffers */
      /* FALLTHRU */ case 4:
      /* FALLTHRU */ case 3: p->Zb[j] = FL(0.0);
      /* FALLTHRU */ case 2: p->Xb[j] = FL(0.0);
      /* FALLTHRU */ case 1: p->Yb[j] = FL(0.0);
      /* FALLTHRU */ case 0: p->Wb[j] = FL(0.0);
      }
    }
    if (p->o_num == 1) {
      switch (p->zout) {
      /* FALLTHRU */ case 4:
      /* FALLTHRU */ case 3: (p->Zb)++;      /* spat3d requires 1    */
      /* FALLTHRU */ case 2: (p->Xb)++;      /* extra sample before  */
      /* FALLTHRU */ case 1: (p->Yb)++;      /* and 3 samples after  */
      /* FALLTHRU */ case 0: (p->Wb)++;      /* the delay buffer     */
      }
    }
    return OK;
}

/* count reflections */

static void spat3d_count_refl(int32_t *cnt, int32_t *md,
                              int32_t d, int32_t mdep, int32_t w, int32_t wm)
{
    int32_t     j;

    (*cnt)++;                       /* update count    */
    if (++d > *md) *md = d;         /* md is the max. depth reached + 1 */
    if (d > mdep) return;           /* mdep is the max. depth allowed   */
    for (j = 32; j; j >>= 1)        /* new reflections */
      if ((wm & j) && ((j > w) || ((j == (w >> 1)) && (j & 21))))
        spat3d_count_refl(cnt, md, d, mdep, j, wm);
}

/* initialise opcode structure */

static int32_t spat3d_set_opcode_params(CSOUND *csound, SPAT3D *p)
{
    int32_t     xidist, xift, ximode, ximdel, xiovr, xirlen, xioutft;
    int32_t     d, wmask;
    int32_t    i;

    /* default settings */

    p->ftable = p->outft = NULL;    /* no ftables */
    p->zout = p->rseed = p->mindep = p->maxdep = p->outftlnth = wmask = 0;
    p->oversamp = 1;                /* oversample */
    p->bs = (int32_t) CS_KSMPS;    /* block size */
    p->irlen = 2;                   /* IR length  */
    p->mdist = p->mdel = FL(0.001); /* unit circle dist., max. delay */
    p->mdel_s = p->del_p = 0L;
    p->Wb = p->Xb = p->Yb = p->Zb = NULL;

    /* select opcode */

    xidist = xift = ximode = ximdel = xiovr = xirlen = xioutft = -1;
    switch (p->o_num) {
    /* FALLTHRU */ case 1: ximdel = 11; xiovr = 12;                /* spat3d  */
    /* FALLTHRU */ case 0: xidist = 8; xift = 9; ximode = 10;      /* spat3di */
      break;
    /* FALLTHRU */ case 2: xidist = 4; xift = 5; ximode = 6;       /* spat3dt */
      xirlen = 7; xioutft = 0;
      break;
    }

    /* read opcode args */

    if (ximode >= 0)                                /* output mode */
      p->zout = (int32_t) MYFLT2LRND(*(p->args[ximode]));
    if (xidist >= 0)                                /* unit circle dist. */
      p->mdist = *(p->args[xidist]);
    if (xift >= 0) {                                /* ftable */
      FUNC *ftp = csound->FTFind(csound, p->args[xift]);
      if (ftp->flen < 53)
        p->ftable = NULL;
      else p->ftable = ftp->ftable;
    }
    if (ximdel >= 0)                                /* max. delay */
      p->mdel = *(p->args[ximdel]);
    if (xiovr >= 0)                                 /* oversample */
      p->oversamp = (int32_t) MYFLT2LRND(*(p->args[xiovr]));
    if (xirlen >= 0)                                /* IR length */
      p->irlen = (int32_t) MYFLT2LRND(*(p->args[xirlen]) * CS_ESR);
    if (xioutft >= 0) {                             /* output table */
      FUNC *ftp = csound->FTFind(csound, p->args[xioutft]);
      if (ftp->flen < 1) {
        p->outft = NULL; p->outftlnth = 0;
      }
      else {
        p->outftlnth = ftp->flen;
        p->outft = ftp->ftable;
      }
    }

    /* get parameters from ftable */

    if (p->ftable != NULL) {
      if (p->o_num == 2) {                    /* min, max depth    */
        p->mindep = (int32_t) MYFLT2LRND(p->ftable[0]) + 1;
        p->maxdep = (int32_t) MYFLT2LRND(p->ftable[1]);
      }
      else {
        p->mindep = 0;
        p->maxdep = (int32_t) MYFLT2LRND(p->ftable[0]);
      }
      if (p->ftable[2] >= FL(0.0))            /* max. delay        */
        p->mdel = p->ftable[2];
      if (p->ftable[3] >= FL(0.0))            /* IR length         */
        p->irlen = (int32_t) MYFLT2LRND(p->ftable[3] * CS_ESR);
      if (p->ftable[4] >= FL(0.0))            /* unit circle dist. */
        p->mdist = p->ftable[4];
      p->rseed = (int32_t) MYFLT2LRND(p->ftable[5]);     /* seed      */
      if (p->rseed < 0L)
        p->rseed = (int32_t) csound->GetRandomSeedFromTime() & 0xFFFFL;
      for (i = 6; i; i--) {                   /* wall mask         */
        wmask <<= 1; if (p->ftable[i*8 - 2] > FL(0.5)) wmask++;
      }
    }

    /* limit parameters to useful range */

    p->oversamp = SPAT3D_LIMIT(p->oversamp, 1, 8);
    p->zout = SPAT3D_LIMIT(p->zout, 0, 4);
    p->mdist = SPAT3D_LIMIT(p->mdist, FL(0.001), FL(1000.0));
    p->rseed = SPAT3D_LIMIT(p->rseed, 0L, 65535L);
    p->mindep = SPAT3D_LIMIT(p->mindep, -1, 256);
    p->maxdep = SPAT3D_LIMIT(p->maxdep, -1, 256);
    p->irlen = SPAT3D_LIMIT(p->irlen, 2, 32000);
    p->mdel = SPAT3D_LIMIT(p->mdel, FL(0.001), FL(1000.0));

    if (p->o_num == 2) p->bs = p->irlen;            /* block size        */

    /* allocate space */

    if (p->maxdep >= 0) {
      i = d = 0; spat3d_count_refl(&i, &d, 0, p->maxdep, 0, wmask);
      i *= (int32_t) sizeof(SPAT3D_WALL);
      if ((p->ws.auxp == NULL) || (p->ws.size < (uint32_t)i))
        csound->AuxAlloc(csound, i, &(p->ws));
      i = (int32_t) p->bs * (int32_t) d;
      i *= (int32_t) sizeof(MYFLT);
      if ((p->y.auxp == NULL) || (p->y.size < (uint32_t)i))
        csound->AuxAlloc(csound, i, &(p->y));
    }
    return OK;
}

/* -------------- initialisation functions for spat3d opcode --------------- */

/* spat3d set-up */

static int32_t    spat3dset(CSOUND *csound, SPAT3D *p)
{
    int32_t    wmax;

    if (*(p->args[13]) != FL(0.0)) return OK; /* skip init    */
    p->o_num = 1;                             /* opcode number         */
    spat3d_set_opcode_params(csound, p);      /* set parameters        */
    if (p->maxdep < 0) return OK;             /* nothing to render */
    wmax = 0L;                                /* init. wall structures */
    spat3d_init_wall(p, 0, 0, &wmax, FL(0.0), FL(0.0), FL(0.0));
    spat3d_init_delay(csound, p);             /* alloc delay buffers   */
    spat3d_init_window(csound, p);            /* init. FIR filter      */
    return OK;
}

/* -------------- initialisation functions for spat3di opcode -------------- */

/* spat3di set-up */

static int32_t    spat3diset(CSOUND *csound, SPAT3D *p)
{
    int32_t    wmax;

    if (*(p->args[11]) != FL(0.0)) return OK; /* skip init             */
    p->o_num = 0;                             /* opcode number         */
    spat3d_set_opcode_params(csound, p);      /* set parameters        */
    if (p->maxdep < 0) return OK;             /* nothing to render     */
    wmax = 0L; p->mdel = FL(0.0);             /* init. wall structures */
    spat3d_init_wall(p, 0, 0, &wmax,
                     *(p->args[5]), *(p->args[6]), *(p->args[7]));
    spat3d_init_delay(csound, p);             /* alloc delay buffers   */
    return OK;
}

/* -------------------------- spat3d performance --------------------------- */

/* spat3d wall perf */

static void spat3d_wall_perf(CSOUND     *csound, /* General environment       */
                             SPAT3D     *p,      /* opcode struct             */
                             MYFLT      *xn,     /* input signal              */
                             SPAT3D_WALL *ws,    /* wall parameters structure */
                             MYFLT      X,       /* sound source X coordinate */
                             MYFLT      Y,       /* sound source Y coordinate */
                             MYFLT      Z)       /* sound source Z coordinate */
{
    MYFLT       *yn, W0, X0, Y0, Z0, *Wb, *Xb, *Yb, *Zb;
    MYFLT       a, d, w, x, y, z, wd, xd, yd, zd, x1;
    double      d0, d1, d0d, d1d, D0, D1;
    int32_t        xpos, nn, pos;

    yn = ws->yn; D0 = ws->D0; D1 = ws->D1; pos = p->del_p;
    W0 = ws->W0; X0 = ws->X0; Y0 = ws->Y0; Z0 = ws->Z0;
    Wb = Xb = Yb = Zb = NULL;

    /* calculate coordinates of reflection      */

    switch (ws->cnum) {
    /* FALLTHRU */ case 0:     X = ws->Xc - X; break;
    /* FALLTHRU */ case 1:     Y = ws->Xc - Y; break;
    /* FALLTHRU */ case 2:     Z = ws->Xc - Z; break;
    }

    /* convert coordinates */

    if (p->zout < 4) {
      d = SPAT3D_XYZ2DIST(X, Y, Z);             /* distance  */
      d0 = d1 = (double) SPAT3D_DIST2DEL(d);    /* delay     */
      a = SPAT3D_DIST2AMP(d);                   /* amp.      */
      d = FL(1.0) / (d > p->mdist ? d : p->mdist);
      w = x = y = z = FL(0.0);
      switch (p->zout) {
      /* FALLTHRU */ case 3:   z =  Z * d; w += z*z; z *= a;   /* Z */
      /* FALLTHRU */ case 2:   x =  Y * d; w += x*x; x *= a;   /* X */
      /* FALLTHRU */ case 1:   y = -X * d; w += y*y; y *= a;   /* Y */
      }
      w = a - FL(0.293) * w * a;                /* W */
    }
    else {
      x = X - p->mdist * FL(0.5);               /* right channel */
      d = SPAT3D_XYZ2DIST(x, Y, Z);             /* distance  */
      d1 = (double) SPAT3D_DIST2DEL(d);         /* delay     */
      a = SPAT3D_DIST2AMP(d);                   /* amp.      */
      z = SQRT(FL(1.0) + (x / (d + FL(0.0001))));
      z *= a; y = a - z;                        /* Rh, Rl    */
      x += p->mdist;                            /* left channel */
      d = SPAT3D_XYZ2DIST(x, Y, Z);             /* distance  */
      d0 = (double) SPAT3D_DIST2DEL(d);         /* delay     */
      a = SPAT3D_DIST2AMP(d);                   /* amp.      */
      x = SQRT(FL(1.0) - (x / (d + FL(0.0001))));
      x *= a; w = a - x;                                /* Lh, Ll   */
      d1 *= (double) p->oversamp * (double) CS_ESR;/* convert  */
    }                                                   /* delay to */
    d0 *= (double) p->oversamp * (double) CS_ESR;  /* samples  */

    /* interpolate W, X, Y, Z, and delay */

    if (ws->init) {             /* first k-cycle */
      D0 = d0; D1 = d1; W0 = w; X0 = x; Y0 = y; Z0 = z; ws->init = 0;
    }
    d0d = d1d = 1.0 / (double) p->bs;
    wd = xd = yd = zd = (MYFLT) d0d;
    a = (MYFLT) p->oversamp;
    switch (p->zout) {
    /* FALLTHRU */ case 4:     d1d *= (d1 - D1); D1 -= d1d * 0.5;
      d = (a + (MYFLT) d1d) / a;        /* correct amplitude */
    /* FALLTHRU */ case 3:     zd *= (z - Z0);
    /* FALLTHRU */ case 2:     xd *= (x - X0);
    /* FALLTHRU */ case 1:     yd *= (y - Y0);
    /* FALLTHRU */ case 0:     wd *= (w - W0);
      d0d *= (d0 - D0); D0 -= d0d * 0.5;
      a = (a + (MYFLT) d0d) / a;
    }

    nn = p->bs; while (nn--) {  /* EQ */

      *yn = ws->b2 * ws->xnm2; *yn += ws->b1 * (ws->xnm2 = ws->xnm1);
      *yn += ws->b0 * (ws->xnm1 = *(xn++));
      *yn -= ws->a2 * ws->ynm2; *yn -= ws->a1 * (ws->ynm2 = ws->ynm1);
      ws->ynm1 = *yn;

      /* write to delay buffer with cubic interpolation */

      xpos = (int32_t) (D0 += d0d);
      x1 = (MYFLT) (D0 - (double) (xpos--));
      z = x1 * x1; z--; z *= FL(0.1666666667);
      y = x1; y++; w = (y *= FL(0.5)); w--;
      x = FL(3.0) * z; y -= x; w -= z; x -= x1;
      x1 *= *yn * a;            /* correct amplitude */
      w *= x1;                  /* sample -1 */
      x *= x1; x += *yn * a;    /* sample 0  */
      y *= x1;                  /* sample +1 */
      z *= x1;                  /* sample +2 */
      xpos += pos; while (xpos >= p->mdel_s) xpos -= p->mdel_s;
      Wb = p->Wb + xpos; W0 += wd;                      /* W / Ll */
      *(Wb++) += w * W0; *(Wb++) += x * W0;
      *(Wb++) += y * W0; *Wb += z * W0;
      switch (p->zout) {
      /* FALLTHRU */ case 3:   Zb = p->Zb + xpos; Z0 += zd;            /* Z */
        *(Zb++) += w * Z0; *(Zb++) += x * Z0;
        *(Zb++) += y * Z0; *Zb += z * Z0;
      /* FALLTHRU */ case 2:   Xb = p->Xb + xpos; X0 += xd;            /* X */
        *(Xb++) += w * X0; *(Xb++) += x * X0;
        *(Xb++) += y * X0; *Xb += z * X0;
      /* FALLTHRU */ case 1:   Yb = p->Yb + xpos; Y0 += yd;            /* Y */
        *(Yb++) += w * Y0; *(Yb++) += x * Y0;
        *(Yb++) += y * Y0; *Yb += z * Y0;
      /* FALLTHRU */ case 0:   break;
      /* FALLTHRU */ case 4:   Xb = p->Xb + xpos; X0 += xd;    /* Lh */
        *(Xb++) += w * X0; *(Xb++) += x * X0;
        *(Xb++) += y * X0; *Xb += z * X0;
        xpos = (int32_t) (D1 += d1d);
        x1 = (MYFLT) (D1 - (double) (xpos--));
        z = x1 * x1; z--; z *= FL(0.1666666667);
        y = x1; y++; w = (y *= FL(0.5)); w--;
        x = FL(3.0) * z; y -= x; w -= z; x -= x1;
        x1 *= *yn * d;                  /* correct amplitude */
        w *= x1;                        /* sample -1 */
        x *= x1; x += *yn * d;          /* sample 0  */
        y *= x1;                        /* sample +1 */
        z *= x1;                        /* sample +2 */
        xpos += pos;
        while (xpos >= p->mdel_s) xpos -= p->mdel_s;
        Yb = p->Yb + xpos; Y0 += yd;    /* Rl */
        *(Yb++) += w * Y0; *(Yb++) += x * Y0;
        *(Yb++) += y * Y0; *Yb += z * Y0;
        Zb = p->Zb + xpos; Z0 += zd;    /* Rh */
        *(Zb++) += w * Z0; *(Zb++) += x * Z0;
        *(Zb++) += y * Z0; *Zb += z * Z0;
      }

      pos += p->oversamp; yn++; /* next sample */
    }

    ws->D0 = D0 + d0d * 0.5; if (p->zout == 4) ws->D1 = D1 + d1d * 0.5;
    ws->W0 = W0; ws->X0 = X0; ws->Y0 = Y0; ws->Z0 = Z0;

    /* next reflection(s) */

    for (nn = 0; nn < 6; nn++)
      if (ws->nextRefl[nn] != NULL)
        spat3d_wall_perf(csound, p, ws->yn, (SPAT3D_WALL *) ws->nextRefl[nn],
                         X, Y, Z);
}

/* spat3d routine */

static int32_t    spat3d(CSOUND *csound, SPAT3D *p)
{
    int32_t        nn, i, j;
    MYFLT       *aoutW, *aoutX, *aoutY, *aoutZ, w;

    /* assign object data to local variables */

    aoutW = p->args[0]; aoutX = p->args[1];
    aoutY = p->args[2]; aoutZ = p->args[3];

    /* clear output variables */

    nn = -1; while (++nn < p->bs)
      aoutW[nn] = aoutX[nn] = aoutY[nn] = aoutZ[nn] = FL(0.0);

    if (p->maxdep < 0) return OK;  /* depth < 0 : nothing to render */

    if (UNLIKELY((p->ws.auxp == NULL) || (p->y.auxp == NULL))) goto err1;

    /* spatialize and send to delay line */

    j = p->mdel_s;
    switch (p->zout) {  /* clear extra samples at beginning and */
    /* FALLTHRU */
    case 4:             /* end of delay line                    */
    /* FALLTHRU */
    case 3:     *(p->Zb - 1) = p->Zb[j] = p->Zb[j + 1] = p->Zb[j + 2] =
                  FL(0.0);
    /* FALLTHRU */
    case 2:     *(p->Xb - 1) = p->Xb[j] = p->Xb[j + 1] = p->Xb[j + 2] =
                  FL(0.0);
    /* FALLTHRU */
    case 1:     *(p->Yb - 1) = p->Yb[j] = p->Yb[j + 1] = p->Yb[j + 2] =
                  FL(0.0);
    /* FALLTHRU */
    case 0:     *(p->Wb - 1) = p->Wb[j] = p->Wb[j + 1] = p->Wb[j + 2] =
                  FL(0.0);
    }
    spat3d_wall_perf(csound, p, p->args[4], (SPAT3D_WALL *) p->ws.auxp,
                     *(p->args[5]), *(p->args[6]), *(p->args[7]));
    switch (p->zout) {  /* copy extra samples from beginning and */
    /* FALLTHRU */ case 4:             /* end of delay line                     */
    /* FALLTHRU */ case 3:     p->Zb[j - 1] += *(p->Zb - 1); p->Zb[0] += p->Zb[j];
      p->Zb[1] += p->Zb[j + 1]; p->Zb[2] += p->Zb[j + 2];
    /* FALLTHRU */ case 2:     p->Xb[j - 1] += *(p->Xb - 1); p->Xb[0] += p->Xb[j];
      p->Xb[1] += p->Xb[j + 1]; p->Xb[2] += p->Xb[j + 2];
    /* FALLTHRU */ case 1:     p->Yb[j - 1] += *(p->Yb - 1); p->Yb[0] += p->Yb[j];
      p->Yb[1] += p->Yb[j + 1]; p->Yb[2] += p->Yb[j + 2];
    /* FALLTHRU */ case 0:     p->Wb[j - 1] += *(p->Wb - 1); p->Wb[0] += p->Wb[j];
      p->Wb[1] += p->Wb[j + 1]; p->Wb[2] += p->Wb[j + 2];
    }

    nn = p->bs; while (nn--) {  /* read from delay line */
                                /* with interpolation   */
      i = 0;
      while ((j = p->sample[i]) > -10000) {
        w = p->window[i++];
        j += p->del_p;
        if (j < 0) j += p->mdel_s;
        if (j >= p->mdel_s) j -= p->mdel_s;
        switch (p->zout) {
        /* FALLTHRU */ case 4:
        /* FALLTHRU */ case 3: *aoutZ += p->Zb[j] * w;
        /* FALLTHRU */ case 2: *aoutX += p->Xb[j] * w;
        /* FALLTHRU */ case 1: *aoutY += p->Yb[j] * w;
        /* FALLTHRU */ case 0: *aoutW += p->Wb[j] * w;
        }
      }
      j = p->del_p - (p->oversamp << 4); if (j < 0) j += p->mdel_s;
      for (i = 0; i < p->oversamp; i++, j++) {
        switch (p->zout) {
        /* FALLTHRU */ case 4:
        /* FALLTHRU */ case 3: p->Zb[j] = FL(0.0);
        /* FALLTHRU */ case 2: p->Xb[j] = FL(0.0);
        /* FALLTHRU */ case 1: p->Yb[j] = FL(0.0);
        /* FALLTHRU */ case 0: p->Wb[j] = FL(0.0);
        }
      }
      aoutW++; aoutX++; aoutY++; aoutZ++;               /* next   */
      if ((p->del_p += p->oversamp) >= p->mdel_s)       /* sample */
        p->del_p -= p->mdel_s;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("spat3d: not initialised"));
}

/* -------------------------- spat3di performance -------------------------- */

/* spat3di wall perf */

static void spat3di_wall_perf(SPAT3D        *p,     /* opcode struct    */
                              MYFLT         *xn,    /* input signal     */
                              SPAT3D_WALL   *ws)    /* wall params      */
{
    MYFLT       *yn, *Wb, *Xb, *Yb, *Zb, w, x, y, z;
    int32_t     xpos0, xpos1, nn, bs;

    yn = ws->yn;
    bs = p->mdel_s;
    xpos0 = (int32_t) ws->D0 + p->del_p; while (xpos0 >= bs) xpos0 -= bs;
    xpos1 = xpos0; Wb = Xb = Yb = Zb = NULL;
    w = ws->W0; x = ws->X0; y = ws->Y0; z = ws->Z0;
    switch (p->zout) {
    /* FALLTHRU */ case 4:     xpos1 = (int32_t) ws->D1 + p->del_p;
      while (xpos1 >= bs) xpos1 -= bs;
    /* FALLTHRU */ case 3:     Zb = p->Zb + xpos1;             /* Z / Rh */
    /* FALLTHRU */ case 2:     Xb = p->Xb + xpos0;             /* X / Lh */
    /* FALLTHRU */ case 1:     Yb = p->Yb + xpos1;             /* Y / Rl */
    /* FALLTHRU */ case 0:     Wb = p->Wb + xpos0;             /* W / Ll */
    }

    nn = p->bs; while (nn--) {  /* EQ */

      *yn = ws->b2 * ws->xnm2; *yn += ws->b1 * (ws->xnm2 = ws->xnm1);
      *yn += ws->b0 * (ws->xnm1 = *(xn++));
      *yn -= ws->a2 * ws->ynm2; *yn -= ws->a1 * (ws->ynm2 = ws->ynm1);
      ws->ynm1 = *yn;

      /* write to delay buffer */

      switch (p->zout) {
      /* FALLTHRU */ case 3:   *(Zb++) += *yn * z;             /* Z */
      /* FALLTHRU */ case 2:   *(Xb++) += *yn * x;             /* X */
      /* FALLTHRU */ case 1:   *(Yb++) += *yn * y;             /* Y */
      /* FALLTHRU */ case 0:   *(Wb++) += *yn * w;             /* W */
        if (++xpos0 >= bs) {
          xpos0 -= bs;
          switch (p->zout) {
          /* FALLTHRU */ case 3:       Zb -= bs;
          /* FALLTHRU */ case 2:       Xb -= bs;
          /* FALLTHRU */ case 1:       Yb -= bs;
          /* FALLTHRU */ case 0:       Wb -= bs;
          }
        }
        break;
      /* FALLTHRU */ case 4:   *(Wb++) += *yn * w;             /* Ll */
        *(Xb++) += *yn * x;                     /* Lh */
        *(Yb++) += *yn * y;                     /* Rl */
        *(Zb++) += *yn * z;                     /* Rh */
        if (++xpos0 >= bs) {
          xpos0 -= bs; Wb -= bs; Xb -= bs;
        }
        if (++xpos1 >= bs) {
          xpos1 -= bs; Yb -= bs; Zb -= bs;
        }
      }
      yn++;
    }

    /* next reflection(s) */

    for (nn = 0; nn < 6; nn++)
      if (ws->nextRefl[nn] != NULL)
        spat3di_wall_perf(p, ws->yn, (SPAT3D_WALL *) ws->nextRefl[nn]);
}

/* spat3di routine */

static int32_t    spat3di(CSOUND *csound, SPAT3D *p)
{
    int32_t     nn;
    MYFLT       *a_outW, *a_outX, *a_outY, *a_outZ;

    /* assign object data to local variables */

    a_outW = p->args[0]; a_outX = p->args[1];
    a_outY = p->args[2]; a_outZ = p->args[3];

    /* clear output variables */

    nn = 0; do {
      a_outW[nn] = a_outX[nn] = a_outY[nn] = a_outZ[nn] = FL(0.0);
    } while (++nn < p->bs);

    if (p->maxdep < 0) return OK;  /* depth < 0 : nothing to render */

    if (UNLIKELY((p->ws.auxp == NULL) || (p->y.auxp == NULL))) goto err1;

    /* spatialize and send to delay line */

    spat3di_wall_perf(p, p->args[4], (SPAT3D_WALL *) p->ws.auxp);

    nn = p->bs;
    do {        /* read from delay line */
      switch (p->zout) {
      /* FALLTHRU */ case 4:
      /* FALLTHRU */ case 3:   *(a_outZ++) = p->Zb[p->del_p];          /* Z */
        p->Zb[p->del_p] = FL(0.0);
        /* FALLTHRU */
      /* FALLTHRU */ case 2:   *(a_outX++) = p->Xb[p->del_p];          /* X */
        p->Xb[p->del_p] = FL(0.0);
        /* FALLTHRU */
      /* FALLTHRU */ case 1:   *(a_outY++) = p->Yb[p->del_p];          /* Y */
        p->Yb[p->del_p] = FL(0.0);
        /* FALLTHRU */
      /* FALLTHRU */ case 0:   *(a_outW++) = p->Wb[p->del_p];          /* W */
        p->Wb[p->del_p] = FL(0.0);
      }
      /* next sample */
      if (++(p->del_p) >= p->mdel_s) p->del_p -= p->mdel_s;
    } while (--nn);
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("spat3di: not initialised"));
}

/* ---------------------------- spat3dt opcode ----------------------------- */

/* spat3dt wall perf */

static void spat3dt_wall_perf(SPAT3D        *p,     /* opcode struct    */
                              MYFLT         *xn,    /* input signal     */
                              SPAT3D_WALL   *ws)    /* wall params      */
{
    MYFLT       *yn, *Wb, *Yb, *endp, w, x, y, z, a, d, ad, yw;
    int32_t     nn;
    yn = ws->yn;
    endp = p->outft;            /* write to ftable      */
    Wb = endp + ((int32_t) ws->D0 << 2);
    Yb = (p->zout < 4 ? Wb : endp + ((int32_t) ws->D1 << 2)) + 2;
    endp += p->outftlnth;       /* end of table         */

    w = ws->W0; x = ws->X0; y = ws->Y0; z = ws->Z0;
    a = FL(1.0) / (MYFLT) p->bs; ad = FL(2.0) * (a *= -a); d = FL(1.0);

    nn = p->bs; while (nn--) {  /* EQ */

      *yn = ws->b2 * ws->xnm2; *yn += ws->b1 * (ws->xnm2 = ws->xnm1);
      *yn += ws->b0 * (ws->xnm1 = *(xn++));
      *yn -= ws->a2 * ws->ynm2; *yn -= ws->a1 * (ws->ynm2 = ws->ynm1);
      ws->ynm1 = *yn;
      yw = *yn * d;             /* yw: windowed signal */

      /* write to delay buffer */

      if (p->zout < 4) {
        if (Wb < endp) {
          *(Wb++) += yw * w; *(Wb++) += yw * x; /* W, X */
          *(Wb++) += yw * y; *(Wb++) += yw * z; /* Y, Z */
        }
      } else {
        if (Wb < endp) {                /* Ll, Lh */
          *(Wb++) += yw * w; *Wb += yw * x; Wb += 3;
        }
        if (Yb < endp) {                /* Rl, Rh */
          *(Yb++) += yw * y; *Yb += yw * z; Yb += 3;
        }
      }
      d += a; a += ad; yn++;
    }

    /* next reflection(s) */

    for (nn = 0; nn < 6; nn++)
      if (ws->nextRefl[nn] != NULL)
        spat3dt_wall_perf(p, ws->yn, (SPAT3D_WALL *) ws->nextRefl[nn]);
}

/* spat3dt opcode (i-time only) */

static int32_t    spat3dt(CSOUND *csound, SPAT3D *p)
{
    int32_t wmax;
    MYFLT   *ir;

    p->o_num = 2;                           /* opcode number         */
    spat3d_set_opcode_params(csound, p);    /* set parameters        */
    if (p->maxdep < 0) return OK;           /* nothing to render */
    wmax = 0L; p->mdel = FL(0.0);           /* init. wall structures */
    spat3d_init_wall(p, 0, 0, &wmax,
                     *(p->args[1]), *(p->args[2]), *(p->args[3]));
    p->outftlnth = ((p->outftlnth) >> 2) << 2;      /* table length  */
    if (UNLIKELY((p->outft == NULL) || (p->outftlnth < 4)))
      return NOTOK;                         /* no table */

    /* initialise IR */

    ir = (MYFLT *) csound->Malloc(csound, sizeof(MYFLT) * (int32_t) p->bs);
    ir[0] = FL(1.0);
    wmax = 0; while (++wmax <  p->bs)
      ir[wmax] = (sizeof(MYFLT) < 8 ? FL(1.0e-24) : FL(1.0e-48));

    if (*(p->args[8]) == FL(0.0)) {          /* clear ftable (if enabled) */
      wmax = -1; while (++wmax < p->outftlnth)
        p->outft[wmax] = FL(0.0);
    } /* braces added by JPff -- REVIEW */
    /* render IR */

    spat3dt_wall_perf(p, ir, (SPAT3D_WALL *) p->ws.auxp);

    csound->Free(csound, ir);               /* free tmp memory */
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
   { "spat3d", S(SPAT3D), 0,  "aaaa", "akkkiiiiio",
     (SUBR) spat3dset,   (SUBR) spat3d   },
   { "spat3di",S(SPAT3D), 0,  "aaaa", "aiiiiiio",
     (SUBR) spat3diset,   (SUBR) spat3di  },
   { "spat3dt",S(SPAT3D), 0,  "", "iiiiiiiio",
     (SUBR) spat3dt,     NULL,   NULL            }
};

int32_t spat3d_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

