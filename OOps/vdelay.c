/*
    vdelay.c:

    Copyright (C) 1994, 1998, 2000, 2001 Paris Smaragdis, Richard Karpen,
                                         rasmus ekman, John ffitch

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

/*      vdelay, multitap, reverb2 coded by Paris Smaragdis 1994 */
/*      Berklee College of Music Csound development team        */
/*      Copyright (c) May 1994.  All rights reserved            */

#include "csoundCore.h"

#include <math.h>
#include "vdelay.h"

//#define ESR     (csound->esr/FL(1000.0))
#define ESR     (csound->esr*FL(0.001))

int32_t vdelset(CSOUND *csound, VDEL *p)            /*  vdelay set-up   */
{
    uint32 n = (int32_t)(*p->imaxd * ESR)+1;

    if (!*p->istod) {
      if (p->aux.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux.size)
        /* allocate space for delay buffer */
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux);
      else {     /*    make sure buffer is empty       */
        memset(p->aux.auxp, '\0', n*sizeof(MYFLT));
      }
      p->left = 0;
    }
    p->maxd = n - 1;
    return OK;
}

int32_t vdelay(CSOUND *csound, VDEL *p)               /*      vdelay  routine */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out = p->sr;     /* assign object data to local variables   */
    MYFLT *in = p->ain;
    MYFLT *del = p->adel;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT esr = ESR;

    if (UNLIKELY(buf==NULL)) goto err1;        /* RWD fix */
    maxd = p->maxd;
    indx = p->left;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (IS_ASIG_ARG(p->adel)) {          /*      if delay is a-rate      */
      for (nn=offset; nn<nsmps; nn++) {
        MYFLT  fv1, fv2;
        int32_t   v1, v2;

        buf[indx] = in[nn];
        fv1 = indx - (del[nn]) * esr;
        /* Make sure Inside the buffer      */
        /*
         * The following has been fixed by adding a cast and making a
         * ">=" instead of a ">" comparison. The order of the comparisons
         * has been swapped as well (a bit of a nit, but comparing a
         * possibly negative number to an unsigned isn't a good idea--and
         * broke on Alpha).
         * heh 981101
         */
        while (UNLIKELY(fv1 < FL(0.0)))
          fv1 += (MYFLT)maxd;
        while (UNLIKELY(fv1 >= (MYFLT)maxd))
          fv1 -= (MYFLT)maxd;

        if (LIKELY(fv1 < maxd - 1)) /* Find next sample for interpolation      */
          fv2 = fv1 + FL(1.0);
        else
          fv2 = FL(0.0);

        v1 = (int32_t)fv1;
        v2 = (int32_t)fv2;
        out[nn] = buf[v1] + (fv1 - v1) * ( buf[v2] - buf[v1]);

        if (UNLIKELY(++indx == maxd))
          indx = 0;             /* Advance current pointer */

      }
    }
    else {                      /* and, if delay is k-rate */
      MYFLT fdel=*del;
      for (nn=offset; nn<nsmps; nn++) {
        MYFLT  fv1, fv2;
        int32_t   v1, v2;

        buf[indx] = in[nn];
        fv1 = indx - fdel * esr;
        /* Make sure inside the buffer      */
        /*
         * See comment above--same fix applied here.  heh 981101
         */
        while (UNLIKELY(fv1 < FL(0.0)))
          fv1 += (MYFLT)maxd;
        while (UNLIKELY(fv1 >= (MYFLT)maxd))
          fv1 -= (MYFLT)maxd;

        if (LIKELY(fv1 < maxd - 1)) /* Find next sample for interpolation      */
          fv2 = fv1 + FL(1.0);
        else
          fv2 = FL(0.0);

        v1 = (int32_t)fv1;
        v2 = (int32_t)fv2;
        out[nn] = buf[v1] + (fv1 - v1) * ( buf[v2] - buf[v1]);

        if (UNLIKELY(++indx == maxd)) indx = 0;   /*      Advance current pointer */

      }
    }
    p->left = indx;             /*      and keep track of where you are */
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelay3(CSOUND *csound, VDEL *p)    /*  vdelay routine with cubic interp */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out = p->sr;  /* assign object data to local variables   */
    MYFLT *in = p->ain;
    MYFLT *del = p->adel;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT esr = ESR;

    if (UNLIKELY(buf==NULL)) goto err1;            /* RWD fix */
    maxd = p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }

    if (IS_ASIG_ARG(p->adel)) {              /*      if delay is a-rate      */
      for (nn=offset; nn<nsmps; nn++) {
        MYFLT  fv1;
        int32_t   v0, v1, v2, v3;

        buf[indx] = in[nn];      /* IV Oct 2001 */
        fv1 = del[nn] * (-esr);
        v1 = (int32_t)fv1;
        fv1 -= (MYFLT) v1;
        v1 += (int32_t)indx;
        /* Make sure Inside the buffer      */
        if ((v1 < 0L) || (fv1 < FL(0.0))) {
          fv1++; v1--; while (UNLIKELY(v1 < 0L)) v1 += (int32_t)maxd;
        }
        else {
          while (UNLIKELY(v1 >= (int32_t)maxd)) v1 -= (int32_t)maxd;
        }
        /* Find next sample for interpolation      */
        v2 = (v1 == (int32_t)(maxd - 1UL) ? 0L : v1 + 1L);

        if (maxd<4) {
          out[nn] = buf[v1] + fv1 * (buf[v2] - buf[v1]);
        }
        else {
          v0 = (v1==0 ? maxd-1 : v1-1);
          v3 = (v2==(int32_t)maxd-1 ? 0 : v2+1);
          {                     /* optimized by Istvan Varga (Oct 2001) */
            MYFLT w, x, y, z;
            z = fv1 * fv1; z--; z *= FL(0.1666666667);
            y = fv1; y++; w = (y *= FL(0.5)); w--;
            x = FL(3.0) * z; y -= x; w -= z; x -= fv1;
            out[nn] = (w*buf[v0] + x*buf[v1] + y*buf[v2] + z*buf[v3])
              * fv1 + buf[v1];
          }
        }
        if (UNLIKELY(++indx == maxd))
          indx = 0;             /* Advance current pointer */

      };
    }
    else {                      /* and, if delay is k-rate */
      MYFLT  fv1, w, x, y, z;
      int32_t   v0, v1, v2, v3;

      fv1 = *del * -esr; v1 = (int32_t)fv1; fv1 -= (MYFLT) v1;
      v1 += (int32_t)indx;
      /* Make sure Inside the buffer      */
      if ((v1 < 0L) || (fv1 < FL(0.0))) {
        fv1++; v1--; while (UNLIKELY(v1 < 0L)) v1 += (int32_t)maxd;
      }
      else {
        while (UNLIKELY(v1 >= (int32_t)maxd)) v1 -= (int32_t)maxd;
      }

      if (maxd<4) {
        for (nn=offset; nn<nsmps; nn++) {
          /* Find next sample for interpolation      */
          v2 = (v1 == (int32_t)(maxd - 1UL) ? 0L : v1 + 1L);
          out[nn] = buf[v1] + fv1 * (buf[v2] - buf[v1]);
          if (UNLIKELY(++v1 >= (int32_t)maxd)) v1 -= (int32_t)maxd;
              if (UNLIKELY(++indx >= maxd)) indx -= maxd;
        }
      }
      else {
        /* calculate interpolation coeffs */
        z = fv1 * fv1; z--; z *= FL(0.1666666667);      /* IV Oct 2001 */
        y = fv1; y++; w = (y *= FL(0.5)); w--;
        x = FL(3.0) * z; y -= x; w -= z; x -= fv1;
        for (nn=offset; nn<nsmps; nn++) {
          buf[indx] = in[nn];
          /* Find next sample for interpolation      */
          v2 = (v1 == (int32_t)(maxd - 1UL) ? 0L : v1 + 1L);
          v0 = (v1 == 0L ? (int32_t)(maxd - 1UL) : v1 - 1L);
          v3 = (v2 == (int32_t)(maxd - 1UL) ? 0L : v2 + 1L);
          out[nn] = (w*buf[v0] + x*buf[v1] + y*buf[v2] + z*buf[v3])
                     * fv1 + buf[v1];
          if (UNLIKELY(++v1 >= (int32_t)maxd)) v1 -= (int32_t)maxd;
          if (UNLIKELY(++indx >= maxd))
            indx -= maxd;     /* Advance current pointer */
        }
      }
    }
    p->left = indx;             /*      and keep track of where you are */
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay3: not initialised"));
}

