/*
    ugmoss.c:

    Copyright (C) 2001 William 'Pete' Moss

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

                                                        /* ugmoss.c */
#include "stdopcod.h"
#include "ugmoss.h"
#include "aops.h"
#include <math.h>

/******************************************************************************
  all this code was written by william 'pete' moss. <petemoss@petemoss.org>
  no copyright, since it seems silly to copyright algorithms.
  do what you want with the code, and credit me if you get the chance
******************************************************************************/

/* rewritten code for dconv, includes speedup tip from
   Moore: Elements of Computer Music */
static int dconvset(CSOUND *csound, DCONV *p)
{
    FUNC *ftp;

    p->len = (int)*p->isize;
    if (LIKELY((ftp = csound->FTnp2Find(csound,
                                        p->ifn)) != NULL)) {   /* find table */
      p->ftp = ftp;
      if ((unsigned)ftp->flen < p->len)
        p->len = ftp->flen; /* correct len if flen shorter */
    }
    else {
      return csound->InitError(csound, Str("No table for dconv"));
    }
    if (p->sigbuf.auxp == NULL ||
        p->sigbuf.size < (unsigned int)(p->len*sizeof(MYFLT)))
      csound->AuxAlloc(csound, p->len*sizeof(MYFLT), &p->sigbuf);
    else
      memset(p->sigbuf.auxp, '\0', p->len*sizeof(MYFLT));
    p->curp = (MYFLT *)p->sigbuf.auxp;
    return OK;
}

static int dconv(CSOUND *csound, DCONV *p)
{
    int32 i = 0;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32 len = p->len;
    MYFLT *ar, *ain, *ftp, *startp, *endp, *curp;
    MYFLT sum;

    ain = p->ain;                               /* read saved values */
    ar = p->ar;
    ftp = p->ftp->ftable;
    startp = (MYFLT *) p->sigbuf.auxp;
    endp = startp + len;
    curp = p->curp;

    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      *curp = ain[n];                           /* get next input sample */
      i = 1, sum = *curp++ * ftp[0];
      while (curp<endp)
        sum += (*curp++ * ftp[i++]);            /* start the convolution */
      curp = startp;                            /* correct the ptr */
      while (i<len)
        sum += (*curp++ * ftp[i++]);            /* finish the convolution */
      if (--curp < startp)
        curp += len;                            /* correct for last curp++ */
      ar[n] = sum;
    }

    p->curp = curp;                             /* save state */
    return OK;
}

static int and_kk(CSOUND *csound, AOP *p)
{
    int32 input1 = MYFLT2LRND(*p->a);
    int32 input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 & input2);
    return OK;
}

static int and_aa(CSOUND *csound, AOP *p)
{
    MYFLT *r    = p->r;
    MYFLT *in1  = p->a;
    MYFLT *in2  = p->b;
    int   n, nsmps = CS_KSMPS;
    int32  input1, input2;

    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT) (input1 & input2);
    }
    return OK;
}

static int and_ak(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2 = MYFLT2LRND(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int and_ka(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2, input1 = MYFLT2LRND(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 & input2);
    }
    return OK;
}

static int or_kk(CSOUND *csound, AOP *p)
{
    int32 input1 = MYFLT2LRND(*p->a);
    int32 input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 | input2);
    return OK;
}