/* vdelayx, vdelayxs, vdelayxq, vdelayxw, vdelayxws, vdelayxwq */
/* coded by Istvan Varga, Mar 2001 */

int32_t vdelxset(CSOUND *csound, VDELX *p)      /*  vdelayx set-up (1 channel) */
{
    uint32_t n = (int32_t)(*p->imaxd * csound->esr);

    if (UNLIKELY(n == 0)) n = 1;          /* fix due to Troxler */

    if (!*p->istod) {
      if (p->aux1.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux1.size)
        /* allocate space for delay buffer */
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux1);
      else
        memset(p->aux1.auxp, 0, n*sizeof(MYFLT));
      p->left = 0;
      p->interp_size = 4 * (int32_t) (FL(0.5) + FL(0.25) * *(p->iquality));
      p->interp_size = (p->interp_size < 4 ? 4 : p->interp_size);
      p->interp_size = (p->interp_size > 1024 ? 1024 : p->interp_size);
    }
    p->maxd = (uint32) n;
    return OK;
}

int32_t vdelxsset(CSOUND *csound, VDELXS *p)    /*  vdelayxs set-up (stereo) */
{
    uint32_t n = (int32_t)(*p->imaxd * csound->esr);

    if (UNLIKELY(n == 0)) n = 1;          /* fix due to Troxler */

    if (!*p->istod) {
      if (p->aux1.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux1.size)
        /* allocate space for delay buffer */
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux1);
      else
        memset(p->aux1.auxp, 0, n*sizeof(MYFLT));
      if (p->aux2.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux2.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux2);
      else
        memset(p->aux2.auxp, 0, n*sizeof(MYFLT));

      p->left = 0;
      p->interp_size = 4 * (int32_t) (FL(0.5) + FL(0.25) * *(p->iquality));
      p->interp_size = (p->interp_size < 4 ? 4 : p->interp_size);
      p->interp_size = (p->interp_size > 1024 ? 1024 : p->interp_size);
    }
    p->maxd = (uint32) n;
    return OK;
}

int32_t vdelxqset(CSOUND *csound, VDELXQ *p) /* vdelayxq set-up (quad channels) */
{
    uint32_t n = (int32_t)(*p->imaxd * csound->esr);

    if (UNLIKELY(n == 0)) n = 1;          /* fix due to Troxler */

    if (!*p->istod) {
      if (p->aux1.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux1.size)
        /* allocate space for delay buffer */
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux1);
      else
        memset(p->aux1.auxp, 0, n*sizeof(MYFLT));
      if (p->aux2.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux2.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux2);
      else
        memset(p->aux2.auxp, 0, n*sizeof(MYFLT));
      if (p->aux3.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux3.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux3);
      else
        memset(p->aux3.auxp, 0, n*sizeof(MYFLT));
      if (p->aux4.auxp == NULL || (uint32_t)(n * sizeof(MYFLT)) > p->aux4.size)
        csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aux4);
      else
        memset(p->aux4.auxp, 0, n*sizeof(MYFLT));

      p->left = 0;
      p->interp_size = 4 * (int32_t) (FL(0.5) + FL(0.25) * *(p->iquality));
      p->interp_size = (p->interp_size < 4 ? 4 : p->interp_size);
      p->interp_size = (p->interp_size > 1024 ? 1024 : p->interp_size);
    }
    p->maxd = (uint32) n;
    return OK;
}