static int or_aa(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2, input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int or_ak(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2 = MYFLT2LRND(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int or_ka(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2, input1 = MYFLT2LRND(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 | input2);
    }
    return OK;
}

static int xor_kk(CSOUND *csound, AOP *p)
{
    int32 input1 = MYFLT2LRND(*p->a);
    int32 input2 = MYFLT2LRND(*p->b);
    *p->r = (MYFLT)(input1 ^ input2);
    return OK;
}

static int xor_aa(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2, input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = 0; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int xor_ak(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2 = MYFLT2LRND(*p->b), input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int xor_ka(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in2 = p->b;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input2, input1 = MYFLT2LRND(*p->a);

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND(in2[n]);
      r[n] = (MYFLT)(input1 ^ input2);
    }
    return OK;
}

static int shift_left_kk(CSOUND *csound, AOP *p)
{
    int32 input1 = MYFLT2LRND(*p->a);
    int  input2 = (int) MYFLT2LRND(*p->b);
    *p->r = (MYFLT) (input1 << input2);
    return OK;
}

static int shift_left_aa(CSOUND *csound, AOP *p)
{
    int32  input1, input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      input2 = (int32) MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_left_ak(CSOUND *csound, AOP *p)
{
    int32  input1;
    int   input2 = MYFLT2LRND(*p->b);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_left_ka(CSOUND *csound, AOP *p)
{
    int32  input1 = MYFLT2LRND(*p->a), input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 << input2);
    }
    return OK;
}

static int shift_right_kk(CSOUND *csound, AOP *p)
{
    int32 input1 = MYFLT2LRND(*p->a);
    int  input2 = (int) MYFLT2LRND(*p->b);
    *p->r = (MYFLT) (input1 >> input2);
    return OK;
}

static int shift_right_aa(CSOUND *csound, AOP *p)
{
    int32  input1, input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      input2 = (int) MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int shift_right_ak(CSOUND *csound, AOP *p)
{
    int32  input1;
    int32   input2 = MYFLT2LRND(*p->b);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(p->a[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int shift_right_ka(CSOUND *csound, AOP *p)
{
    int32  input1 = MYFLT2LRND(*p->a), input2;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset; n < nsmps; n++) {
      input2 = MYFLT2LRND(p->b[n]);
      p->r[n] = (MYFLT) (input1 >> input2);
    }
    return OK;
}

static int not_k(CSOUND *csound, AOP *p)    /* Added for completeness by JPff */
{
    int32 input1 = MYFLT2LRND(*p->a);
    *p->r = (MYFLT)(~input1);
    return OK;
}

static int not_a(CSOUND *csound, AOP *p)
{
    MYFLT *r = p->r;
    MYFLT *in1 = p->a;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32  input1;

    if (UNLIKELY(offset)) memset(r, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&r[nsmps], '\0', early*sizeof(MYFLT));
    }
     for (n = offset; n < nsmps; n++) {
      input1 = MYFLT2LRND(in1[n]);
      r[n] = (MYFLT)(~input1);
    }
    return OK;
}

/* all the vcomb and valpass stuff adapted from comb() and alpass()
   with additional insight from me (petemoss@petemoss.org)  */

static int vcombset(CSOUND *csound, VCOMB *p)
{
    int32        lpsiz, nbytes;

    if (*p->insmps != FL(0.0)) {
      if (UNLIKELY((lpsiz = MYFLT2LONG(*p->imaxlpt)) <= 0)) {
        return csound->InitError(csound, Str("illegal loop time"));
      }
    }
    else if (UNLIKELY((lpsiz = (int32)(*p->imaxlpt * CS_ESR)) <= 0)) {
      return csound->InitError(csound, Str("illegal loop time"));
    }
    nbytes = lpsiz * sizeof(MYFLT);
    if (p->auxch.auxp == NULL || nbytes != (int)p->auxch.size) {
      csound->AuxAlloc(csound, (size_t)nbytes, &p->auxch);
      p->pntr = (MYFLT *) p->auxch.auxp;
      if (UNLIKELY(p->pntr==NULL)) {
        return csound->InitError(csound, Str("could not allocate memory"));
      }
    }
    else if (!(*p->istor)) {
      int32 *fp = (int32 *) p->auxch.auxp;
      p->pntr = (MYFLT *) fp;
      memset(p->pntr, 0, nbytes);
      /* do   /\* Seems to assume sizeof(int32)=sizeof(MYFLT) *\/ */
      /*   *fp++ = 0; */
      /* while (--lpsiz); */
    }
    p->rvt = FL(0.0);
    p->lpt = FL(0.0);
    p->g   = FL(0.0);
    p->lpta = IS_ASIG_ARG(p->xlpt) ? 1 : 0;
    if (*p->insmps == 0) p->maxlpt = *p->imaxlpt * CS_ESR;
    else p->maxlpt = *p->imaxlpt;
    return OK;
}

static int vcomb(CSOUND *csound, VCOMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    uint32      xlpt, maxlpt = (uint32)p->maxlpt;
    MYFLT       *ar, *asig, *rp, *endp, *startp, *wp, *lpt;
    MYFLT       g = p->g;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->lpta) {                               /* if xlpt is a-rate */
      lpt = p->xlpt;
      for (n=offset; n<nsmps; n++) {
        xlpt = (uint32)((*p->insmps != 0) ? *lpt : *lpt * CS_ESR);
        if (xlpt > maxlpt) xlpt = maxlpt;
        if ((rp = wp - xlpt) < startp) rp += maxlpt;
        if ((p->rvt != *p->krvt) || (p->lpt != *lpt)) {
          p->rvt = *p->krvt, p->lpt = *lpt;
          g = p->g = POWER(FL(0.001), (p->lpt / p->rvt));
        }
        lpt++;
        ar[n] = *rp++;
        *wp++ = (ar[n] * g) + asig[n];
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    else {                                       /* if xlpt is k-rate */
      xlpt = (uint32) ((*p->insmps != 0) ? *p->xlpt
                                                  : *p->xlpt * CS_ESR);
      if (xlpt > maxlpt) xlpt = maxlpt;
      if ((rp = wp - xlpt) < startp) rp += maxlpt;
      if ((p->rvt != *p->krvt) || (p->lpt != *p->xlpt)) {
        p->rvt = *p->krvt, p->lpt = *p->xlpt;
        g = p->g = POWER(FL(0.001), (p->lpt / p->rvt));
      }
      for (n=offset; n<nsmps; n++) {
        ar[n] = *rp++;
        *wp++ = (ar[n] * g) + asig[n];
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    p->pntr = wp;
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("vcomb: not initialised"));
}

static int valpass(CSOUND *csound, VCOMB *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    uint32 xlpt, maxlpt = (uint32)p->maxlpt;
    MYFLT       *ar, *asig, *rp, *startp, *endp, *wp, *lpt;
    MYFLT       y, z, g = p->g;

    if (UNLIKELY(p->auxch.auxp==NULL)) goto err1;
    ar = p->ar;
    asig = p->asig;
    endp = (MYFLT *) p->auxch.endp;
    startp = (MYFLT *) p->auxch.auxp;
    wp = p->pntr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->lpta) {                                      /* if xlpt is a-rate */
      lpt = p->xlpt;
      for (n=offset; n<nsmps; n++) {
        xlpt = (uint32)((*p->insmps != 0) ? lpt[n] : lpt[n] * CS_ESR);
        if (xlpt > maxlpt) xlpt = maxlpt;
        if ((rp = wp - xlpt) < startp) rp += maxlpt;
        if ((p->rvt != *p->krvt) || (p->lpt != lpt[n])) {
          p->rvt = *p->krvt, p->lpt = lpt[n];
          g = p->g = POWER(FL(0.001), (p->lpt / p->rvt));
        }
        y = *rp++;
        *wp++ = z = y * g + asig[n];
        ar[n] = y - g * z;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    else {                                              /* if xlpt is k-rate */
      xlpt = (uint32) ((*p->insmps != 0) ? *p->xlpt
                                         : *p->xlpt * CS_ESR);
      if (xlpt > maxlpt) xlpt = maxlpt;
      if ((rp = wp - xlpt) < startp) rp += maxlpt;
      if ((p->rvt != *p->krvt) || (p->lpt != *p->xlpt)) {
        p->rvt = *p->krvt, p->lpt = *p->xlpt;
        g = p->g = POWER(FL(0.001), (p->lpt / p->rvt));
      }
      for (n=offset; n<nsmps; n++) {
        y = *rp++;
        *wp++ = z = y * g + asig[n];
        ar[n] = y - g * z;
        if (wp >= endp) wp = startp;
        if (rp >= endp) rp = startp;
      }
    }
    p->pntr = wp;
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("valpass: not initialised"));
}

static int ftmorfset(CSOUND *csound, FTMORF *p)
{
    FUNC *ftp;
    int j = 0;
    unsigned int len;
    /* make sure resfn exists and set it up */
    if (LIKELY((ftp = csound->FTnp2Find(csound, p->iresfn)) != NULL)) {
      p->resfn = ftp, len = p->resfn->flen;
    }
    else {
      return csound->InitError(csound, Str("iresfn for ftmorf does not exist"));
    }
    /* make sure ftfn exists and set it up */
    if (LIKELY((ftp = csound->FTnp2Find(csound, p->iftfn)) != NULL)) {
      p->ftfn = ftp;
    }
    else {
      return csound->InitError(csound, Str("iftfn for ftmorf does not exist"));
    }

    do {                /* make sure tables in ftfn exist and are right size*/
      if (LIKELY((ftp = csound->FTnp2Find(csound, p->ftfn->ftable + j)) != NULL)) {
        if (UNLIKELY((unsigned int)ftp->flen != len)) {
          return csound->InitError(csound,
                                   Str("table in iftfn for ftmorf wrong size"));
        }
      }
      else {
        return csound->InitError(csound, Str("table in iftfn for ftmorf "
                                             "does not exist"));
      }
    } while (++j < (int)p->ftfn->flen);

    p->len = len;
    p->ftndx = -FL(1.0);
    return OK;
}

static int ftmorf(CSOUND *csound, FTMORF *p)
{
    unsigned int j = 0;
    int i;
    MYFLT f;
    FUNC *ftp1, *ftp2;

    if (*p->kftndx >= p->ftfn->flen) *p->kftndx = (MYFLT)(p->ftfn->flen - 1);
    i = (int)*p->kftndx;
    f = *p->kftndx - i;
    if (p->ftndx != *p->kftndx) {
      p->ftndx = *p->kftndx;
      ftp1 = csound->FTnp2Find(csound, p->ftfn->ftable + i++);
      ftp2 = csound->FTnp2Find(csound, p->ftfn->ftable + i--);
      do {
        *(p->resfn->ftable + j) = (*(ftp1->ftable + j) * (1-f)) +
          (*(ftp2->ftable + j) * f);
      } while (++j < p->len);
    }
    return OK;
}

/* end of ugmoss.c */
#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "dconv",  S(DCONV), TR, 5, "a", "aii",   (SUBR)dconvset, NULL, (SUBR)dconv },
{ "vcomb", S(VCOMB),  0,5, "a", "akxioo", (SUBR)vcombset, NULL, (SUBR)vcomb   },
{ "valpass", S(VCOMB),0,5, "a", "akxioo", (SUBR)vcombset, NULL, (SUBR)valpass },
{ "ftmorf", S(FTMORF),TR, 3, "",  "kii",  (SUBR)ftmorfset,  (SUBR)ftmorf,    },
{ "##and.ii",  S(AOP),  0,1, "i", "ii",   (SUBR)and_kk                  },
{ "##and.kk",  S(AOP),  0,2, "k", "kk",   NULL,   (SUBR)and_kk          },
{ "##and.ka",  S(AOP),  0,4, "a", "ka",   NULL,   NULL,   (SUBR)and_ka  },
{ "##and.ak",  S(AOP),  0,4, "a", "ak",   NULL,   NULL,   (SUBR)and_ak  },
{ "##and.aa",  S(AOP),  0,4, "a", "aa",   NULL,   NULL,   (SUBR)and_aa  },
{ "##or.ii",   S(AOP),  0,1, "i", "ii",   (SUBR)or_kk                   },
{ "##or.kk",   S(AOP),  0,2, "k", "kk",   NULL,   (SUBR)or_kk           },
{ "##or.ka",   S(AOP),  0,4, "a", "ka",   NULL,   NULL,   (SUBR)or_ka   },
{ "##or.ak",   S(AOP),  0,4, "a", "ak",   NULL,   NULL,   (SUBR)or_ak   },
{ "##or.aa",   S(AOP),  0,4, "a", "aa",   NULL,   NULL,   (SUBR)or_aa   },
{ "##xor.ii",  S(AOP),  0,1, "i", "ii",   (SUBR)xor_kk                  },
{ "##xor.kk",  S(AOP),  0,2, "k", "kk",   NULL,   (SUBR)xor_kk          },
{ "##xor.ka",  S(AOP),  0,4, "a", "ka",   NULL,   NULL,   (SUBR)xor_ka  },
{ "##xor.ak",  S(AOP),  0,4, "a", "ak",   NULL,   NULL,   (SUBR)xor_ak  },
{ "##xor.aa",  S(AOP),  0,4, "a", "aa",   NULL,   NULL,   (SUBR)xor_aa  },
{ "##not.i",   S(AOP),  0,1, "i", "i",    (SUBR)not_k                   },
{ "##not.k",   S(AOP),  0,2, "k", "k",    NULL,   (SUBR)not_k           },
{ "##not.a",   S(AOP),  0,4, "a", "a",    NULL,   NULL,   (SUBR)not_a   },
{ "##shl.ii",  S(AOP),  0,1, "i", "ii",   (SUBR) shift_left_kk          },
{ "##shl.kk",  S(AOP),  0,2, "k", "kk",   NULL, (SUBR) shift_left_kk    },
{ "##shl.ka", S(AOP), 0,4, "a", "ka", NULL, NULL, (SUBR) shift_left_ka  },
{ "##shl.ak", S(AOP), 0,4, "a", "ak", NULL, NULL, (SUBR) shift_left_ak  },
{ "##shl.aa", S(AOP), 0,4, "a", "aa", NULL, NULL, (SUBR) shift_left_aa  },
{ "##shr.ii",  S(AOP),  0,1, "i", "ii",   (SUBR) shift_right_kk         },
{ "##shr.kk",  S(AOP),  0,2, "k", "kk",   NULL, (SUBR) shift_right_kk   },
{ "##shr.ka", S(AOP), 0,4, "a", "ka", NULL, NULL, (SUBR) shift_right_ka },
{ "##shr.ak", S(AOP), 0,4, "a", "ak", NULL, NULL, (SUBR) shift_right_ak },
{ "##shr.aa", S(AOP), 0,4, "a", "aa", NULL, NULL, (SUBR) shift_right_aa }
};

int ugmoss_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}