int32_t vdelayx(CSOUND *csound, VDELX *p)               /*      vdelayx routine  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t indx, maxd;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *in1 = p->ain1;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1;
    int32_t   i, i2, xpos;

    if (UNLIKELY(buf1 == NULL)) goto err1;                          /* RWD fix */
    maxd = p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);
    if (UNLIKELY(offset)) memset(out1, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (nn=offset; nn<nsmps; nn++) {
      buf1[indx] = in1[nn];
      n1 = 0.0;

      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx - ((double)del[nn] * (double)csound->esr);
      while (x1 < 0.0) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (xpos >= maxd) xpos -= maxd;

      if (x1 * (1.0 - x1) > 0.00000001) {
        xpos += (1 - i2);
        while (xpos < 0) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 += (double)buf1[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 -= (double)buf1[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
        out1[nn] = (MYFLT) (n1 * x2);
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);      /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        out1[nn] = buf1[xpos];
      }

      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelayxw(CSOUND *csound, VDELX *p)      /*      vdelayxw routine  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t nn, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *in1 = p->ain1;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1;
    int32_t   i, i2, xpos;

    if (UNLIKELY(buf1 == NULL)) goto err1;                          /* RWD fix */
    maxd =  p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);

    if (UNLIKELY(offset)) memset(out1, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (nn=offset;nn<nsmps;nn++) {
      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx + ((double)del[nn] * (double)csound->esr);
      while (x1 < 0.0) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (xpos >= maxd) xpos -= maxd;

      if (LIKELY(x1 * (1.0 - x1) > 0.00000001)) {
        n1 = (double)in1[nn] * x2;
        xpos += (1 - i2);
        while (xpos < 0) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] += (MYFLT) (n1 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] -= (MYFLT) (n1 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);      /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        buf1[xpos] += in1[nn];
      }

      out1[nn] = buf1[indx]; buf1[indx] = FL(0.0);
      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelayxs(CSOUND *csound, VDELXS *p)     /*      vdelayxs routine  */
{
    int32_t  maxd, indx;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *out2 = p->sr2;
    MYFLT *in1 = p->ain1;
    MYFLT *in2 = p->ain2;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT *buf2 = (MYFLT *)p->aux2.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1, n2;
    int32_t   i, i2, xpos;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY((buf1 == NULL) || (buf2 == NULL))) goto err1; /* RWD fix */
    maxd =  p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);
    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      buf1[indx] = in1[n]; buf2[indx] = in2[n];
      n1 = 0.0; n2 = 0.0;

      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx - ((double)del[n] * (double)csound->esr);
      while (UNLIKELY(x1 < 0.0)) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (UNLIKELY(xpos >= maxd)) xpos -= maxd;

      if (x1 * (1.0 - x1) > 0.00000001) {
        xpos += (1 - i2);
        while (UNLIKELY(xpos < 0)) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 += (double)buf1[xpos] * w; n2 += (double)buf2[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 -= (double)buf1[xpos] * w; n2 -= (double)buf2[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
        out1[n] = (MYFLT) (n1 * x2); out2[n] = (MYFLT) (n2 * x2);
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);      /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        out1[n] = buf1[xpos]; out2[n] = buf2[xpos];
      }

      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelayxws(CSOUND *csound, VDELXS *p)    /*      vdelayxws routine  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *out2 = p->sr2;
    MYFLT *in1 = p->ain1;
    MYFLT *in2 = p->ain2;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT *buf2 = (MYFLT *)p->aux2.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1, n2;
    int32_t   i, i2, xpos;

    if (UNLIKELY((buf1 == NULL) || (buf2 == NULL))) goto err1;     /* RWD fix */
    maxd =  p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx + ((double)del[n] * (double)csound->esr);
      while (UNLIKELY(x1 < 0.0)) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (UNLIKELY(xpos >= maxd)) xpos -= maxd;

      if (x1 * (1.0 - x1) > 0.00000001) {
        n1 = (double)in1[n] * x2; n2 = (double)in2[n] * x2;
        xpos += (1 - i2);
        while (UNLIKELY(xpos < 0)) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] += (MYFLT) (n1 * w); buf2[xpos] += (MYFLT) (n2 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] -= (MYFLT) (n1 * w); buf2[xpos] -= (MYFLT) (n2 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);       /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        buf1[xpos] += in1[n]; buf2[xpos] += in2[n];
      }

      out1[n] = buf1[indx]; buf1[indx] = FL(0.0);
      out2[n] = buf2[indx]; buf2[indx] = FL(0.0);
      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelayxq(CSOUND *csound, VDELXQ *p)     /*      vdelayxq routine  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *out2 = p->sr2;
    MYFLT *out3 = p->sr3;
    MYFLT *out4 = p->sr4;
    MYFLT *in1 = p->ain1;
    MYFLT *in2 = p->ain2;
    MYFLT *in3 = p->ain3;
    MYFLT *in4 = p->ain4;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT *buf2 = (MYFLT *)p->aux2.auxp;
    MYFLT *buf3 = (MYFLT *)p->aux3.auxp;
    MYFLT *buf4 = (MYFLT *)p->aux4.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1, n2, n3, n4;
    int32_t   i, i2, xpos;
    /* RWD fix */
    if (UNLIKELY((buf1 == NULL) || (buf2 == NULL) ||
                 (buf3 == NULL) || (buf4 == NULL))) goto err1;
    maxd =  p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
      memset(out3, '\0', offset*sizeof(MYFLT));
      memset(out4, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out4[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      buf1[indx] = in1[n]; buf2[indx] = in2[n];
      buf3[indx] = in3[n]; buf4[indx] = in4[n];
      n1 = 0.0; n2 = 0.0; n3 = 0.0; n4 = 0.0;

      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx - ((double)*del++ * (double)csound->esr);
      while (UNLIKELY(x1 < 0.0)) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (UNLIKELY(xpos >= maxd)) xpos -= maxd;

      if (LIKELY(x1 * (1.0 - x1) > 0.00000001)) {
        xpos += (1 - i2);
        while (UNLIKELY(xpos < 0)) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 += (double)buf1[xpos] * w; n2 += (double)buf2[xpos] * w;
          n3 += (double)buf3[xpos] * w; n4 += (double)buf4[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          n1 -= (double)buf1[xpos] * w; n2 -= (double)buf2[xpos] * w;
          n3 -= (double)buf3[xpos] * w; n4 -= (double)buf4[xpos] * w;
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
        out1[n] = (MYFLT) (n1 * x2); out2[n] = (MYFLT) (n2 * x2);
        out3[n] = (MYFLT) (n3 * x2); out4[n] = (MYFLT) (n4 * x2);
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);       /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        out1[n] = buf1[xpos]; out2[n] = buf2[xpos];
        out3[n] = buf3[xpos]; out4[n] = buf4[xpos];
      }

      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t vdelayxwq(CSOUND *csound, VDELXQ *p)    /*      vdelayxwq routine  */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t  maxd, indx;
    MYFLT *out1 = p->sr1;  /* assign object data to local variables   */
    MYFLT *out2 = p->sr2;
    MYFLT *out3 = p->sr3;
    MYFLT *out4 = p->sr4;
    MYFLT *in1 = p->ain1;
    MYFLT *in2 = p->ain2;
    MYFLT *in3 = p->ain3;
    MYFLT *in4 = p->ain4;
    MYFLT *del = p->adel;
    MYFLT *buf1 = (MYFLT *)p->aux1.auxp;
    MYFLT *buf2 = (MYFLT *)p->aux2.auxp;
    MYFLT *buf3 = (MYFLT *)p->aux3.auxp;
    MYFLT *buf4 = (MYFLT *)p->aux4.auxp;
    int32_t   wsize = p->interp_size;
    double x1, x2, w, d, d2x, n1, n2, n3, n4;
    int32_t   i, i2, xpos;
    /* RWD fix */
    if (UNLIKELY((buf1 == NULL) || (buf2 == NULL) ||
                 (buf3 == NULL) || (buf4 == NULL))) goto err1;
    maxd =  p->maxd;
    if (UNLIKELY(maxd == 0)) maxd = 1;    /* Degenerate case */
    indx = p->left;
    i2 = (wsize >> 1);
    d2x = (1.0 - pow ((double)wsize * 0.85172, -0.89624)) / (double)(i2 * i2);

    if (UNLIKELY(offset)) {
      memset(out1, '\0', offset*sizeof(MYFLT));
      memset(out2, '\0', offset*sizeof(MYFLT));
      memset(out3, '\0', offset*sizeof(MYFLT));
      memset(out4, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out3[nsmps], '\0', early*sizeof(MYFLT));
      memset(&out4[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset; n<nsmps; n++) {
      /* x1: fractional part of delay time */
      /* x2: sine of x1 (for interpolation) */
      /* xpos: integer part of delay time (buffer position to read from) */

      x1 = (double)indx + ((double)del[n] * (double)csound->esr);
      while (UNLIKELY(x1 < 0.0)) x1 += (double)maxd;
      xpos = (int32_t)x1;
      x1 -= (double)xpos;
      x2 = sin (PI * x1) / PI;
      while (UNLIKELY(xpos >= maxd)) xpos -= maxd;

      if (x1 * (1.0 - x1) > 0.00000001) {
        n1 = (double)in1[n] * x2; n2 = (double)in2[n] * x2;
        n3 = (double)in3[n] * x2; n4 = (double)in4[n] * x2;
        xpos += (1 - i2);
        while (UNLIKELY(xpos < 0)) xpos += maxd;
        d = (double)(1 - i2) - x1;
        for (i = i2; i--;) {
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] += (MYFLT) (n1 * w); buf2[xpos] += (MYFLT) (n2 * w);
          buf3[xpos] += (MYFLT) (n3 * w); buf4[xpos] += (MYFLT) (n4 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
          w = 1.0 - d*d*d2x; w *= (w / d++);
          buf1[xpos] -= (MYFLT) (n1 * w); buf2[xpos] -= (MYFLT) (n2 * w);
          buf3[xpos] -= (MYFLT) (n3 * w); buf4[xpos] -= (MYFLT) (n4 * w);
          if (UNLIKELY(++xpos >= maxd)) xpos -= maxd;
        }
      }
      else {                                            /* integer sample */
        xpos = (int32_t)((double)xpos + x1 + 0.5);       /* position */
        if (UNLIKELY(xpos >= maxd)) xpos -= maxd;
        buf1[xpos] += in1[n]; buf2[xpos] += in2[n];
        buf3[xpos] += in3[n]; buf4[xpos] += in4[n];
      }

      out1[n] = buf1[indx]; buf1[indx] = FL(0.0);
      out2[n] = buf2[indx]; buf2[indx] = FL(0.0);
      out3[n] = buf3[indx]; buf3[indx] = FL(0.0);
      out4[n] = buf4[indx]; buf4[indx] = FL(0.0);
      if (UNLIKELY(++indx == maxd)) indx = 0;
    }

    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("vdelay: not initialised"));
}

int32_t multitap_set(CSOUND *csound, MDEL *p)
{
    uint32_t n, i;
    MYFLT max = FL(0.0);

    //if (UNLIKELY(p->INOCOUNT/2 == (MYFLT)p->INOCOUNT*FL(0.5)))
    /* Should this test just be p->INOCOUNT&1 ==  */
    if (UNLIKELY((p->INOCOUNT&1)==0))
      return csound->InitError(csound, Str("Wrong input count in multitap\n"));

    for (i = 0; i < p->INOCOUNT - 1; i += 2) {
      if (max < *p->ndel[i]) max = *p->ndel[i];
    }

    n = (uint32_t)(csound->esr * max * sizeof(MYFLT));
    if (p->aux.auxp == NULL ||    /* allocate space for delay buffer */
        n > p->aux.size)
      csound->AuxAlloc(csound, n, &p->aux);
    else {
      memset(p->aux.auxp, 0, n);
    }

    p->left = 0;
    p->max = (int32_t)(csound->esr * max);
    return OK;
}

int32_t multitap_play(CSOUND *csound, MDEL *p)
{                               /* assign object data to local variables   */
    int32_t  indx = p->left, delay;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, n, nsmps = CS_KSMPS;
    MYFLT *out = p->sr, *in = p->ain;
    MYFLT *buf = (MYFLT *)p->aux.auxp;
    MYFLT max = (MYFLT)p->max;

    if (UNLIKELY(buf==NULL)) goto err1;           /* RWD fix */
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT v = FL(0.0);
      buf[indx] = in[n];        /*      Write input     */

      if (UNLIKELY(++indx == max)) indx = 0;   /*      Advance input pointer   */
      for (i = 0; i < p->INOCOUNT - 1; i += 2) {
        delay = indx - (int32_t)(csound->esr * *p->ndel[i]);
        if (UNLIKELY(delay < 0))
          delay += (int32_t)max;
        v += buf[delay] * *p->ndel[i+1]; /*      Write output    */
      }
      out[n] = v;
    }
    p->left = indx;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("multitap: not initialised"));
}

/*      nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998 */

#define LOG001  (-6.9077552789821370521)       /* log(.001) */

static const int32_t smallprime[] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
  67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137,
  139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
  223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283,
  293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379,
  383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461,
  463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563,
  569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643,
  647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739,
  743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829,
  839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937,
  941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021,
  1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097,
  1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193,
  1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283,
  1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
  1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459,
  1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549,
  1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619,
  1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721,
  1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
  1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907,
  1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003,
  2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083, 2087, 2089,
  2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203,
  2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287,
  2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377,
  2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423, 2437, 2441, 2447, 2459,
  2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579,
  2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677,
  2683, 2687, 2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741,
  2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837,
  2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903, 2909, 2917, 2927, 2939,
  2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 3037, 3041,
  3049, 3061, 3067, 3079, 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167,
  3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257,
  3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347,
  3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461,
  3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541,
  3547, 3557, 3559, 3571};

static int32_t prime(int32_t val)
{
    int32_t i, last;
    if (val < 3572) {
      for (i = 0; smallprime[i] < val; i++)
        ;
      return (smallprime[i] == val ? 1 : 0);
    }
    last = (int32_t) sqrt((double)val);
    for (i = 0; smallprime[i] < (last < 3572 ? last : 3572); i++) {
      if (UNLIKELY((val % smallprime[i]) == 0))
        return 0;
    }
    for (i = 3573; i <= last; i += 2) {
      if (UNLIKELY((val % i) == 0))
        return 0;
    }
    return 1;
}

/*
 * Based on nreverb coded by Paris Smaragdis 1994 and Richard Karpen 1998.
 * Changes made to allow user-defined comb and alpas constant in a ftable.
 * Sept 2000, by rasmus ekman.
 * Memory allocation fixed April 2001 by JPff
 *
 */

/* The original reverb2 constants were in samples at sample rate 25641.0
 * (suggestedly being "probably CCRMA 'samson box' sampling-rate").
 * Used to be defined in single-purpose header revsets.h.
 * Get rid of this right here so we can use them as times in the code.
 */

#define orgCombs 6
#define orgAlpas 5

static const MYFLT cc_time[orgCombs] = {
    FL(0.055887056)     /* 1433.0 / 25641.0 */,
    FL(0.062439062)     /* 1601.0 / 25641.0 */,
    FL(0.072813073)     /* 1867.0 / 25641.0 */,
    FL(0.080067080)     /* 2053.0 / 25641.0 */,
    FL(0.087789088)     /* 2251.0 / 25641.0 */,
    FL(0.093561094)     /* 2399.0 / 25641.0 */
};

static const MYFLT cc_gain[orgCombs] = {
    FL(0.822), FL(0.802), FL(0.773), FL(0.753), FL(0.753), FL(0.753)
};

static const MYFLT ca_time[orgAlpas] = {
    FL(0.013533014)     /*  347.0 / 25641.0 */,
    FL(0.0044070044)    /*  113.0 / 25641.0 */,
    FL(0.0014430014)    /*   37.0 / 25641.0 */,
    FL(0.0023010023)    /*   59.0 / 25641.0 */,
    FL(0.0016770017)    /*   43.0 / 25641.0 */
};

static const MYFLT ca_gain[orgAlpas] = {
    FL(0.7), FL(0.7), FL(0.7), FL(0.7), FL(0.7)
};

int32_t reverbx_set(CSOUND *csound, NREV2 *p)
{
    int32_t  i, n;
    /* Temp holder of old or user constants. */
    const MYFLT *c_orgtime, *a_orgtime;
    int32_t   c_time, a_time;
    int32_t   cmbAllocSize, alpAllocSize;

    if (UNLIKELY(*p->hdif > FL(1.0) || *p->hdif < FL(0.0)))
      return
        csound->InitError(csound, Str("High frequency diffusion not in (0, 1)\n"));

    /* Init comb constants and allocate dynamised work space */
    if (*p->inumCombs < FL(1.0)) {  /* Using old defaults */
      /* Get nreverb defaults */
      p->numCombs = orgCombs;
      c_orgtime = cc_time;
      p->c_orggains = cc_gain;
    }
    else {                          /* User provided constants */
      FUNC *ftCombs;
      p->numCombs = (int32_t) *p->inumCombs;
      /* Get user-defined set of comb constants from table */
      if (UNLIKELY((ftCombs = csound->FTnp2Finde(csound, p->ifnCombs)) == NULL))
        return NOTOK;
      if (UNLIKELY(ftCombs->flen < (uint32_t)p->numCombs * 2)) {
        return csound->InitError(csound, Str("reverbx; Combs ftable must have "
                                             "%d time and %d gain values"),
                                 p->numCombs, p->numCombs);
      }
      c_orgtime = ftCombs->ftable;
      p->c_orggains = (ftCombs->ftable + p->numCombs);
    }
    /* Alloc a single block and get arrays of comb pointers from that */
    cmbAllocSize = p->numCombs * sizeof(MYFLT);
    csound->AuxAlloc(csound,
                     4 * cmbAllocSize + 2 * (p->numCombs + 1) * sizeof(MYFLT*),
                     &p->caux2);
    p->c_time = (MYFLT*) p->caux2.auxp;
    p->c_gain = (MYFLT*) ((char*) p->caux2.auxp + 1 * cmbAllocSize);
    p->z = (MYFLT*) ((char*) p->caux2.auxp + 2 * cmbAllocSize);
    p->g = (MYFLT*) ((char*) p->caux2.auxp + 3 * cmbAllocSize);
    p->cbuf_cur = (MYFLT**) ((char*) p->caux2.auxp + 4 * cmbAllocSize);
    p->pcbuf_cur = p->cbuf_cur + (p->numCombs + 1);

    /* ...and allpass constants and allocs */
    if (*p->inumAlpas < FL(1.0)) {
      /* Get nreverb defaults */
      p->numAlpas = orgAlpas;
      a_orgtime = ca_time;
      p->a_orggains = ca_gain;
    }
    else {    /* Have user-defined set of alpas constants */
      FUNC *ftAlpas;
      p->numAlpas = (int32_t) *p->inumAlpas;
      if (UNLIKELY((ftAlpas = csound->FTnp2Finde(csound, p->ifnAlpas)) == NULL))
        return NOTOK;
      if (UNLIKELY(ftAlpas->flen < (uint32_t)p->numAlpas * 2)) {
        return csound->InitError(csound, Str("reverbx; Alpas ftable must have"
                                             " %d time and %d gain values"),
                                         p->numAlpas, p->numAlpas);
      }
      a_orgtime = ftAlpas->ftable;
      p->a_orggains = (ftAlpas->ftable + p->numAlpas);
    }
    /* Dynamic alloc of alpass space */
    alpAllocSize = p->numAlpas * sizeof(MYFLT);
    csound->AuxAlloc(csound,
                     2 * alpAllocSize + 2 * (p->numAlpas + 1) * sizeof(MYFLT*),
                     &p->aaux2);
    p->a_time = (MYFLT*) p->aaux2.auxp;
    p->a_gain = (MYFLT*) ((char*) p->aaux2.auxp + 1 * alpAllocSize);
    p->abuf_cur = (MYFLT**) ((char*) p->aaux2.auxp + 2 * alpAllocSize);
    p->pabuf_cur = (MYFLT**) ((char*) p->aaux2.auxp + 2 * alpAllocSize
                              + (p->numAlpas + 1) * sizeof(MYFLT*));

    /* Init variables */
    if (*p->istor == FL(0.0) ||
        p->temp.auxp == NULL || p->temp.size<CS_KSMPS * sizeof(MYFLT)) {
      csound->AuxAlloc(csound, CS_KSMPS * sizeof(MYFLT), &p->temp);

      n = 0;
      for (i = 0; i < p->numCombs; i++) {
        MYFLT ftime = c_orgtime[i];
        /* Use directly as num samples if negative */
        if (ftime < FL(0.0))
          c_time = (int32_t) -ftime;
        else {
          /* convert from to seconds to samples, and make prime */
          c_time = (int32_t) (ftime * csound->esr);
          /* Mangle sample number to primes. */
          if (c_time % 2 == 0)
            c_time += 1;
          while (!prime(c_time))
            c_time += 2;
        }
        p->c_time[i] = (MYFLT) c_time;
        n += c_time;
        p->c_gain[i] = (MYFLT) exp((double)(LOG001 * (p->c_time[i]
                                                       * csound->onedsr)
                                             / (p->c_orggains[i] * *p->time)));
        p->g[i] = *p->hdif;
        p->c_gain[i] = p->c_gain[i] * (FL(1.0) - p->g[i]);
        p->z[i] = FL(0.0);
      }
      csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->caux);
      /* unnecessary as Auxlloc clears to 0 */
      /* for (i = 0; i < n; i++) { */
      /*   ((MYFLT*) p->caux.auxp)[i] = FL(0.0); */
      /* } */
      p->pcbuf_cur[0] = p->cbuf_cur[0] = (MYFLT*)p->caux.auxp;
      for (i = 0; i < p->numCombs; i++) {
        p->pcbuf_cur[i + 1] = p->cbuf_cur[i + 1] =
          p->cbuf_cur[i] + (int32_t) p->c_time[i];
        p->c_time[i] *= csound->onedsr; /* Scale to save division in reverbx */
      }
      n = 0;
      for (i = 0; i < p->numAlpas; i++) {
        MYFLT ftime = a_orgtime[i];
        if (ftime < FL(0.0))
          a_time = (int32_t) -ftime;
        else {
          /* convert seconds to samples and make prime */
          a_time = (int32_t) (ftime * csound->esr);
          if (a_time % 2 == 0)
            a_time += 1;
          while (!prime(a_time))
            a_time += 2;
        }
        p->a_time[i] = (MYFLT) a_time;
        p->a_gain[i] = (MYFLT) exp((double)(LOG001 * (p->a_time[i]
                                                       * csound->onedsr)
                                             / (p->a_orggains[i] * *p->time)));
        n += a_time;
      }
      csound->AuxAlloc(csound, n * sizeof(MYFLT), &p->aaux);
      /* unnecessary as AuxAlloc clears */
      /* for (i = 0; i < n; i++) { */
      /*   ((MYFLT*) p->aaux.auxp)[i] = FL(0.0); */
      /* } */
      p->pabuf_cur[0] = p->abuf_cur[0] = (MYFLT*) p->aaux.auxp;
      for (i = 0; i < p->numAlpas; i++) {
        p->pabuf_cur[i + 1] = p->abuf_cur[i + 1] =
          p->abuf_cur[i] + (int32_t) p->a_time[i];
        p->a_time[i] *= csound->onedsr; /* Scale to save division in reverbx */
      }
    }

    p->prev_time = *p->time;
    p->prev_hdif = *p->hdif;

    return OK;
}

int32_t reverbx(CSOUND *csound, NREV2 *p)
{
    int32_t i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *in, *out = p->out, *buf, *end;
    MYFLT   gain, z;
    MYFLT   hdif = *p->hdif;
    MYFLT   time = *p->time;
    int32_t     numCombs = p->numCombs;
    int32_t     numAlpas = p->numAlpas;

    if (UNLIKELY(p->temp.auxp == NULL)) goto err1;
    buf = (MYFLT*) p->temp.auxp;
    in = p->in;
    memcpy(buf, in, nsmps*sizeof(MYFLT));
    memset(out, 0,  nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;
    if (*p->time != p->prev_time || *p->hdif != p->prev_hdif) {
      if (UNLIKELY(hdif > FL(1.0))) {
        csound->Warning(csound, Str("High frequency diffusion>1\n"));
        hdif = FL(1.0);
      }
      if (UNLIKELY(hdif < FL(0.0))) {
        csound->Warning(csound, Str("High frequency diffusion<0\n"));
        hdif = FL(0.0);
      }
      if (UNLIKELY(time <= FL(0.0))) {
        csound->Warning(csound, Str("Non positive reverb time\n"));
        time = FL(0.001);
      }
      for (i = 0; i < numCombs; i++) {
        p->c_gain[i] = EXP((LOG001 * p->c_time[i] /
                            (p->c_orggains[i] * time)));
        p->g[i] = hdif;
        p->c_gain[i] = p->c_gain[i] * (FL(1.0) - p->g[i]);
        p->z[i] = FL(0.0);
      }

      for (i = 0; i < numAlpas; i++)
        p->a_gain[i] = EXP((LOG001 * p->a_time[i] /
                            (p->a_orggains[i] * time)));

      p->prev_time = time;
      p->prev_hdif = hdif;
    }

    for (i = 0; i < numCombs; i++) {
      buf = p->pcbuf_cur[i];
      end = p->cbuf_cur[i + 1];
      gain = p->c_gain[i];
      in = (MYFLT*) p->temp.auxp;
      out = p->out;
      for (n=offset;n<nsmps;n++) {
        out[n] += *buf;
        *buf += p->z[i] * p->g[i];
        p->z[i] = *buf;
        *buf *= gain;
        *buf += in[n];
        if (UNLIKELY(++buf >= end))
          buf = (MYFLT*) p->cbuf_cur[i];
      }
      p->pcbuf_cur[i] = buf;
    }

    for (i = 0; i < numAlpas; i++) {
      in = (MYFLT*) p->temp.auxp;
      out = p->out;
      memcpy(in+offset, out+offset, (nsmps-offset)*sizeof(MYFLT));
      buf = p->pabuf_cur[i];
      end = p->abuf_cur[i + 1];
      gain = p->a_gain[i];
      for (n=offset;n<nsmps;n++) {
        z = *buf;
        *buf = gain * z + in[n];
        out[n] = z - gain * *buf;
        if (UNLIKELY(++buf >= end))
          buf = (MYFLT*) p->abuf_cur[i];
      }
      p->pabuf_cur[i] = buf;
    }

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("reverbx: not initialised"));
}

